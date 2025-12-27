#define _GNU_SOURCE
#include <limits.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PATH PATH_MAX

int clurg_clone(const char *project_name, const char *remote_url) {
  char cwd[PATH_MAX];
  char cmd[4096];
  char last_commit_id[256] = "";
  FILE *fp;
  int ret;

  if (!project_name || !remote_url) {
    fprintf(stderr, "erro: argumentos insuficientes\n");
    fprintf(stderr, "Uso: clurg clone <project_name> <remote_url>\n");
    return 1;
  }

  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("getcwd");
    return 1;
  }

  printf("Clonando projeto '%s' de %s...\n", project_name, remote_url);

  /* Primeiro, obter o ID do último commit */
  /* curl -s <remote_url>/api/last-commit/<project_name> */
  if (snprintf(cmd, sizeof(cmd), "curl -s \"%s/api/last-commit/%s\"", remote_url, project_name) >=
      (int)sizeof(cmd)) {
    fprintf(stderr, "comando muito longo\n");
    return 1;
  }

  fp = popen(cmd, "r");
  if (!fp) {
    perror("popen");
    return 1;
  }

  if (fgets(last_commit_id, sizeof(last_commit_id), fp)) {
    last_commit_id[strcspn(last_commit_id, "\n")] = 0; /* remover newline */
  }
  pclose(fp);

  if (strlen(last_commit_id) == 0) {
    fprintf(stderr, "erro: não foi possível obter o último commit\n");
    return 1;
  }

  printf("Último commit: %s\n", last_commit_id);

  /* Criar diretório .clurg/commits se não existir */
  char commits_dir[MAX_PATH];
  snprintf(commits_dir, sizeof(commits_dir), "%s/.clurg/commits", cwd);
  char mkdir_cmd[1024];
  snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"", commits_dir);
  system(mkdir_cmd);

  /* Baixar o commit */
  /* curl -s -o .clurg/commits/<id>.tar.gz <remote_url>/download/<project>/<id> */
  char archive_path[MAX_PATH];
  char meta_path[MAX_PATH];
  snprintf(archive_path, sizeof(archive_path), "%s/%s.tar.gz", commits_dir, last_commit_id);
  snprintf(meta_path, sizeof(meta_path), "%s/%s.meta", commits_dir, last_commit_id);

  if (snprintf(cmd, sizeof(cmd), "curl -s -o \"%s\" \"%s/download/%s/%s\"", archive_path,
               remote_url, project_name, last_commit_id) >= (int)sizeof(cmd)) {
    fprintf(stderr, "comando muito longo\n");
    return 1;
  }

  ret = system(cmd);
  if (ret != 0) {
    fprintf(stderr, "erro ao baixar commit (ret=%d)\n", ret);
    return 1;
  }

  /* Baixar o meta */
  if (snprintf(cmd, sizeof(cmd), "curl -s -o \"%s\" \"%s/download/%s/%s.meta\"", meta_path,
               remote_url, project_name, last_commit_id) >= (int)sizeof(cmd)) {
    fprintf(stderr, "comando muito longo\n");
    return 1;
  }

  ret = system(cmd);
  if (ret != 0) {
    fprintf(stderr, "erro ao baixar meta (ret=%d)\n", ret);
    return 1;
  }

  /* Verificar se arquivos existem */
  if (access(archive_path, F_OK) != 0) {
    fprintf(stderr, "erro: arquivo de commit não baixado\n");
    return 1;
  }

  if (access(meta_path, F_OK) != 0) {
    fprintf(stderr, "erro: arquivo meta não baixado\n");
    return 1;
  }

  /* Verificar integridade do arquivo baixado */
  char expected_checksum[65] = "";
  FILE *meta_f = fopen(meta_path, "r");
  if (meta_f) {
    char line[256];
    while (fgets(line, sizeof(line), meta_f)) {
      if (strncmp(line, "checksum: ", 10) == 0) {
        sscanf(line + 10, "%64s", expected_checksum);
        break;
      }
    }
    fclose(meta_f);
  }

  if (expected_checksum[0] != '\0') {
    /* Calcular checksum do arquivo baixado */
    char checksum_cmd[4096];
    char checksum_output[128];
    FILE *checksum_fp;

    if (snprintf(checksum_cmd, sizeof(checksum_cmd), "sha256sum \"%s\" 2>/dev/null",
                 archive_path) >= (int)sizeof(checksum_cmd)) {
      fprintf(stderr, "comando de checksum muito longo\n");
      return 1;
    }

    checksum_fp = popen(checksum_cmd, "r");
    if (checksum_fp) {
      if (fgets(checksum_output, sizeof(checksum_output), checksum_fp)) {
        char calculated_checksum[65];
        if (sscanf(checksum_output, "%64s", calculated_checksum) == 1) {
          if (strcmp(calculated_checksum, expected_checksum) != 0) {
            fprintf(stderr, "ERRO DE INTEGRIDADE: Checksum do arquivo baixado não corresponde!\n");
            fprintf(stderr, "Esperado: %s\n", expected_checksum);
            fprintf(stderr, "Calculado: %s\n", calculated_checksum);
            fprintf(stderr, "O arquivo pode ter sido corrompido durante o download.\n");
            return 1;
          }
        }
      }
      pclose(checksum_fp);
    }

    printf("Integridade verificada: checksum OK\n");
  } else {
    fprintf(stderr, "aviso: arquivo meta não contém checksum (versão antiga?)\n");
  }

  /* Extrair o tar.gz no diretório atual */
  if (snprintf(cmd, sizeof(cmd), "tar -xzf \"%s\" -C \"%s\"", archive_path, cwd) >=
      (int)sizeof(cmd)) {
    fprintf(stderr, "comando muito longo\n");
    return 1;
  }

  ret = system(cmd);
  if (ret != 0) {
    fprintf(stderr, "erro ao extrair commit (ret=%d)\n", ret);
    return 1;
  }

  /* Criar HEAD apontando para o commit baixado */
  char head_file[MAX_PATH];
  snprintf(head_file, sizeof(head_file), "%s/HEAD", commits_dir);
  fp = fopen(head_file, "w");
  if (fp) {
    fprintf(fp, "%s\n", last_commit_id);
    fclose(fp);
  } else {
    perror("fopen HEAD");
    return 1;
  }

  printf("Projeto clonado com sucesso!\n");
  return 0;
}