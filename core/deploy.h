#ifndef CLURG_DEPLOY_H
#define CLURG_DEPLOY_H

#include <time.h>

// Estrutura para configuração de deploy
typedef struct {
  char environment[256];
  char deploy_cmd[2048];
  char healthcheck_cmd[2048];
  int timeout_seconds;
} deploy_config_t;

// Função principal de deploy
int clurg_deploy(const char *environment, const char *commit_id);

// Funções auxiliares
int deploy_load_config(const char *project_name, deploy_config_t *config, const char *environment);
int deploy_create_backup(const char *project_name, const char *environment);
int deploy_execute(const char *project_name, const char *environment, const char *commit_id,
                   deploy_config_t *config);
int deploy_healthcheck(deploy_config_t *config);
int deploy_update_current(const char *project_name, const char *environment, const char *commit_id);
int deploy_log(const char *project_name, const char *environment, const char *commit_id,
               const char *status, const char *message);

#endif  // CLURG_DEPLOY_H