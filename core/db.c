#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>

/* Conexão global */
PGconn *db_conn = NULL;

/* Carregar configuração do arquivo */
static int db_load_config(const char *config_file, db_config_t *config) {
    FILE *fp = fopen(config_file, "r");
    if (!fp) {
        fprintf(stderr, "erro: não conseguiu abrir arquivo de config DB: %s\n", config_file);
        return 1;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        char *key = strtok(line, "=");
        char *value = strtok(NULL, "\n");

        if (!key || !value) continue;

        // Remover espaços
        while (*key == ' ' || *key == '\t') key++;
        char *end = key + strlen(key) - 1;
        while (end > key && (*end == ' ' || *end == '\t')) *end-- = '\0';

        while (*value == ' ' || *value == '\t') value++;
        end = value + strlen(value) - 1;
        while (end > value && (*end == ' ' || *end == '\t' || *end == '\n')) *end-- = '\0';

        if (strcmp(key, "host") == 0) {
            strncpy(config->host, value, sizeof(config->host) - 1);
        } else if (strcmp(key, "port") == 0) {
            config->port = atoi(value);
        } else if (strcmp(key, "database") == 0) {
            strncpy(config->database, value, sizeof(config->database) - 1);
        } else if (strcmp(key, "user") == 0) {
            strncpy(config->user, value, sizeof(config->user) - 1);
        } else if (strcmp(key, "password") == 0) {
            strncpy(config->password, value, sizeof(config->password) - 1);
        }
    }

    fclose(fp);
    return 0;
}

/* Inicializar conexão com banco */
int db_init(const char *config_file) {
    db_config_t config = {0};

    // Valores padrão
    strcpy(config.host, "localhost");
    config.port = 5432;
    strcpy(config.database, "clurg");
    strcpy(config.user, "clurg");
    strcpy(config.password, "");

    // Carregar do arquivo se existir
    if (config_file) {
        db_load_config(config_file, &config);
    }

    // Construir string de conexão
    char conninfo[1024];
    snprintf(conninfo, sizeof(conninfo),
             "host=%s port=%d dbname=%s user=%s password=%s",
             config.host, config.port, config.database,
             config.user, config.password);

    // Conectar
    db_conn = PQconnectdb(conninfo);
    if (PQstatus(db_conn) != CONNECTION_OK) {
        fprintf(stderr, "erro de conexão: %s\n", PQerrorMessage(db_conn));
        PQfinish(db_conn);
        db_conn = NULL;
        return 1;
    }

    printf("✅ Conectado ao PostgreSQL: %s@%s:%d/%s\n",
           config.user, config.host, config.port, config.database);

    return 0;
}

/* Fechar conexão */
void db_close(void) {
    if (db_conn) {
        PQfinish(db_conn);
        db_conn = NULL;
        printf("🔌 Conexão PostgreSQL fechada\n");
    }
}

/* Teste básico de conexão */
int db_test_connection(void) {
    if (!db_conn) {
        fprintf(stderr, "erro: conexão não inicializada\n");
        return 1;
    }

    PGresult *res = PQexec(db_conn, "SELECT 1 as test");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "erro na query de teste: %s\n", PQerrorMessage(db_conn));
        PQclear(res);
        return 1;
    }

    int rows = PQntuples(res);
    if (rows > 0) {
        char *value = PQgetvalue(res, 0, 0);
        printf("🧪 Teste de conexão OK: SELECT 1 = %s\n", value);
    }

    PQclear(res);
    return 0;
}

/* Executar query genérica */
PGresult *db_query(const char *query) {
    if (!db_conn) {
        fprintf(stderr, "erro: conexão não inicializada\n");
        return NULL;
    }

    PGresult *res = PQexec(db_conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "erro na query: %s\nQuery: %s\n", PQerrorMessage(db_conn), query);
        PQclear(res);
        return NULL;
    }

    return res;
}

/* Verificar se tabela existe */
int db_check_table_exists(const char *table_name) {
    char query[256];
    snprintf(query, sizeof(query),
             "SELECT EXISTS (SELECT 1 FROM information_schema.tables "
             "WHERE table_schema = 'public' AND table_name = '%s')", table_name);

    PGresult *res = db_query(query);
    if (!res) return 0;

    int exists = 0;
    if (PQntuples(res) > 0) {
        char *value = PQgetvalue(res, 0, 0);
        exists = strcmp(value, "t") == 0;
    }

    PQclear(res);
    return exists;
}

/* Ler primeiro commit do banco */
PGresult *db_read_first_commit(void) {
    if (!db_conn) {
        fprintf(stderr, "erro: conexão não inicializada\n");
        return NULL;
    }

    const char *query = "SELECT id, project_id, commit_hash, message, author, "
                        "timestamp, parent_hash, created_at "
                        "FROM clurg.commits "
                        "ORDER BY created_at ASC LIMIT 1";

    PGresult *res = PQexec(db_conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "erro ao ler primeiro commit: %s\n", PQerrorMessage(db_conn));
        PQclear(res);
        return NULL;
    }

    return res;
}