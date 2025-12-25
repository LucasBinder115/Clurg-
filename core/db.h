#ifndef CLURG_DB_H
#define CLURG_DB_H

#include <libpq-fe.h>
#include <stdbool.h>

/* Configuração de conexão PostgreSQL */
typedef struct {
    char host[256];
    int port;
    char database[256];
    char user[256];
    char password[256];
} db_config_t;

/* Conexão global */
extern PGconn *db_conn;

/* Funções */
int db_init(const char *config_file);
void db_close(void);
int db_test_connection(void);
PGresult *db_query(const char *query);
int db_check_table_exists(const char *table_name);

/* Ler primeiro commit do banco */
PGresult *db_read_first_commit(void);

#endif /* CLURG_DB_H */