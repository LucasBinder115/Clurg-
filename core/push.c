#include <limits.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
<<<<<<< HEAD
=======
#include <time.h>
>>>>>>> 6f9723b (Adjusts.)
#include <unistd.h>

#define MAX_PATH PATH_MAX

<<<<<<< HEAD
int clurg_push(const char *arg1, const char *arg2) {
  char cwd[PATH_MAX];
  char commits_dir[PATH_MAX];
  char head_file[PATH_MAX];
  char cmd[4096];
  FILE *fp;
  char head_id[256];
  char archive_path[MAX_PATH];
  char meta_path[MAX_PATH];
  char project_name[256] = "default";
  char remote_url[1024];
  int ret;

  // Parse arguments: project_name and remote_url
  if (!arg1) {
    fprintf(stderr, "erro: argumentos insuficientes\n");
    fprintf(stderr, "Uso: clurg push <project_name> <remote_url>\n");
    fprintf(stderr, "Ou: clurg push <remote_url> (usa 'default' como projeto)\n");
    return 1;
  }

  if (arg2) {
    // arg1 is project_name, arg2 is remote_url
=======
static int prepare_snapshot(const char *project_name, char *snapshot_path,
                            size_t size) {
  char timestamp[64];
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  strftime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M%S", t);

  // Caminho temporário para o snapshot
  snprintf(snapshot_path, size, "/tmp/clurg_%s_%s.tar.gz", project_name,
           timestamp);

  char cmd[2048];
  /* tar --exclude=.clurg -czf <path> . */
  snprintf(cmd, sizeof(cmd), "tar --exclude=.clurg -czf \"%s\" . 2>/dev/null",
           snapshot_path);

  printf("📦 Gerando snapshot do projeto '%s'...\n", project_name);
  int ret = system(cmd);
  if (ret != 0) {
    fprintf(stderr, "erro: falha ao criar tar.gz (ret=%d)\n", ret);
    return 1;
  }
  return 0;
}

static int store_local_copy(const char *snapshot_path) {
  char local_dir[] = ".clurg/snapshots";
  char cmd[2048];

  printf("💾 Salvando backup local em %s/...\n", local_dir);

  // Criar diretório e copiar
  snprintf(cmd, sizeof(cmd), "mkdir -p %s && cp \"%s\" %s/", local_dir,
           snapshot_path, local_dir);

  int ret = system(cmd);
  if (ret != 0) {
    fprintf(stderr, "aviso: falha ao salvar cópia local (ret=%d)\n", ret);
    // Não retornamos erro fatal aqui, pois o remote ainda pode funcionar
  }
  return 0;
}

static int send_remote(const char *project_name, const char *snapshot_path,
                       const char *remote_url, const char *notes) {
  char cmd[4096];
  char response_body_path[] = "/tmp/clurg_push_res.body";
  char http_code_path[] = "/tmp/clurg_push_res.code";

  printf("🌐 Enviando para o Clurg Remote: %s...\n", remote_url);

  // Verificar se o arquivo existe e não está vazio
  FILE *check_fp = fopen(snapshot_path, "rb");
  if (!check_fp) {
    fprintf(stderr, "erro: snapshot não encontrado em %s\n", snapshot_path);
    return 1;
  }
  fseek(check_fp, 0, SEEK_END);
  long size = ftell(check_fp);
  fclose(check_fp);

  if (size <= 0) {
    fprintf(stderr, "erro: snapshot gerado está vazio\n");
    return 1;
  }

  /* curl -s -o body -w "%{http_code}" -X POST ... > code */
  if (snprintf(cmd, sizeof(cmd),
               "curl -s -o \"%s\" -w \"%%{http_code}\" -X POST -F \"file=@%s\" -F \"project=%s\" -F \"notes=%s\" \"%s\" > \"%s\"",
               response_body_path, snapshot_path, project_name, notes ? notes : "", remote_url, http_code_path) >= (int)sizeof(cmd)) {
    fprintf(stderr, "erro: comando curl muito longo\n");
    return 1;
  }

  int ret = system(cmd);
  if (ret != 0) {
    fprintf(stderr, "erro: falha ao executar curl (ret=%d)\n", ret);
    return 1;
  }

  // Ler código HTTP
  FILE *code_fp = fopen(http_code_path, "r");
  int http_code = 0;
  if (code_fp) {
    fscanf(code_fp, "%d", &http_code);
    fclose(code_fp);
  }
  remove(http_code_path);

  if (http_code < 200 || http_code >= 300) {
    fprintf(stderr, "❌ Erro no servidor remoto (HTTP %d)\n", http_code);
    
    // Tentar mostrar o corpo da resposta (mensagem de erro do servidor)
    FILE *body_fp = fopen(response_body_path, "r");
    if (body_fp) {
      char buffer[1024];
      fprintf(stderr, "Mensagem do servidor: ");
      while (fgets(buffer, sizeof(buffer), body_fp)) {
        fprintf(stderr, "%s", buffer);
      }
      fprintf(stderr, "\n");
      fclose(body_fp);
    }
    remove(response_body_path);
    return 1;
  }

  remove(response_body_path);
  return 0;
}

int clurg_push(const char *arg1, const char *arg2, const char *arg3) {
  char project_name[256] = "default";
  char remote_url[1024];
  const char *notes = arg3;
  char snapshot_path[MAX_PATH];

  // Parse arguments
  if (!arg1) {
    fprintf(stderr, "erro: argumentos insuficientes\n");
    fprintf(stderr, "Uso: clurg push <project_name> <remote_url> [notes]\n");
    fprintf(stderr, "Ou: clurg push <remote_url> [notes] (usa 'default' como projeto)\n");
    return 1;
  }

  if (arg2 && (strncmp(arg2, "http", 4) == 0)) {
    // arg1: project, arg2: url, arg3: notes
>>>>>>> 6f9723b (Adjusts.)
    strncpy(project_name, arg1, sizeof(project_name) - 1);
    project_name[sizeof(project_name) - 1] = '\0';
    strncpy(remote_url, arg2, sizeof(remote_url) - 1);
    remote_url[sizeof(remote_url) - 1] = '\0';
  } else {
<<<<<<< HEAD
    // arg1 is remote_url, use default project
    strncpy(remote_url, arg1, sizeof(remote_url) - 1);
    remote_url[sizeof(remote_url) - 1] = '\0';
  }

  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("getcwd");
    return 1;
  }

  /* Caminhos */
  snprintf(commits_dir, sizeof(commits_dir), "%s/.clurg/commits", cwd);
  snprintf(head_file, sizeof(head_file), "%s/HEAD", commits_dir);

  /* Verificar se HEAD existe */
  if (access(head_file, F_OK) != 0) {
    fprintf(stderr, "erro: nenhum commit encontrado. Faça um commit primeiro.\n");
    return 1;
  }

  /* Ler HEAD */
  fp = fopen(head_file, "r");
  if (!fp) {
    perror("fopen HEAD");
    return 1;
  }
  if (!fgets(head_id, sizeof(head_id), fp)) {
    fprintf(stderr, "erro ao ler HEAD\n");
    fclose(fp);
    return 1;
  }
  fclose(fp);
  head_id[strcspn(head_id, "\n")] = 0; /* remover newline */

  /* Caminhos do commit */
  snprintf(archive_path, sizeof(archive_path), "%s/%s.tar.gz", commits_dir, head_id);
  snprintf(meta_path, sizeof(meta_path), "%s/%s.meta", commits_dir, head_id);

  /* Verificar se arquivos existem */
  if (access(archive_path, F_OK) != 0) {
    fprintf(stderr, "erro: arquivo de commit não encontrado: %s\n", archive_path);
    return 1;
  }

  printf("Enviando commit %s do projeto '%s' para %s...\n", head_id, project_name, remote_url);

  /* Usar curl para enviar apenas project e commit_id via POST */
  /* curl -X POST -d "project=project_name&commit_id=head_id" remote_url */
  if (snprintf(cmd, sizeof(cmd), "curl -s -X POST -d \"project=%s&commit_id=%s\" \"%s\"",
               project_name, head_id, remote_url) >= (int)sizeof(cmd)) {
    fprintf(stderr, "comando muito longo\n");
    return 1;
  }

  ret = system(cmd);
  if (ret != 0) {
    fprintf(stderr, "erro ao enviar commit (ret=%d). Verifique a URL e conexão.\n", ret);
    return 1;
  }

  printf("Commit enviado com sucesso!\n");
  return 0;
=======
    // arg1: url, arg2: notes
    strncpy(remote_url, arg1, sizeof(remote_url) - 1);
    remote_url[sizeof(remote_url) - 1] = '\0';
    notes = arg2;
  }

  // 1. Prepare Snapshot
  if (prepare_snapshot(project_name, snapshot_path, sizeof(snapshot_path)) != 0) {
    return 1;
  }

  // 2. Store Local Copy
  store_local_copy(snapshot_path);

  // 3. Send Remote
  int ret = send_remote(project_name, snapshot_path, remote_url, notes);

  // Cleanup temp file
  remove(snapshot_path);

  if (ret == 0) {
    printf("✅ Push executado com sucesso!\n");
  } else {
    printf("⚠️  Push concluído com erros no remoto, mas o snapshot local foi salvo.\n");
  }

  return ret;
>>>>>>> 6f9723b (Adjusts.)
}