#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <linux/limits.h>

#define MAX_PATH PATH_MAX

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
        strncpy(project_name, arg1, sizeof(project_name) - 1);
        project_name[sizeof(project_name) - 1] = '\0';
        strncpy(remote_url, arg2, sizeof(remote_url) - 1);
        remote_url[sizeof(remote_url) - 1] = '\0';
    } else {
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
    if (snprintf(cmd, sizeof(cmd),
                 "curl -s -X POST -d \"project=%s&commit_id=%s\" \"%s\"",
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
}