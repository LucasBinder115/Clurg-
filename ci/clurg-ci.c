#include "ci.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <linux/limits.h>

#define MAX_PATH 512

static void usage(const char *prog_name) {
    fprintf(stderr, "Uso: %s run [pipeline.ci]\n", prog_name);
    fprintf(stderr, "  run: executar pipeline\n");
    fprintf(stderr, "  [pipeline.ci]: arquivo de pipeline (padrão: pipelines/default.ci)\n");
}

static char *get_clurg_root(void) {
    static char root[PATH_MAX];
    char cwd[PATH_MAX];
    char *p;
    
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        return NULL;
    }
    
    /* Procurar diretório .clurg subindo na árvore */
    strncpy(root, cwd, sizeof(root) - 1);
    root[sizeof(root) - 1] = '\0';
    
    while (1) {
        char test_path[PATH_MAX];
        snprintf(test_path, sizeof(test_path), "%s/.clurg", root);
        
        if (access(test_path, F_OK) == 0) {
            return root;
        }
        
        p = strrchr(root, '/');
        if (!p || p == root) {
            /* Não encontrou, usar diretório atual */
            return getcwd(root, sizeof(root)) ? root : NULL;
        }
        
        *p = '\0';
    }
}

int main(int argc, char *argv[]) {
    ci_pipeline_t pipeline;
    char config_file[MAX_PATH];
    char workspace_path[MAX_PATH];
    char log_dir[MAX_PATH];
    char *clurg_root;
    int i;
    int ret = 0;
    
    if (argc < 2 || strcmp(argv[1], "run") != 0) {
        usage(argv[0]);
        return 1;
    }
    
    /* Determinar arquivo de pipeline */
    if (argc >= 3) {
        strncpy(config_file, argv[2], sizeof(config_file) - 1);
        config_file[sizeof(config_file) - 1] = '\0';
    } else {
        strncpy(config_file, "pipelines/default.ci", sizeof(config_file) - 1);
        config_file[sizeof(config_file) - 1] = '\0';
    }
    
    /* Encontrar raiz do projeto Clurg */
    clurg_root = get_clurg_root();
    if (!clurg_root) {
        fprintf(stderr, "erro: não foi possível determinar raiz do projeto\n");
        return 1;
    }
    
    /* Preparar diretório de logs */
    snprintf(log_dir, sizeof(log_dir), "%s/.clurg/ci/logs", clurg_root);
    
    /* Inicializar logger */
    if (logger_init(log_dir) != 0) {
        fprintf(stderr, "erro ao inicializar logger\n");
        return 1;
    }
    
    /* Parsear pipeline */
    if (config_parse(config_file, &pipeline) != 0) {
        fprintf(stderr, "erro ao parsear pipeline: %s\n", config_file);
        logger_cleanup();
        return 1;
    }
    
    printf("Executando pipeline: %s\n", pipeline.name);
    printf("Steps: %zu\n", pipeline.step_count);
    
    /* Criar workspace */
    if (workspace_create(workspace_path, sizeof(workspace_path)) != 0) {
        fprintf(stderr, "erro ao criar workspace\n");
        config_free(&pipeline);
        logger_cleanup();
        return 1;
    }
    
    /* Setup workspace - copiar estado do repo */
    if (workspace_setup(workspace_path, clurg_root) != 0) {
        fprintf(stderr, "erro ao configurar workspace\n");
        workspace_cleanup(workspace_path);
        config_free(&pipeline);
        logger_cleanup();
        return 1;
    }
    
    printf("Workspace criado em: %s\n", workspace_path);
    
    /* Executar cada step no workspace */
    for (i = 0; i < (int)pipeline.step_count; i++) {
        int exit_code;
        
        printf("Executando step: %s\n", pipeline.steps[i].name);
        printf("  Comando: %s\n", pipeline.steps[i].command);
        
        /* Executar no workspace */
        exit_code = executor_run_step(&pipeline.steps[i], workspace_path);
        
        if (exit_code == 0) {
            logger_log_step(pipeline.steps[i].name, 0, 0);
        } else {
            logger_log_step(pipeline.steps[i].name, 1, exit_code);
            ret = 1; /* Pipeline falhou */
            /* Continuar executando os outros steps? 
             * Por enquanto, vamos parar no primeiro erro conforme a regra de ouro */
            break;
        }
    }
    
    /* Limpar workspace */
    workspace_cleanup(workspace_path);
    
    /* Limpar recursos */
    config_free(&pipeline);
    logger_cleanup();
    
    if (ret == 0) {
        printf("Pipeline executado com sucesso!\n");
    } else {
        printf("Pipeline falhou!\n");
    }
    
    return ret;
}

