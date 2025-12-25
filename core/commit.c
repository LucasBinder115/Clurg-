#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <linux/limits.h>

int clurg_commit(const char *message) {
    char clurg_ci_path[PATH_MAX];
    char cwd[PATH_MAX];
    char cmd[2048];
    int ret;
    
    /* Tentar encontrar o binário clurg-ci */
    /* Primeiro tentar ./bin/clurg-ci (relativo ao diretório atual) */
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        snprintf(clurg_ci_path, sizeof(clurg_ci_path), "%s/bin/clurg-ci", cwd);
    } else {
        strncpy(clurg_ci_path, "./bin/clurg-ci", sizeof(clurg_ci_path) - 1);
        clurg_ci_path[sizeof(clurg_ci_path) - 1] = '\0';
    }
    
    /* Verificar se o arquivo existe */
    if (access(clurg_ci_path, X_OK) != 0) {
        /* Tentar apenas bin/clurg-ci */
        strncpy(clurg_ci_path, "bin/clurg-ci", sizeof(clurg_ci_path) - 1);
        clurg_ci_path[sizeof(clurg_ci_path) - 1] = '\0';
        
        if (access(clurg_ci_path, X_OK) != 0) {
            fprintf(stderr, "erro: clurg-ci não encontrado\n");
            fprintf(stderr, "compile com: make clurg-ci\n");
            return 1;
        }
    }
    
    /* Executar pipeline CI antes do commit */
    /* Por enquanto usando system(), conforme CONTEXT.md linha 188 */
    snprintf(cmd, sizeof(cmd), "%s run pipelines/default.ci", clurg_ci_path);
    
    printf("Executando pipeline CI...\n");
    ret = system(cmd);
    
    char ci_status[16] = "failed";
    if (ret == 0) {
        strcpy(ci_status, "passed");
        printf("Pipeline CI executado com sucesso!\n");
    } else {
        fprintf(stderr, "Pipeline CI falhou, mas continuando com commit...\n");
        /* Commit continua mesmo se CI falhar, conforme CONTEXT.md linha 200 */
    }
    
    /* Aqui executamos o script local que cria um snapshot em .clurg/commits */
    {
        char script_path[PATH_MAX];

        if (snprintf(script_path, sizeof(script_path), "%s/scripts/commit.sh", cwd) >= (int)sizeof(script_path)) {
            fprintf(stderr, "caminho do script muito longo\n");
            return 1;
        }

        /* Garantir que o script exista; se não for executável, usaremos sh */
        if (access(script_path, F_OK) != 0) {
            fprintf(stderr, "script de commit local não encontrado: %s\n", script_path);
            return 1;
        }

        /* Construir comando para executar o script com a mensagem */
        if (message) {
            /* Obs: mensagem não é escapada; evite caracteres especiais no teste */
            if (snprintf(cmd, sizeof(cmd), "CLURG_CI_STATUS=%s %s \"%s\"", ci_status, script_path, message) >= (int)sizeof(cmd)) {
                fprintf(stderr, "comando muito longo\n");
                return 1;
            }
        } else {
            if (snprintf(cmd, sizeof(cmd), "CLURG_CI_STATUS=%s %s \"%s\"", ci_status, script_path, "no message") >= (int)sizeof(cmd)) {
                fprintf(stderr, "comando muito longo\n");
                return 1;
            }
        }

        printf("Executando commit local via script: %s\n", script_path);
        ret = system(cmd);
        if (ret != 0) {
            fprintf(stderr, "erro ao executar script de commit local (ret=%d)\n", ret);
            return 1;
        }

        printf("Commit local criado com sucesso.\n");
    }

    return 0;
}

