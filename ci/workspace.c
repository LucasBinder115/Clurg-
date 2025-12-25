#define _GNU_SOURCE
#include "ci.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

int workspace_create(char *workspace_path, size_t path_size) {
    char template[] = "/tmp/clurg-ci-XXXXXX";
    char *tmpdir = mkdtemp(template);
    
    if (!tmpdir) {
        perror("mkdtemp");
        return -1;
    }
    
    if (strlen(tmpdir) >= path_size) {
        fprintf(stderr, "workspace path too long\n");
        rmdir(tmpdir);
        return -1;
    }
    
    strncpy(workspace_path, tmpdir, path_size - 1);
    workspace_path[path_size - 1] = '\0';
    
    return 0;
}

static int should_ignore(const char *name) {
    /* Ignorar apenas diretórios específicos do sistema de controle de versão/CI */
    return strcmp(name, ".") == 0 ||
           strcmp(name, "..") == 0 ||
           strcmp(name, ".clurg") == 0 ||
           strcmp(name, ".git") == 0;
           /* Não ignorar outros arquivos ocultos, podem ser necessários para o build */
}

static int copy_file(const char *src_path, const char *dst_path) {
    FILE *src, *dst;
    char buffer[8192];
    size_t n;
    
    src = fopen(src_path, "rb");
    if (!src) {
        perror("fopen src");
        return -1;
    }
    
    dst = fopen(dst_path, "wb");
    if (!dst) {
        perror("fopen dst");
        fclose(src);
        return -1;
    }
    
    while ((n = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, n, dst) != n) {
            perror("fwrite");
            fclose(src);
            fclose(dst);
            return -1;
        }
    }
    
    fclose(src);
    fclose(dst);
    return 0;
}

static int copy_dir_recursive(const char *src_dir, const char *dst_dir) {
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    char src_path[PATH_MAX];
    char dst_path[PATH_MAX];
    int ret = 0;
    
    /* Criar diretório destino */
    if (mkdir(dst_dir, 0755) != 0 && errno != EEXIST) {
        perror("mkdir dst_dir");
        return -1;
    }
    
    dir = opendir(src_dir);
    if (!dir) {
        perror("opendir");
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (should_ignore(entry->d_name)) {
            continue;
        }
        
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst_dir, entry->d_name);
        
        if (stat(src_path, &st) != 0) {
            continue;
        }
        
        if (S_ISDIR(st.st_mode)) {
            /* Copiar diretório recursivamente */
            if (copy_dir_recursive(src_path, dst_path) != 0) {
                ret = -1;
                break;
            }
        } else if (S_ISREG(st.st_mode)) {
            /* Copiar arquivo regular */
            if (copy_file(src_path, dst_path) != 0) {
                ret = -1;
                break;
            }
            /* Copiar permissões */
            chmod(dst_path, st.st_mode);
        }
        /* Ignorar outros tipos (symlinks, etc) */
    }
    
    closedir(dir);
    return ret;
}

int workspace_setup(const char *workspace_path, const char *repo_path) {
    char abs_repo_path[PATH_MAX];
    char *real_repo_path;
    
    /* Verificar se o diretório existe */
    struct stat st;
    if (stat(repo_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "repo_path não é um diretório válido: %s\n", repo_path);
        return -1;
    }
    
    /* Obter caminho absoluto do repo */
    real_repo_path = realpath(repo_path, abs_repo_path);
    if (!real_repo_path) {
        perror("realpath repo_path");
        return -1;
    }
    
    /* Copiar conteúdo do repo para o workspace */
    if (copy_dir_recursive(abs_repo_path, workspace_path) != 0) {
        fprintf(stderr, "erro ao copiar estado do repo para workspace\n");
        return -1;
    }
    
    return 0;
}

int workspace_cleanup(const char *workspace_path) {
    char cmd[1024];
    int ret;
    
    /* Remover diretório temporário recursivamente */
    if (snprintf(cmd, sizeof(cmd), "rm -rf %s", workspace_path) >= (int)sizeof(cmd)) {
        fprintf(stderr, "workspace_path muito longo\n");
        return -1;
    }
    
    ret = system(cmd);
    
    if (ret != 0) {
        fprintf(stderr, "falha ao limpar workspace: %s\n", workspace_path);
        return -1;
    }
    
    return 0;
}

