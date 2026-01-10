#define _GNU_SOURCE
<<<<<<< HEAD
=======
#include <jansson.h>
>>>>>>> 6f9723b (Adjusts.)
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
<<<<<<< HEAD
  char last_commit_id[256] = "";
=======
  char response_path[] = "/tmp/clurg_snapshots.json";
  char last_commit_id[256] = "";
  char expected_hash[256] = "";
>>>>>>> 6f9723b (Adjusts.)
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

<<<<<<< HEAD
  /* Primeiro, obter o ID do último commit */
  /* curl -s <remote_url>/api/last-commit/<project_name> */
  if (snprintf(cmd, sizeof(cmd), "curl -s \"%s/api/last-commit/%s\"", remote_url, project_name) >=
=======
  /* Obter lista de snapshots */
  if (snprintf(cmd, sizeof(cmd), "curl -s -o \"%s\" \"%s/snapshots\"", response_path, remote_url) >=
>>>>>>> 6f9723b (Adjusts.)
      (int)sizeof(cmd)) {
    fprintf(stderr, "comando muito longo\n");
    return 1;
  }

<<<<<<< HEAD
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
=======
  ret = system(cmd);
  if (ret != 0) {
    fprintf(stderr, "erro ao consultar snapshots (ret=%d)\n", ret);
    return 1;
  }

  /* Parsear JSON com jansson */
  json_error_t error;
  json_t *root = json_load_file(response_path, 0, &error);
  remove(response_path);

  if (!root) {
    fprintf(stderr, "erro ao parsear JSON: %s\n", error.text);
    return 1;
  }

  json_t *snapshots = json_object_get(root, "snapshots");
  if (!json_is_array(snapshots)) {
    fprintf(stderr, "erro: campo 'snapshots' não encontrado ou não é array\n");
    json_decref(root);
    return 1;
  }

  /* Encontrar o último snapshot para o projeto */
  size_t index;
  json_t *value;
  json_array_foreach(snapshots, index, value) {
    const char *p_name = json_string_value(json_object_get(value, "project"));
    if (p_name && strcmp(p_name, project_name) == 0) {
      const char *id = json_string_value(json_object_get(value, "id"));
      const char *hash = json_object_get(value, "hash") ? json_string_value(json_object_get(value, "hash")) : "";
      if (id) {
        strncpy(last_commit_id, id, sizeof(last_commit_id) - 1);
        strncpy(expected_hash, hash, sizeof(expected_hash) - 1);
        break;
      }
    }
  }

  json_decref(root);

  if (strlen(last_commit_id) == 0) {
    fprintf(stderr, "erro: nenhum snapshot encontrado para o projeto '%s'\n", project_name);
    return 1;
  }

  printf("Último snapshot: %s\n", last_commit_id);
>>>>>>> 6f9723b (Adjusts.)

  /* Criar diretório .clurg/commits se não existir */
  char commits_dir[MAX_PATH];
  snprintf(commits_dir, sizeof(commits_dir), "%s/.clurg/commits", cwd);
  char mkdir_cmd[1024];
  snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"", commits_dir);
  system(mkdir_cmd);

<<<<<<< HEAD
  /* Baixar o commit */
  /* curl -s -o .clurg/commits/<id>.tar.gz <remote_url>/download/<project>/<id> */
  char archive_path[MAX_PATH];
  char meta_path[MAX_PATH];
  snprintf(archive_path, sizeof(archive_path), "%s/%s.tar.gz", commits_dir, last_commit_id);
  snprintf(meta_path, sizeof(meta_path), "%s/%s.meta", commits_dir, last_commit_id);

  if (snprintf(cmd, sizeof(cmd), "curl -s -o \"%s\" \"%s/download/%s/%s\"", archive_path,
               remote_url, project_name, last_commit_id) >= (int)sizeof(cmd)) {
=======
  /* Baixar o snapshot */
  /* curl -s -o .clurg/commits/<id>.tar.gz <remote_url>/snapshot/<id> */
  char archive_path[MAX_PATH];
  snprintf(archive_path, sizeof(archive_path), "%s/%s.tar.gz", commits_dir, last_commit_id);

  if (snprintf(cmd, sizeof(cmd), "curl -s -o \"%s\" \"%s/snapshot/%s\"", archive_path,
               remote_url, last_commit_id) >= (int)sizeof(cmd)) {
>>>>>>> 6f9723b (Adjusts.)
    fprintf(stderr, "comando muito longo\n");
    return 1;
  }

  ret = system(cmd);
  if (ret != 0) {
<<<<<<< HEAD
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
=======
    fprintf(stderr, "erro ao baixar snapshot (ret=%d)\n", ret);
    return 1;
  }

  /* Verificar se arquivo existe */
  if (access(archive_path, F_OK) != 0) {
    fprintf(stderr, "erro: arquivo de snapshot não baixado\n");
    return 1;
  }

  /* Verificar integridade (MD5 como no servidor Python) */
  if (strlen(expected_hash) > 0) {
    char hash_cmd[4096];
    char hash_output[128];
    FILE *hash_fp;

    snprintf(hash_cmd, sizeof(hash_cmd), "md5sum \"%s\" | cut -d' ' -f1", archive_path);
    hash_fp = popen(hash_cmd, "r");
    if (hash_fp) {
      if (fgets(hash_output, sizeof(hash_output), hash_fp)) {
        hash_output[strcspn(hash_output, "\n")] = 0;
        if (strcmp(hash_output, expected_hash) != 0) {
          fprintf(stderr, "ERRO DE INTEGRIDADE: Hash MD5 não corresponde!\n");
          fprintf(stderr, "Esperado: %s\n", expected_hash);
          fprintf(stderr, "Calculado: %s\n", hash_output);
          return 1;
        }
      }
      pclose(hash_fp);
    }
    printf("Integridade verificada: MD5 OK\n");
  }

  /* Extrair o tar.gz no diretório atual */
  if (snprintf(cmd, sizeof(cmd), "tar --exclude=.clurg -xzf \"%s\" -C \"%s\"", archive_path, cwd) >=
>>>>>>> 6f9723b (Adjusts.)
      (int)sizeof(cmd)) {
    fprintf(stderr, "comando muito longo\n");
    return 1;
  }

  ret = system(cmd);
  if (ret != 0) {
<<<<<<< HEAD
    fprintf(stderr, "erro ao extrair commit (ret=%d)\n", ret);
    return 1;
  }

  /* Criar HEAD apontando para o commit baixado */
=======
    fprintf(stderr, "erro ao extrair snapshot (ret=%d)\n", ret);
    return 1;
  }

  /* Criar HEAD apontando para o snapshot baixado */
>>>>>>> 6f9723b (Adjusts.)
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