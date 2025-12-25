#ifndef API_H
#define API_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>

// Constantes
#define MAX_PATH 4096
#define MAX_BODY_SIZE 32768

// Estrutura para resposta da API
typedef struct {
    int status_code;
    char content_type[64];
    char body[MAX_BODY_SIZE];
} api_response_t;

// Funções da API
void api_system_status(api_response_t *response, const char *clurg_root);
void api_projects_list(api_response_t *response, const char *clurg_root);
void api_projects_create(api_response_t *response, const char *clurg_root, const char *body);
void api_project_get(api_response_t *response, const char *clurg_root, const char *project_name);
void api_project_commits(api_response_t *response, const char *clurg_root, const char *project_name);
void api_project_deploy(api_response_t *response, const char *clurg_root, const char *project_name, const char *body);
void api_projects_delete(api_response_t *response, const char *clurg_root, const char *project_name);
void api_system_metrics(api_response_t *response, const char *clurg_root);
void api_system_logs(api_response_t *response, const char *clurg_root);
void api_project_ci_run(api_response_t *response, const char *clurg_root, const char *project_name);
void api_project_ci_status(api_response_t *response, const char *clurg_root, const char *project_name);
void api_webhooks_test(api_response_t *response, const char *clurg_root, const char *body);

#endif // API_H