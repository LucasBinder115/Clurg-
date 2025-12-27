#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clone.h"
#include "commit.h"
#include "deploy.h"
#include "push.h"
#include "init.h"

static void usage(const char *prog_name) {
  printf("Uso: %s <comando> [opções]\n", prog_name);
  printf("Comandos:\n");
  printf("  init                 - Inicializar novo repositório\n");
  printf("  status               - Mostrar estado do repositório\n");
  printf("  add .                - Adicionar arquivos (auto-stage)\n");
  printf("  commit [mensagem]    - Fazer commit\n");
  printf("  log                  - Ver histórico de commits\n");
  printf("  push <remote>        - Enviar commits\n");
  printf("  clone <url>          - Clonar repositório\n");
  printf("  deploy <env> <id>    - Deploy para ambiente\n");
  printf("  plugin <cmd>         - Gerenciar plugins\n");
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

  if (strcmp(argv[1], "init") == 0) {
      return clurg_init();
  } else if (strcmp(argv[1], "status") == 0) {
      return system("./.clurg/scripts/status.sh");
  } else if (strcmp(argv[1], "log") == 0) {
      return system("./.clurg/scripts/log.sh");
  } else if (strcmp(argv[1], "add") == 0) {
      printf("ℹ️  Clurg usa modelo snapshot-based. Todos os arquivos serão incluídos no commit.\n");
      return 0;
  } else if (strcmp(argv[1], "commit") == 0) {
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
  } else {
    fprintf(stderr, "comando desconhecido: %s\n", argv[1]);
    usage(argv[0]);
    return 1;
  }
}
