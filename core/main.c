#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "commit.h"
#include "push.h"
#include "clone.h"
#include "deploy.h"
#include "db.h"

static void usage(const char *prog_name) {
    printf("Uso: %s <comando> [opções]\n", prog_name);
    printf("Comandos:\n");
    printf("  commit [mensagem]  - Fazer commit (executa CI antes)\n");
    printf("  push [project] <remote_url>  - Enviar commits para repositório remoto\n");
    printf("  clone <project> <remote_url> - Clonar projeto de repositório remoto\n");
    printf("  deploy <env> <commit_id>     - Deploy commit para ambiente específico\n");
    printf("  plugin <command>   - Gerenciar plugins (list, install, run)\n");
    printf("  db-test            - Testar conexão com banco de dados\n");
    printf("  db-first-commit    - Mostrar primeiro commit do banco de dados\n");
}

static int clurg_plugin(int argc, char *argv[]) {
    if (argc < 1) {
        printf("Uso: clurg plugin <comando> [opções]\n");
        printf("Comandos:\n");
        printf("  list                    - Listar plugins instalados\n");
        printf("  install <url>           - Instalar plugin do repositório git\n");
        printf("  run <plugin> [args...]  - Executar plugin específico\n");
        return 1;
    }
    
    // Construir comando para executar o script de plugins
    char cmd[2048] = "./scripts/plugin-manager.sh";
    
    for (int i = 0; i < argc; i++) {
        strcat(cmd, " \"");
        // Escapar aspas nos argumentos
        char *arg = argv[i];
        char escaped[1024] = {0};
        int j = 0;
        for (int k = 0; arg[k] && j < sizeof(escaped) - 1; k++) {
            if (arg[k] == '"') {
                escaped[j++] = '\\';
            }
            escaped[j++] = arg[k];
        }
        escaped[j] = '\0';
        strcat(cmd, escaped);
        strcat(cmd, "\"");
    }
    
    // Executar comando
    int result = system(cmd);
    return result == 0 ? 0 : 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "commit") == 0) {
        const char *message = NULL;
        if (argc >= 3) {
            if (strcmp(argv[2], "-m") == 0 || strcmp(argv[2], "--message") == 0) {
                if (argc >= 4) {
                    message = argv[3];
                } else {
                    fprintf(stderr, "erro: %s requer um argumento de mensagem\n", argv[2]);
                    usage(argv[0]);
                    return 1;
                }
            } else {
                message = argv[2];
            }
        }
        return clurg_commit(message);
    } else if (strcmp(argv[1], "push") == 0) {
        const char *arg1 = (argc >= 3) ? argv[2] : NULL;
        const char *arg2 = (argc >= 4) ? argv[3] : NULL;
        return clurg_push(arg1, arg2);
    } else if (strcmp(argv[1], "clone") == 0) {
        if (argc < 4) {
            fprintf(stderr, "erro: clone requer <project> <remote_url>\n");
            usage(argv[0]);
            return 1;
        }
        return clurg_clone(argv[2], argv[3]);
    } else if (strcmp(argv[1], "deploy") == 0) {
        if (argc < 4) {
            fprintf(stderr, "erro: deploy requer <environment> <commit_id>\n");
            usage(argv[0]);
            return 1;
        }
        return clurg_deploy(argv[2], argv[3]);
    } else if (strcmp(argv[1], "plugin") == 0) {
        return clurg_plugin(argc - 2, argv + 2);
    } else if (strcmp(argv[1], "db-test") == 0) {
        if (db_init(".clurg/database.conf") != 0) {
            return 1;
        }
        int result = db_test_connection();
        db_close();
        return result;
    } else if (strcmp(argv[1], "db-first-commit") == 0) {
        if (db_init(".clurg/database.conf") != 0) {
            return 1;
        }
        
        PGresult *res = db_read_first_commit();
        if (!res) {
            db_close();
            return 1;
        }
        
        int rows = PQntuples(res);
        if (rows == 0) {
            printf("📭 Nenhum commit encontrado no banco de dados\n");
        } else {
            printf("📋 Primeiro commit no banco de dados:\n");
            printf("ID: %s\n", PQgetvalue(res, 0, 0));
            printf("Project ID: %s\n", PQgetvalue(res, 0, 1));
            printf("Commit Hash: %s\n", PQgetvalue(res, 0, 2));
            printf("Message: %s\n", PQgetvalue(res, 0, 3));
            printf("Author: %s\n", PQgetvalue(res, 0, 4));
            printf("Timestamp: %s\n", PQgetvalue(res, 0, 5));
            printf("Parent Hash: %s\n", PQgetvalue(res, 0, 6));
            printf("Created At: %s\n", PQgetvalue(res, 0, 7));
        }
        
        PQclear(res);
        db_close();
        return 0;
    } else {
        fprintf(stderr, "comando desconhecido: %s\n", argv[1]);
        usage(argv[0]);
        return 1;
    }
}
