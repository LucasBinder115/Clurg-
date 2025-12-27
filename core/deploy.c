#include "deploy.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Função auxiliar para obter timestamp formatado
static void get_timestamp(char *buffer, size_t size) {
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  strftime(buffer, size, "%Y%m%d_%H%M%S", tm_info);
}

// Função auxiliar para criar diretórios recursivamente
static int create_directories(const char *path) {
  char tmp[4096];
  char *p = NULL;
  size_t len;

  snprintf(tmp, sizeof(tmp), "%s", path);
  len = strlen(tmp);
  if (tmp[len - 1] == '/') {
    tmp[len - 1] = 0;
  }
  for (p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = 0;
      if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        return -1;
      }
      *p = '/';
    }
  }
  if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
    return -1;
  }
  return 0;
}

// Função auxiliar para executar comando (simplificada)
static int execute_command(const char *cmd) {
  int result = system(cmd);
  if (result == -1) {
    return -1;
  }
  return WEXITSTATUS(result);
}

// Carregar configuração de deploy
int deploy_load_config(const char *project_name, deploy_config_t *config, const char *environment) {
  char config_path[4096];

  // Procurar arquivo clurg.deploy no diretório atual (raiz do projeto)
  snprintf(config_path, sizeof(config_path), "clurg.deploy");

  FILE *file = fopen(config_path, "r");
  if (!file) {
    fprintf(stderr, "Arquivo de configuração não encontrado: %s\n", config_path);
    return -1;
  }

  char line[2048];
  int in_environment = 0;
  memset(config, 0, sizeof(deploy_config_t));

  while (fgets(line, sizeof(line), file)) {
    // Remover newline
    line[strcspn(line, "\n")] = 0;

    // Ignorar linhas vazias
    if (line[0] == '\0') {
      continue;
    }

    // Verificar se estamos no ambiente correto
    if (strstr(line, "# Ambiente:") == line) {
      char env_name[256];
      if (sscanf(line, "# Ambiente: %255s", env_name) == 1) {
        in_environment = (strcmp(env_name, environment) == 0);
        if (in_environment) {
          strcpy(config->environment, env_name);
        }
      }
      continue;
    }

    // Ignorar comentários fora de ambientes
    if (line[0] == '#' || !in_environment) {
      continue;
    }

    if (!in_environment) {
      continue;
    }

    // Parse das configurações
    if (strstr(line, "deploy:") == line) {
      char *value = strchr(line, ':');
      if (value) {
        value += 2;  // Pular ": "
        strcpy(config->deploy_cmd, value);
      }
    } else if (strstr(line, "healthcheck:") == line) {
      char *value = strchr(line, ':');
      if (value) {
        value += 2;  // Pular ": "
        strcpy(config->healthcheck_cmd, value);
      }
    } else if (strstr(line, "timeout:") == line) {
      char *value = strchr(line, ':');
      if (value) {
        value += 2;  // Pular ": "
        config->timeout_seconds = atoi(value);
      }
    }
  }

  fclose(file);

  if (config->environment[0] == '\0') {
    fprintf(stderr, "Ambiente '%s' não encontrado na configuração\n", environment);
    return -1;
  }

  // Valores padrão
  if (config->timeout_seconds == 0) {
    config->timeout_seconds = 300;  // 5 minutos
  }

  return 0;
}

// Criar backup do ambiente atual
int deploy_create_backup(const char *project_name, const char *environment) {
  char backup_dir[4096];
  char timestamp[32];
  char cmd[4096];

  get_timestamp(timestamp, sizeof(timestamp));
  snprintf(backup_dir, sizeof(backup_dir), ".clurg/projects/%s/deploy/%s/backups/deploy_%s",
           project_name, environment, timestamp);

  if (create_directories(backup_dir) != 0) {
    fprintf(stderr, "Erro ao criar diretório de backup: %s\n", backup_dir);
    return -1;
  }

  // Copiar arquivos atuais para backup
  snprintf(cmd, sizeof(cmd), "cp -r .clurg/projects/%s/deploy/%s/current %s/ 2>/dev/null || true",
           project_name, environment, backup_dir);

  if (system(cmd) != 0) {
    printf("📦 Backup criado: %s (nenhum deploy anterior)\n", timestamp);
  } else {
    printf("📦 Backup criado: %s\n", timestamp);
  }
  return 0;
}

// Executar deploy
int deploy_execute(const char *project_name, const char *environment, const char *commit_id,
                   deploy_config_t *config) {
  char deploy_dir[4096];
  char commit_path[4096];
  char cmd[4096];

  // Criar diretório de deploy se não existir
  snprintf(deploy_dir, sizeof(deploy_dir), ".clurg/projects/%s/deploy/%s", project_name,
           environment);

  if (create_directories(deploy_dir) != 0) {
    fprintf(stderr, "Erro ao criar diretório de deploy\n");
    return -1;
  }

  // Extrair commit para diretório temporário
  snprintf(commit_path, sizeof(commit_path), ".clurg/projects/%s/commits/%s.tar.gz", project_name,
           commit_id);

  if (access(commit_path, F_OK) != 0) {
    fprintf(stderr, "Commit %s não encontrado\n", commit_id);
    return -1;
  }

  // Extrair commit
  printf("📦 Extraindo commit %s...\n", commit_id);
  snprintf(cmd, sizeof(cmd), "cd %s && tar -xzf ../%s", deploy_dir, commit_path);
  if (system(cmd) != 0) {
    fprintf(stderr, "Erro ao extrair commit\n");
    return -1;
  }

  // Executar comando de deploy
  printf("⚙️  Executando deploy...\n");
  int result = execute_command(config->deploy_cmd);

  if (result != 0) {
    fprintf(stderr, "❌ Comando de deploy falhou (código: %d)\n", result);
    return -1;
  }

  printf("✅ Deploy executado com sucesso\n");
  return 0;
}

// Executar healthcheck
int deploy_healthcheck(deploy_config_t *config) {
  if (config->healthcheck_cmd[0] == '\0') {
    printf("🏥 Healthcheck não configurado, pulando...\n");
    return 0;
  }

  printf("🏥 Executando healthcheck...\n");
  int result = execute_command(config->healthcheck_cmd);

  if (result != 0) {
    fprintf(stderr, "❌ Healthcheck falhou (código: %d)\n", result);
    return -1;
  }

  printf("✅ Healthcheck OK\n");
  return 0;
}

// Atualizar arquivo current (simplificado)
int deploy_update_current(const char *project_name, const char *environment,
                          const char *commit_id) {
  char deploy_dir[4096];
  char current_file[4096];
  FILE *file;

  // Criar diretório se não existir
  snprintf(deploy_dir, sizeof(deploy_dir), ".clurg/projects/%s/deploy/%s", project_name,
           environment);

  if (create_directories(deploy_dir) != 0) {
    fprintf(stderr, "Erro ao criar diretório de deploy\n");
    return -1;
  }

  // Arquivo current
  snprintf(current_file, sizeof(current_file), "%s/current", deploy_dir);

  file = fopen(current_file, "w");
  if (!file) {
    fprintf(stderr, "Erro ao criar arquivo current\n");
    return -1;
  }

  fprintf(file, "%s\n", commit_id);
  fclose(file);

  printf("🔗 Current atualizado para %s\n", commit_id);
  return 0;
}

// Registrar log de deploy
int deploy_log(const char *project_name, const char *environment, const char *commit_id,
               const char *status, const char *message) {
  char log_dir[4096];
  char log_file[4096];
  char timestamp[32];
  FILE *file;

  get_timestamp(timestamp, sizeof(timestamp));

  // Criar diretório de logs
  snprintf(log_dir, sizeof(log_dir), ".clurg/projects/%s/deploy/%s/logs", project_name,
           environment);

  if (create_directories(log_dir) != 0) {
    fprintf(stderr, "Erro ao criar diretório de logs\n");
    return -1;
  }

  // Arquivo de log
  snprintf(log_file, sizeof(log_file), "%s/deploy_%s.log", log_dir, timestamp);

  file = fopen(log_file, "w");
  if (!file) {
    fprintf(stderr, "Erro ao criar arquivo de log\n");
    return -1;
  }

  fprintf(file, "[START] %s\n", timestamp);
  fprintf(file, "[ENV] %s\n", environment);
  fprintf(file, "[COMMIT] %s\n", commit_id);
  fprintf(file, "[STATUS] %s\n", status);
  if (message && message[0]) {
    fprintf(file, "[MESSAGE] %s\n", message);
  }
  fprintf(file, "[END]\n");

  fclose(file);

  printf("📊 Log salvo: %s\n", log_file);
  return 0;
}

// Função principal de deploy
int clurg_deploy(const char *environment, const char *commit_id) {
  char cwd[4096];
  char project_name[256];
  deploy_config_t config;

  if (!environment || !commit_id) {
    fprintf(stderr, "Uso: clurg deploy <environment> <commit_id>\n");
    return 1;
  }

  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    fprintf(stderr, "Erro ao obter diretório atual\n");
    return 1;
  }

  // Extrair nome do projeto do diretório atual
  char *last_slash = strrchr(cwd, '/');
  if (last_slash) {
    strcpy(project_name, last_slash + 1);
  } else {
    strcpy(project_name, cwd);
  }

  printf("🚀 Iniciando deploy para %s...\n", environment);
  printf("📁 Projeto: %s\n", project_name);
  printf("🔖 Commit: %s\n", commit_id);

  // 1. Carregar configuração
  if (deploy_load_config(project_name, &config, environment) != 0) {
    return 1;
  }

  printf("⚙️  Comando: %s\n", config.deploy_cmd);
  printf("🏥 Healthcheck: %s\n", config.healthcheck_cmd[0] ? config.healthcheck_cmd : "N/A");
  printf("⏱️  Timeout: %ds\n", config.timeout_seconds);

  // 2. Criar backup
  if (deploy_create_backup(project_name, environment) != 0) {
    deploy_log(project_name, environment, commit_id, "FAILED", "Erro ao criar backup");
    return 1;
  }

  // 3. Executar deploy
  if (deploy_execute(project_name, environment, commit_id, &config) != 0) {
    deploy_log(project_name, environment, commit_id, "FAILED", "Erro na execução do deploy");
    return 1;
  }

  // 4. Healthcheck
  if (deploy_healthcheck(&config) != 0) {
    deploy_log(project_name, environment, commit_id, "FAILED", "Healthcheck falhou");
    return 1;
  }

  // 5. Atualizar current
  if (deploy_update_current(project_name, environment, commit_id) != 0) {
    deploy_log(project_name, environment, commit_id, "FAILED", "Erro ao atualizar current");
    return 1;
  }

  // 6. Log de sucesso
  deploy_log(project_name, environment, commit_id, "SUCCESS", "Deploy concluído com sucesso");

  printf("✅ Deploy concluído com sucesso!\n");
  return 0;
}