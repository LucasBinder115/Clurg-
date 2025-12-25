#define _GNU_SOURCE
#include "ci.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <glob.h>
#include <limits.h>

static int has_wildcard(const char *str) {
    return strchr(str, '*') != NULL || strchr(str, '?') != NULL || strchr(str, '[') != NULL;
}

static int split_command_simple(const char *command, char **argv, int max_args) {
    const char *p = command;
    int argc = 0;
    int in_quote = 0;
    char *start = NULL;
    
    while (*p && argc < max_args - 1) {
        if (*p == '"') {
            in_quote = !in_quote;
            p++;
            continue;
        }
        
        if (!in_quote && isspace(*p)) {
            if (start) {
                size_t len = p - start;
                argv[argc] = malloc(len + 1);
                strncpy(argv[argc], start, len);
                argv[argc][len] = '\0';
                argc++;
                start = NULL;
            }
            p++;
            continue;
        }
        
        if (!start) {
            start = (char *)p;
        }
        p++;
    }
    
    if (start) {
        size_t len = p - start;
        argv[argc] = malloc(len + 1);
        strncpy(argv[argc], start, len);
        argv[argc][len] = '\0';
        argc++;
    }
    
    argv[argc] = NULL;
    return argc;
}

static int expand_argv_wildcards(char **argv, int argc, int max_args, const char *cwd) {
    char **new_argv = malloc(sizeof(char *) * max_args);
    int new_argc = 0;
    glob_t glob_result;
    int i, j;
    int old_cwd_fd = -1;
    
    if (!new_argv) {
        return -1;
    }
    
    /* Salvar diretório atual se necessário */
    if (cwd && cwd[0] != '\0') {
        old_cwd_fd = open(".", O_RDONLY);
        if (old_cwd_fd >= 0 && chdir(cwd) != 0) {
            close(old_cwd_fd);
            old_cwd_fd = -1;
        }
    }
    
    /* Primeiro argumento (comando) nunca expande */
    if (argc > 0) {
        new_argv[new_argc++] = argv[0];
    }
    
    /* Expandir wildcards nos argumentos restantes */
    for (i = 1; i < argc && new_argc < max_args - 1; i++) {
        if (has_wildcard(argv[i])) {
            int ret = glob(argv[i], GLOB_TILDE, NULL, &glob_result);
            
            if (ret == 0 && glob_result.gl_pathc > 0) {
                /* Expansão bem-sucedida */
                for (j = 0; j < (int)glob_result.gl_pathc && new_argc < max_args - 1; j++) {
                    new_argv[new_argc++] = strdup(glob_result.gl_pathv[j]);
                }
            } else {
                /* Não expandiu ou erro - usar original */
                new_argv[new_argc++] = strdup(argv[i]);
            }
            
            globfree(&glob_result);
        } else {
            /* Sem wildcard, copiar diretamente */
            new_argv[new_argc++] = strdup(argv[i]);
        }
    }
    
    new_argv[new_argc] = NULL;
    
    /* Restaurar diretório */
    if (old_cwd_fd >= 0) {
        fchdir(old_cwd_fd);
        close(old_cwd_fd);
    }
    
    /* Liberar argv antigo (exceto o primeiro que é compartilhado) */
    for (i = 1; i < argc; i++) {
        free(argv[i]);
    }
    
    /* Copiar novo argv de volta */
    for (i = 0; i < new_argc; i++) {
        argv[i] = new_argv[i];
    }
    argv[new_argc] = NULL;
    
    free(new_argv);
    return new_argc;
}

int executor_run_step(const ci_step_t *step, const char *workspace_path) {
    pid_t pid;
    int status;
    char *argv[256]; /* Aumentado para suportar expansão de wildcards */
    int argc;
    int i;
    int exit_code;
    char cwd[PATH_MAX];
    
    /* Obter diretório atual se workspace_path for NULL */
    if (workspace_path == NULL || workspace_path[0] == '\0') {
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            cwd[0] = '\0';
        }
        workspace_path = cwd[0] != '\0' ? cwd : NULL;
    }
    
    /* Dividir comando em argv */
    argc = split_command_simple(step->command, argv, 256);
    if (argc == 0) {
        fprintf(stderr, "comando vazio no step: %s\n", step->name);
        return -1;
    }
    
    /* Expandir wildcards nos argumentos (exceto no comando) */
    argc = expand_argv_wildcards(argv, argc, 256, workspace_path);
    if (argc < 0) {
        fprintf(stderr, "erro ao expandir wildcards no step: %s\n", step->name);
        /* Limpar argv parcial */
        for (i = 0; argv[i] != NULL; i++) {
            free(argv[i]);
        }
        return -1;
    }
    
    pid = fork();
    if (pid < 0) {
        perror("fork");
        /* Limpar argv */
        for (i = 0; i < argc; i++) {
            free(argv[i]);
        }
        return -1;
    }
    
    if (pid == 0) {
        /* Processo filho */
        /* Se workspace_path for fornecido, mudar para ele */
        /* Por enquanto, NULL significa usar diretório atual */
        if (workspace_path && workspace_path[0] != '\0' && chdir(workspace_path) != 0) {
            perror("chdir workspace");
            exit(1);
        }
        
        /* Executar comando */
        execvp(argv[0], argv);
        
        /* Se chegou aqui, execvp falhou */
        perror("execvp");
        exit(1);
    } else {
        /* Processo pai */
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            /* Limpar argv */
            for (i = 0; i < argc; i++) {
                free(argv[i]);
            }
            return -1;
        }
        
        /* Limpar argv */
        for (i = 0; i < argc; i++) {
            free(argv[i]);
        }
        
        if (WIFEXITED(status)) {
            exit_code = WEXITSTATUS(status);
            return exit_code;
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "step '%s' terminado por sinal: %d\n", 
                    step->name, WTERMSIG(status));
            return 128 + WTERMSIG(status); /* Convenção Unix */
        } else {
            return -1;
        }
    }
}

