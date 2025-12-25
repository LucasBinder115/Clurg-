#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>
#include <limits.h>
#include <ctype.h>

#include "api.h"

#define PORT 8080
#define BUFFER_SIZE 32768
#define MAX_PATH 1024
#define MAX_TOKEN_LENGTH 256

/* Estrutura para armazenar informações de autenticação */
typedef struct {
    char token[MAX_TOKEN_LENGTH];
    char permissions[16];
    char projects[512];
} auth_info_t;

/* Cache de tokens carregados */
static auth_info_t *auth_cache = NULL;
static int auth_cache_count = 0;
static time_t auth_cache_time = 0;

/* Função para verificar se um projeto é público */
static int is_project_public(const char *clurg_root, const char *project_name) {
    char config_path[MAX_PATH];
    FILE *fp;
    char line[1024];
    
    snprintf(config_path, sizeof(config_path), "%s/.clurg/security.conf", clurg_root);
    
    fp = fopen(config_path, "r");
    if (!fp) {
        return 0; /* Se não há config, assume privado */
    }
    
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "PUBLIC_PROJECTS=", 16) == 0) {
            char *projects_str = line + 16;
            char *start = projects_str;
            
            /* Remover aspas */
            if (*start == '"') {
                start++;
                char *end = strchr(start, '"');
                if (end) *end = '\0';
            }
            
            /* Verificar se projeto está na lista */
            if (strlen(start) > 0) {
                char *temp = strdup(start);
                char *proj = strtok(temp, ",");
                while (proj) {
                    /* Trim whitespace */
                    while (*proj == ' ') proj++;
                    char *end = proj + strlen(proj) - 1;
                    while (end > proj && *end == ' ') *end-- = '\0';
                    
                    if (strcmp(proj, project_name) == 0) {
                        free(temp);
                        fclose(fp);
                        return 1; /* Projeto é público */
                    }
                    
                    proj = strtok(NULL, ",");
                }
                free(temp);
            }
            break;
        }
    }
    
    fclose(fp);
    return 0; /* Projeto é privado */
}

/* Função para carregar configuração de segurança */
static int load_security_config(const char *clurg_root, auth_info_t **tokens, int *count) {
    char config_path[MAX_PATH];
    FILE *fp;
    char line[1024];
    int capacity = 0;
    
    snprintf(config_path, sizeof(config_path), "%s/.clurg/security.conf", clurg_root);
    
    fp = fopen(config_path, "r");
    if (!fp) {
        /* Arquivo não existe, usar configuração padrão (sem autenticação) */
        *tokens = NULL;
        *count = 0;
        return 0;
    }
    
    /* Contar tokens primeiro */
    *count = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "TOKENS=", 7) == 0) {
            char *tokens_str = line + 7;
            char *token_start = tokens_str;
            char *token_end;
            
            /* Remover aspas se presentes */
            if (*token_start == '"') {
                token_start++;
                token_end = strchr(token_start, '"');
                if (token_end) *token_end = '\0';
            }
            
            /* Contar tokens separados por vírgula */
            if (strlen(token_start) > 0) {
                char *temp = strdup(token_start);
                char *tok = strtok(temp, ",");
                while (tok) {
                    (*count)++;
                    tok = strtok(NULL, ",");
                }
                free(temp);
            }
            break;
        }
    }
    
    if (*count == 0) {
        fclose(fp);
        *tokens = NULL;
        return 0;
    }
    
    /* Alocar memória */
    *tokens = malloc(*count * sizeof(auth_info_t));
    if (!*tokens) {
        fclose(fp);
        return -1;
    }
    
    /* Ler tokens */
    rewind(fp);
    int idx = 0;
    while (fgets(line, sizeof(line), fp) && idx < *count) {
        if (strncmp(line, "TOKENS=", 7) == 0) {
            char *tokens_str = line + 7;
            char *token_start = tokens_str;
            
            /* Remover aspas */
            if (*token_start == '"') {
                token_start++;
                char *quote_end = strchr(token_start, '"');
                if (quote_end) *quote_end = '\0';
            }
            
            /* Parse tokens */
            char *temp = strdup(token_start);
            char *tok = strtok(temp, ",");
            while (tok && idx < *count) {
                /* Trim whitespace */
                while (*tok == ' ') tok++;
                char *end = tok + strlen(tok) - 1;
                while (end > tok && *end == ' ') *end-- = '\0';
                
                /* Parse token: formato token:permissions:projects */
                char *colon1 = strchr(tok, ':');
                char *colon2 = colon1 ? strchr(colon1 + 1, ':') : NULL;
                
                if (colon1 && colon2) {
                    *colon1 = '\0';
                    *colon2 = '\0';
                    
                    strncpy((*tokens)[idx].token, tok, sizeof((*tokens)[idx].token) - 1);
                    strncpy((*tokens)[idx].permissions, colon1 + 1, sizeof((*tokens)[idx].permissions) - 1);
                    strncpy((*tokens)[idx].projects, colon2 + 1, sizeof((*tokens)[idx].projects) - 1);
                    
                    idx++;
                }
                
                tok = strtok(NULL, ",");
            }
            free(temp);
            break;
        }
    }
    
    fclose(fp);
    *count = idx; /* Atualizar contagem real */
    return 0;
}

/* Função para verificar autenticação */
static int authenticate_request(const char *request, const char *method, const char *path, 
                               const char *clurg_root, char **error_msg) {
    // Check if this is a public endpoint
    if (strcmp(path, "/") == 0 || 
        strncmp(path, "/static/", 8) == 0 ||
        strncmp(path, "/api/v1/system/status", 22) == 0) {
        return 1; // Public endpoints don't require auth
    }
    
    // Parse Authorization header
    const char *auth_header = strstr(request, "Authorization: Bearer ");
    if (!auth_header) {
        *error_msg = "Missing Authorization header";
        return 0;
    }
    
    char token[256];
    if (sscanf(auth_header, "Authorization: Bearer %255s", token) != 1) {
        *error_msg = "Invalid Authorization header format";
        return 0;
    }
    
    // Load security config and validate token
    auth_info_t *tokens = NULL;
    int token_count = 0;
    if (load_security_config(clurg_root, &tokens, &token_count) != 0) {
        *error_msg = "Failed to load security configuration";
        return 0;
    }
    
    // Check token
    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i].token, token) == 0) {
            // Check permissions based on method
            int requires_write = (strcmp(method, "POST") == 0 || 
                                strcmp(method, "PUT") == 0 || 
                                strcmp(method, "DELETE") == 0);
            
            if (requires_write && !strchr(tokens[i].permissions, 'w')) {
                free(tokens);
                *error_msg = "Insufficient permissions for this operation";
                return 0;
            }
            
            // Check project-specific access if path contains project
            if (strncmp(path, "/api/v1/projects/", 17) == 0) {
                char project_name[256];
                sscanf(path + 17, "%255[^/]", project_name);
                
                // Check if project is public
                if (is_project_public(clurg_root, project_name)) {
                    free(tokens);
                    return 1; // Public project
                }
                
                // Check if token has access to this project
                if (strcmp(tokens[i].projects, "*") != 0 && 
                    strstr(tokens[i].projects, project_name) == NULL) {
                    free(tokens);
                    *error_msg = "Access denied to this project";
                    return 0;
                }
            }
            
            free(tokens);
            return 1; // Valid token with appropriate permissions
        }
    }
    
    free(tokens);
    *error_msg = "Invalid token";
    return 0;
}

static void send_response(int client_fd, const char *status, const char *content_type, const char *body) {
    char header[2048];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, content_type, strlen(body));
    
    write(client_fd, header, header_len);
    if (strlen(body) > 0) {
        write(client_fd, body, strlen(body));
    }
}

static int verify_file_integrity(const char *file_path, const char *expected_checksum) {
    char cmd[4096];
    char output[128];
    FILE *fp;
    
    /* Usar sha256sum para calcular o checksum */
    snprintf(cmd, sizeof(cmd), "sha256sum \"%s\" 2>/dev/null", file_path);
    
    fp = popen(cmd, "r");
    if (!fp) {
        return 0; /* Não conseguiu executar */
    }
    
    if (fgets(output, sizeof(output), fp) == NULL) {
        pclose(fp);
        return 0; /* Não conseguiu ler output */
    }
    
    pclose(fp);
    
    /* Extrair o checksum do output (primeiros 64 caracteres) */
    char calculated_checksum[65];
    if (sscanf(output, "%64s", calculated_checksum) != 1) {
        return 0; /* Formato inválido */
    }
    
    /* Comparar checksums */
    return strcmp(calculated_checksum, expected_checksum) == 0;
}

static void send_file_response(int client_fd, const char *status, const char *content_type, const char *file_path) {
    FILE *f = fopen(file_path, "r");
    if (!f) {
        send_response(client_fd, "404 Not Found", "text/plain", "File not found");
        return;
    }
    
    /* Obter tamanho do arquivo */
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    /* Enviar cabeçalho */
    char header[1024];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, content_type, file_size);
    
    write(client_fd, header, header_len);
    
    /* Enviar conteúdo */
    char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        write(client_fd, buffer, n);
    }
    
    fclose(f);
}

typedef struct {
    char name[256];
    time_t mtime;
    int has_fail;
    char status_str[16];
} log_entry_t;

static int compare_logs(const void *a, const void *b) {
    const log_entry_t *log_a = (const log_entry_t *)a;
    const log_entry_t *log_b = (const log_entry_t *)b;
    /* Ordenar por data (mais recente primeiro) */
    if (log_a->mtime > log_b->mtime) return -1;
    if (log_a->mtime < log_b->mtime) return 1;
    return 0;
}

static void log_access(const char *clurg_root, const char *method, const char *path, const char *status, const char *user_agent) {
    char log_path[MAX_PATH];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    snprintf(log_path, sizeof(log_path), "%s/.clurg/access.log", clurg_root);
    
    FILE *f = fopen(log_path, "a");
    if (f) {
        fprintf(f, "[%s] %s %s %s \"%s\"\n", timestamp, method, path, status, user_agent ? user_agent : "-");
        fclose(f);
    }
}

static void get_log_status(const char *log_file_path, char *status_str, size_t status_size, int *has_fail) {
    FILE *f = fopen(log_file_path, "r");
    if (!f) {
        strncpy(status_str, "?", status_size - 1);
        status_str[status_size - 1] = '\0';
        *has_fail = 0;
        return;
    }
    
    *has_fail = 0;
    int has_ok = 0;
    char line[1024];
    
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, ": OK")) {
            has_ok = 1;
        } else if (strstr(line, ": FAIL")) {
            *has_fail = 1;
            break; /* Basta um FAIL para marcar como falhou */
        }
    }
    
    fclose(f);
    
    if (*has_fail) {
        strncpy(status_str, "FAIL", status_size - 1);
        status_str[status_size - 1] = '\0';
    } else if (has_ok) {
        strncpy(status_str, "OK", status_size - 1);
        status_str[status_size - 1] = '\0';
    } else {
        strncpy(status_str, "?", status_size - 1);
        status_str[status_size - 1] = '\0';
    }
}

static void list_log_files(char *buffer, size_t buffer_size, const char *logs_dir) {
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    char file_path[MAX_PATH];
    log_entry_t logs[256];
    int log_count = 0;
    
    buffer[0] = '\0';
    
    dir = opendir(logs_dir);
    if (!dir) {
        snprintf(buffer, buffer_size, "<p>Nenhum log encontrado</p>");
        return;
    }
    
    /* Coletar todos os logs */
    while ((entry = readdir(dir)) != NULL && log_count < 256) {
        if (entry->d_name[0] == '.') continue;
        
        snprintf(file_path, sizeof(file_path), "%s/%s", logs_dir, entry->d_name);
        if (stat(file_path, &st) == 0 && S_ISREG(st.st_mode)) {
            strncpy(logs[log_count].name, entry->d_name, sizeof(logs[log_count].name) - 1);
            logs[log_count].name[sizeof(logs[log_count].name) - 1] = '\0';
            logs[log_count].mtime = st.st_mtime;
            get_log_status(file_path, logs[log_count].status_str, sizeof(logs[log_count].status_str), 
                          &logs[log_count].has_fail);
            log_count++;
        }
    }
    closedir(dir);
    
    /* Ordenar por data (mais recente primeiro) */
    if (log_count > 0) {
        qsort(logs, log_count, sizeof(log_entry_t), compare_logs);
    }
    
    strcat(buffer, "<h2>Logs de CI</h2>");
    
    /* Botão de auto-refresh */
    strcat(buffer, "<p><a href='/?refresh=5' style='background: #0066cc; color: white; padding: 5px 10px; text-decoration: none; border-radius: 3px;'>Auto-refresh (5s)</a> ");
    strcat(buffer, "<a href='/' style='background: #666; color: white; padding: 5px 10px; text-decoration: none; border-radius: 3px;'>Sem refresh</a></p>");
    
    strcat(buffer, "<ul style='list-style: none; padding: 0;'>");
    
    /* Listar logs ordenados */
    for (int i = 0; i < log_count; i++) {
        char time_str[64];
        struct tm *tm_info = localtime(&logs[i].mtime);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        
        const char *status_color = logs[i].has_fail ? "#dc3545" : "#28a745";
        const char *status_bg = logs[i].has_fail ? "#ffe6e6" : "#e6ffe6";
        
        char item[1024];
        snprintf(item, sizeof(item),
            "<li style='background: %s; padding: 12px; margin: 8px 0; border-radius: 4px; border-left: 4px solid %s;'>"
            "<a href=\"/logs/%s\" style='color: #0066cc; text-decoration: none; font-weight: bold;'>%s</a> - "
            "<span style='color: #666;'>%s</span> - "
            "<span style='background: %s; color: %s; padding: 2px 8px; border-radius: 3px; font-size: 0.9em; font-weight: bold;'>%s</span>"
            "</li>",
            status_bg, status_color,
            logs[i].name, logs[i].name, time_str,
            status_color, "white", logs[i].status_str);
        
        size_t remaining = buffer_size - strlen(buffer) - 1;
        if (remaining > strlen(item)) {
            strncat(buffer, item, remaining);
        }
    }
    
    strcat(buffer, "</ul>");
}

static void read_log_file(char *buffer, size_t buffer_size, const char *log_file_path, off_t max_size) {
    FILE *f = fopen(log_file_path, "r");
    if (!f) {
        snprintf(buffer, buffer_size, "<p>Erro ao ler arquivo de log</p>");
        return;
    }
    
    size_t pos = 0;
    pos += snprintf(buffer + pos, buffer_size - pos, "<pre>");
    
    /* Limitar leitura ao tamanho máximo especificado */
    size_t bytes_read = 0;
    size_t max_bytes = (size_t)max_size;
    if (max_bytes > buffer_size - 1000) {
        max_bytes = buffer_size - 1000; /* Reservar espaço para tags HTML */
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), f) && pos < buffer_size - 100 && bytes_read < max_bytes) {
        size_t line_len = strlen(line);
        if (bytes_read + line_len > max_bytes) {
            line_len = max_bytes - bytes_read;
            line[line_len] = '\0';
        }
        
        /* Escape HTML */
        for (char *p = line; *p && pos < buffer_size - 10 && bytes_read < max_bytes; p++) {
            if (*p == '<') {
                pos += snprintf(buffer + pos, buffer_size - pos, "&lt;");
            } else if (*p == '>') {
                pos += snprintf(buffer + pos, buffer_size - pos, "&gt;");
            } else if (*p == '&') {
                pos += snprintf(buffer + pos, buffer_size - pos, "&amp;");
            } else {
                buffer[pos++] = *p;
            }
            bytes_read++;
        }
        
        if (bytes_read >= max_bytes) {
            pos += snprintf(buffer + pos, buffer_size - pos, "\n... (arquivo truncado)");
            break;
        }
    }
    
    pos += snprintf(buffer + pos, buffer_size - pos, "</pre>");
    buffer[pos] = '\0';
    
    fclose(f);
}

static int copy_file(const char *src_path, const char *dst_path) {
    FILE *src = fopen(src_path, "rb");
    FILE *dst = fopen(dst_path, "wb");
    if (!src || !dst) {
        if (src) fclose(src);
        if (dst) fclose(dst);
        return -1;
    }
    
    char buffer[4096];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, n, dst) != n) {
            fclose(src);
            fclose(dst);
            return -1;
        }
    }
    
    fclose(src);
    fclose(dst);
    return 0;
}

static void parse_multipart_form_data(const char *body, const char *boundary, 
                                      char *project_name, size_t pn_size,
                                      char *message, size_t msg_size,
                                      char ***files, int *file_count,
                                      char ***file_contents, size_t **file_sizes) {
    /* Implementação completa para parsing de multipart/form-data */
    char boundary_start[512];
    char boundary_end[512];
    snprintf(boundary_start, sizeof(boundary_start), "--%s", boundary);
    snprintf(boundary_end, sizeof(boundary_end), "--%s--", boundary);
    
    const char *pos = body;
    *file_count = 0;
    *files = NULL;
    *file_contents = NULL;
    *file_sizes = NULL;
    
    while ((pos = strstr(pos, boundary_start)) != NULL) {
        pos += strlen(boundary_start);
        
        /* Pular \r\n */
        if (*pos == '\r' && *(pos+1) == '\n') pos += 2;
        
        /* Verificar se é o boundary final */
        if (strncmp(pos, "--", 2) == 0) break;
        
        /* Procurar Content-Disposition */
        const char *content_disp = strstr(pos, "Content-Disposition:");
        if (!content_disp) continue;
        
        const char *name_start = strstr(content_disp, "name=\"");
        if (!name_start) continue;
        name_start += 6;
        
        const char *name_end = strchr(name_start, '"');
        if (!name_end) continue;
        
        char field_name[256];
        size_t name_len = name_end - name_start;
        if (name_len >= sizeof(field_name)) continue;
        strncpy(field_name, name_start, name_len);
        field_name[name_len] = '\0';
        
        /* Procurar o início dos dados */
        const char *data_start = strstr(name_end, "\r\n\r\n");
        if (!data_start) continue;
        data_start += 4;
        
        /* Procurar o próximo boundary */
        const char *data_end = strstr(data_start, boundary_start);
        if (!data_end) data_end = strstr(data_start, boundary_end);
        if (!data_end) continue;
        
        /* Retroceder para encontrar \r\n antes do boundary */
        while (data_end > data_start && !(*(data_end-1) == '\n' && *(data_end-2) == '\r')) data_end--;
        if (data_end <= data_start) continue;
        data_end -= 2; /* Remover \r\n */
        
        size_t data_len = data_end - data_start;
        
        if (strcmp(field_name, "project_name") == 0) {
            if (data_len < pn_size - 1) {
                strncpy(project_name, data_start, data_len);
                project_name[data_len] = '\0';
                /* Remover espaços em branco */
                char *end = project_name + strlen(project_name) - 1;
                while (end > project_name && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) *end-- = '\0';
                char *start = project_name;
                while (*start && (*start == ' ' || *start == '\t')) start++;
                memmove(project_name, start, strlen(start) + 1);
            }
        }
        else if (strcmp(field_name, "message") == 0) {
            if (data_len < msg_size - 1) {
                strncpy(message, data_start, data_len);
                message[data_len] = '\0';
                /* Remover espaços em branco */
                char *end = message + strlen(message) - 1;
                while (end > message && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) *end-- = '\0';
                char *start = message;
                while (*start && (*start == ' ' || *start == '\t')) start++;
                memmove(message, start, strlen(start) + 1);
            }
        }
        else if (strcmp(field_name, "files") == 0) {
            /* Extrair nome do arquivo */
            const char *filename_start = strstr(content_disp, "filename=\"");
            if (filename_start) {
                filename_start += 10;
                const char *filename_end = strchr(filename_start, '"');
                if (filename_end) {
                    char filename[256];
                    size_t filename_len = filename_end - filename_start;
                    if (filename_len < sizeof(filename) - 1) {
                        strncpy(filename, filename_start, filename_len);
                        filename[filename_len] = '\0';
                        
                        /* Adicionar arquivo à lista */
                        *file_count += 1;
                        *files = realloc(*files, *file_count * sizeof(char*));
                        *file_contents = realloc(*file_contents, *file_count * sizeof(char*));
                        *file_sizes = realloc(*file_sizes, *file_count * sizeof(size_t));
                        
                        (*files)[*file_count - 1] = strdup(filename);
                        (*file_contents)[*file_count - 1] = malloc(data_len + 1);
                        memcpy((*file_contents)[*file_count - 1], data_start, data_len);
                        (*file_contents)[*file_count - 1][data_len] = '\0';
                        (*file_sizes)[*file_count - 1] = data_len;
                    }
                }
            }
        }
        
        pos = data_end;
    }
}

static int validate_upload(const char *project_name, char **files, int file_count, size_t *file_sizes, char **error_msg) {
    /* Validações básicas */
    if (strlen(project_name) == 0) {
        *error_msg = "Nome do projeto é obrigatório";
        return 0;
    }
    
    if (strlen(project_name) > 100) {
        *error_msg = "Nome do projeto muito longo (máximo 100 caracteres)";
        return 0;
    }
    
    /* Verificar caracteres válidos no nome do projeto */
    for (const char *p = project_name; *p; p++) {
        if (!isalnum(*p) && *p != '-' && *p != '_') {
            *error_msg = "Nome do projeto contém caracteres inválidos (apenas letras, números, - e _ são permitidos)";
            return 0;
        }
    }
    
    if (file_count == 0) {
        *error_msg = "Pelo menos um arquivo deve ser enviado";
        return 0;
    }
    
    if (file_count > 100) {
        *error_msg = "Muitos arquivos (máximo 100 arquivos)";
        return 0;
    }
    
    /* Verificar tamanho total */
    size_t total_size = 0;
    for (int i = 0; i < file_count; i++) {
        total_size += file_sizes[i];
        if (file_sizes[i] > 10 * 1024 * 1024) { /* 10MB por arquivo */
            *error_msg = "Arquivo muito grande (máximo 10MB por arquivo)";
            return 0;
        }
    }
    
    if (total_size > 50 * 1024 * 1024) { /* 50MB total */
        *error_msg = "Upload muito grande (máximo 50MB total)";
        return 0;
    }
    
    /* Verificar nomes de arquivos */
    for (int i = 0; i < file_count; i++) {
        if (strlen(files[i]) == 0 || strlen(files[i]) > 255) {
            *error_msg = "Nome de arquivo inválido";
            return 0;
        }
        
        /* Verificar caracteres perigosos */
        if (strstr(files[i], "..") || files[i][0] == '/' || strstr(files[i], "\\")) {
            *error_msg = "Nome de arquivo contém caracteres perigosos";
            return 0;
        }
    }
    
    return 1;
}

static int create_project_archive(const char *clurg_root, const char *project_name, const char *commit_id,
                                 char **files, char **file_contents, size_t *file_sizes, int file_count) {
    char temp_dir[MAX_PATH];
    char tar_path[MAX_PATH];
    
    /* Criar diretório temporário */
    snprintf(temp_dir, sizeof(temp_dir), "%s/.clurg/temp/%s_%s", clurg_root, project_name, commit_id);
    char mkdir_cmd[2048];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"", temp_dir);
    if (system(mkdir_cmd) != 0) return 0;
    
    /* Salvar arquivos no diretório temporário */
    for (int i = 0; i < file_count; i++) {
        char file_path[MAX_PATH];
        snprintf(file_path, sizeof(file_path), "%s/%s", temp_dir, files[i]);
        
        /* Criar subdiretórios se necessário */
        char *dir_end = strrchr(file_path, '/');
        if (dir_end) {
            *dir_end = '\0';
            char mkdir_sub[2048];
            snprintf(mkdir_sub, sizeof(mkdir_sub), "mkdir -p \"%s\"", file_path);
            system(mkdir_sub);
            *dir_end = '/';
        }
        
        FILE *f = fopen(file_path, "wb");
        if (f) {
            fwrite(file_contents[i], 1, file_sizes[i], f);
            fclose(f);
        } else {
            return 0;
        }
    }
    
    /* Criar tar.gz */
    char commit_dir[MAX_PATH];
    snprintf(commit_dir, sizeof(commit_dir), "%s/.clurg/projects/%s/commits", clurg_root, project_name);
    snprintf(tar_path, sizeof(tar_path), "%s/%s.tar.gz", commit_dir, commit_id);
    
    char tar_cmd[4096];
    snprintf(tar_cmd, sizeof(tar_cmd), "cd \"%s\" && tar -czf \"%s\" .", temp_dir, tar_path);
    int result = (system(tar_cmd) == 0);
    
    /* Limpar diretório temporário */
    char rm_cmd[2048];
    snprintf(rm_cmd, sizeof(rm_cmd), "rm -rf \"%s\"", temp_dir);
    system(rm_cmd);
    
    return result;
}

static void handle_request(int client_fd, const char *request, const char *clurg_root) {
    char method[16], path[512], user_agent[256] = "";
    char logs_dir[MAX_PATH];
    char *auth_error = NULL;
    
    // printf("DEBUG: handle_request started\n");
    
    /* Parsear requisição básica */
    if (sscanf(request, "%15s %511s", method, path) != 2) {
        // printf("DEBUG: Bad request parsing\n");
        send_response(client_fd, "400 Bad Request", "text/plain", "Bad Request");
        // // log_access(clurg_root, method, path, "400 Bad Request", user_agent);
        return;
    }
    
    // printf("DEBUG: Method=%s, Path=%s\n", method, path);
    
    /* Verificar autenticação */
    if (!authenticate_request(request, method, path, clurg_root, &auth_error)) {
        char error_response[512];
        snprintf(error_response, sizeof(error_response), 
            "HTTP/1.1 401 Unauthorized\r\n"
            "Content-Type: text/plain\r\n"
            "WWW-Authenticate: Bearer\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n%s",
            strlen(auth_error), auth_error);
        
        write(client_fd, error_response, strlen(error_response));
        // log_access(clurg_root, method, path, "401 Unauthorized", user_agent);
        return;
    }
    
    /* Extract User-Agent */
    char *ua_line = strstr(request, "User-Agent: ");
    if (ua_line) {
        char *ua_end = strstr(ua_line, "\r\n");
        if (ua_end) {
            size_t ua_len = ua_end - (ua_line + 12);
            if (ua_len < sizeof(user_agent)) {
                strncpy(user_agent, ua_line + 12, ua_len);
                user_agent[ua_len] = '\0';
            }
        }
    }
    
    snprintf(logs_dir, sizeof(logs_dir), "%s/.clurg/ci/logs", clurg_root);
    
    /* Rotas */
    if (strcmp(method, "GET") == 0 && strcmp(path, "/test") == 0) {
        send_response(client_fd, "200 OK", "text/plain", "OK");
        // log_access(clurg_root, method, path, "200 OK", user_agent);
    }
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/upload") == 0) {
        /* Página de upload de projeto */
        char html[16384];
        snprintf(html, sizeof(html),
            "<!DOCTYPE html><html><head><title>Upload Projeto - Clurg</title>"
            "<style>:root {  --bg-primary: #1a1a1a;  --bg-secondary: #2d2d2d;  --bg-card: #333333;  --text-primary: #ffffff;  --text-secondary: #cccccc;  --accent: #9b59b6;  --success: #27ae60;  --error: #e74c3c;  --border: #444444;  --shadow: rgba(0,0,0,0.3);}"
            "@media (prefers-color-scheme: light) {  :root {    --bg-primary: #f5f5f5;    --bg-secondary: #ffffff;    --bg-card: #ffffff;    --text-primary: #2c3e50;    --text-secondary: #7f8c8d;    --accent: #3498db;    --success: #27ae60;    --error: #e74c3c;    --border: #95a5a6;    --shadow: rgba(0,0,0,0.1);  }}"
            "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 20px; background: var(--bg-primary); color: var(--text-primary); }"
            "h1 { color: var(--text-primary); border-bottom: 3px solid var(--accent); padding-bottom: 10px; }"
            ".upload-form { background: var(--bg-card); padding: 30px; border-radius: 8px; box-shadow: 0 2px 4px var(--shadow); max-width: 600px; margin: 20px auto; }"
            ".form-group { margin: 20px 0; }"
            "label { display: block; margin-bottom: 8px; font-weight: bold; color: var(--text-primary); }"
            "input[type='text'], input[type='file'] { width: 100%%; padding: 12px; border: 1px solid var(--border); border-radius: 4px; background: var(--bg-secondary); color: var(--text-primary); box-sizing: border-box; }"
            "input[type='file'] { padding: 8px; }"
            ".btn { display: inline-block; padding: 12px 24px; text-decoration: none; border-radius: 5px; font-size: 16px; margin-right: 10px; border: none; cursor: pointer; }"
            ".btn-upload { background: var(--success); color: white; }"
            ".btn-upload:hover { background: #229954; }"
            ".btn-back { background: var(--border); color: white; }"
            ".btn-back:hover { background: #7f8c8d; }"
            ".info { background: var(--bg-secondary); padding: 15px; border-radius: 4px; margin: 20px 0; border-left: 4px solid var(--accent); }"
            "@media (max-width: 768px) {  body { padding: 10px; }  .upload-form { padding: 20px; margin: 10px; }  .btn { padding: 10px 20px; font-size: 14px; }}"
            "</style></head><body>"
            "<h1>📤 Upload de Projeto</h1>"
            "<div class='info'>"
            "<strong>Como usar:</strong><br>"
            "1. Digite o nome do projeto<br>"
            "2. Selecione todos os arquivos do seu projeto (use Ctrl+Click para múltiplos)<br>"
            "3. Clique em 'Fazer Upload'<br>"
            "4. O projeto será versionado automaticamente"
            "</div>"
            "<form class='upload-form' action='/upload' method='post' enctype='multipart/form-data'>"
            "<div class='form-group'>"
            "<label for='project_name'>Nome do Projeto:</label>"
            "<input type='text' id='project_name' name='project_name' required placeholder='Ex: meu-site-vendas'>"
            "</div>"
            "<div class='form-group'>"
            "<label for='files'>Arquivos do Projeto:</label>"
            "<input type='file' id='files' name='files' multiple required>"
            "<br><small>Selecione todos os arquivos do seu projeto (HTML, CSS, JS, imagens, etc.)</small>"
            "</div>"
            "<div class='form-group'>"
            "<label for='message'>Mensagem do Commit:</label>"
            "<input type='text' id='message' name='message' placeholder='Ex: Versão inicial do site de vendas'>"
            "</div>"
            "<button type='submit' class='btn btn-upload'>📤 Fazer Upload</button>"
            "<a href='/projects' class='btn btn-back'>← Voltar aos Projetos</a>"
            "</form>"
            "</body></html>");
        send_response(client_fd, "200 OK", "text/html; charset=utf-8", html);
        // log_access(clurg_root, method, path, "200 OK", user_agent);
    }
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/upload") == 0) {
        /* Processar upload de arquivos */
        char *content_type = strstr(request, "Content-Type: ");
        char boundary[256] = "";
        
        if (content_type) {
            char *multipart = strstr(content_type, "multipart/form-data; boundary=");
            if (multipart) {
                sscanf(multipart, "multipart/form-data; boundary=%255[^;\r\n]", boundary);
            }
        }
        
        if (strlen(boundary) == 0) {
            send_response(client_fd, "400 Bad Request", "text/plain", "Invalid multipart request");
            return;
        }
        
        char *body = strstr(request, "\r\n\r\n");
        if (!body) {
            send_response(client_fd, "400 Bad Request", "text/plain", "Invalid request");
            return;
        }
        body += 4;
        
        /* Parse multipart data */
        char project_name[256] = "";
        char message[512] = "Upload via web";
        char **files = NULL;
        char **file_contents = NULL;
        size_t *file_sizes = NULL;
        int file_count = 0;
        
        parse_multipart_form_data(body, boundary, project_name, sizeof(project_name),
                                message, sizeof(message), &files, &file_count,
                                &file_contents, &file_sizes);
        
        /* Validar upload */
        char *error_msg = NULL;
        if (!validate_upload(project_name, files, file_count, file_sizes, &error_msg)) {
            char error_html[1024];
            snprintf(error_html, sizeof(error_html),
                "<!DOCTYPE html><html><body><h1>❌ Erro no Upload</h1>"
                "<p>%s</p><a href='/upload'>← Tentar Novamente</a></body></html>",
                error_msg);
            send_response(client_fd, "400 Bad Request", "text/html; charset=utf-8", error_html);
            
            /* Liberar memória */
            if (files) {
                for (int i = 0; i < file_count; i++) {
                    free(files[i]);
                    free(file_contents[i]);
                }
                free(files);
                free(file_contents);
                free(file_sizes);
            }
            return;
        }
        
        /* Gerar commit_id único */
        time_t now = time(NULL);
        char commit_id[32];
        snprintf(commit_id, sizeof(commit_id), "%ld-%d", now, rand() % 100000);
        
        /* Criar estrutura de commit */
        char commit_dir[MAX_PATH];
        snprintf(commit_dir, sizeof(commit_dir), "%s/.clurg/projects/%s/commits", clurg_root, project_name);
        char mkdir_cmd[1024];
        snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"", commit_dir);
        system(mkdir_cmd);
        
        /* Criar arquivo tar.gz com os arquivos reais */
        if (!create_project_archive(clurg_root, project_name, commit_id, files, file_contents, file_sizes, file_count)) {
            send_response(client_fd, "500 Internal Server Error", "text/plain", "Failed to create archive");
            
            /* Liberar memória */
            if (files) {
                for (int i = 0; i < file_count; i++) {
                    free(files[i]);
                    free(file_contents[i]);
                }
                free(files);
                free(file_contents);
                free(file_sizes);
            }
            return;
        }
        
        /* Criar arquivo meta */
        char meta_path[MAX_PATH];
        snprintf(meta_path, sizeof(meta_path), "%s/%s.meta", commit_dir, commit_id);
        FILE *meta_file = fopen(meta_path, "w");
        if (meta_file) {
            fprintf(meta_file, "project=%s\n", project_name);
            fprintf(meta_file, "commit_id=%s\n", commit_id);
            fprintf(meta_file, "timestamp=%ld\n", now);
            fprintf(meta_file, "message=%s\n", message);
            fprintf(meta_file, "files=%d\n", file_count);
            
            /* Listar arquivos */
            for (int i = 0; i < file_count; i++) {
                fprintf(meta_file, "file%d=%s (%zu bytes)\n", i+1, files[i], file_sizes[i]);
            }
            
            fclose(meta_file);
        }
        
        /* Liberar memória */
        if (files) {
            for (int i = 0; i < file_count; i++) {
                free(files[i]);
                free(file_contents[i]);
            }
            free(files);
            free(file_contents);
            free(file_sizes);
        }
        
        /* Redirecionar para página de sucesso */
        char response_html[2048];
        snprintf(response_html, sizeof(response_html),
            "<!DOCTYPE html><html><body><h1>✅ Upload Concluído!</h1>"
            "<p><strong>Projeto:</strong> %s</p>"
            "<p><strong>Commit ID:</strong> %s</p>"
            "<p><strong>Arquivos:</strong> %d</p>"
            "<p><strong>Mensagem:</strong> %s</p>"
            "<br><a href='/projects/%s'>📁 Ver Projeto</a> | <a href='/upload'>📤 Fazer Outro Upload</a></body></html>",
            project_name, commit_id, file_count, message, project_name);
        send_response(client_fd, "200 OK", "text/html; charset=utf-8", response_html);
        // log_access(clurg_root, method, path, "200 OK", user_agent);
    }
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/push") == 0) {
        /* Handle push: receive project and commit_id, copy local files */
        char *body = strstr(request, "\r\n\r\n");
        char project_name[256] = "default";
        char commit_id[256] = "";
        char src_commit_path[MAX_PATH];
        char src_meta_path[MAX_PATH];
        char projects_dir[MAX_PATH];
        char project_commits_dir[MAX_PATH];
        char dst_commit_path[MAX_PATH];
        char dst_meta_path[MAX_PATH];
        
        if (body) {
            body += 4;
            /* Parse project=...&commit_id=... */
            char *project_param = strstr(body, "project=");
            char *commit_param = strstr(body, "commit_id=");
            
            if (project_param) {
                sscanf(project_param, "project=%255[^&]", project_name);
            }
            if (commit_param) {
                sscanf(commit_param, "commit_id=%255[^& \r\n]", commit_id);
            }
        }
        
        if (strlen(commit_id) == 0) {
            send_response(client_fd, "400 Bad Request", "text/plain", "Missing commit_id");
            return;
        }
        
        /* Source paths (local .clurg) */
        snprintf(src_commit_path, sizeof(src_commit_path), "%s/.clurg/commits/%s.tar.gz", clurg_root, commit_id);
        snprintf(src_meta_path, sizeof(src_meta_path), "%s/.clurg/commits/%s.meta", clurg_root, commit_id);
        
        /* Check if source files exist */
        if (access(src_commit_path, F_OK) != 0 || access(src_meta_path, F_OK) != 0) {
            send_response(client_fd, "404 Not Found", "text/plain", "Commit not found locally");
            return;
        }
        
        /* Create project directory */
        snprintf(projects_dir, sizeof(projects_dir), "%s/.clurg/projects", clurg_root);
        char mkdir_cmd[2048];
        snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"", projects_dir);
        system(mkdir_cmd);
        snprintf(project_commits_dir, sizeof(project_commits_dir), "%s/%s/commits", projects_dir, project_name);
        snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"", project_commits_dir);
        system(mkdir_cmd);
        
        /* Destination paths */
        snprintf(dst_commit_path, sizeof(dst_commit_path), "%s/%s.tar.gz", project_commits_dir, commit_id);
        snprintf(dst_meta_path, sizeof(dst_meta_path), "%s/%s.meta", project_commits_dir, commit_id);
        
        /* Copy files */
        if (copy_file(src_commit_path, dst_commit_path) == 0 &&
            copy_file(src_meta_path, dst_meta_path) == 0) {
            send_response(client_fd, "200 OK", "text/plain", "Commit received and saved");
        } else {
            send_response(client_fd, "500 Internal Server Error", "text/plain", "Failed to save");
        }
    }
    else if (strncmp(path, "/api/last-commit/", 17) == 0) {
        /* Return the last commit ID for a project */
        char project_name[256];
        char project_commits_dir[MAX_PATH];
        DIR *dir;
        struct dirent *entry;
        char last_commit[256] = "";
        time_t newest_time = 0;
        
        sscanf(path + 17, "%255[^/]", project_name);
        snprintf(project_commits_dir, sizeof(project_commits_dir), "%s/.clurg/projects/%s/commits", clurg_root, project_name);
        
        dir = opendir(project_commits_dir);
        if (dir) {
            while ((entry = readdir(dir)) != NULL) {
                if (strstr(entry->d_name, ".tar.gz")) {
                    char commit_id[256];
                    char meta_path[MAX_PATH];
                    struct stat st;
                    
                    sscanf(entry->d_name, "%255[^.]", commit_id);
                    snprintf(meta_path, sizeof(meta_path), "%s/%s.meta", project_commits_dir, commit_id);
                    
                    if (stat(meta_path, &st) == 0 && st.st_mtime > newest_time) {
                        newest_time = st.st_mtime;
                        strcpy(last_commit, commit_id);
                    }
                }
            }
            closedir(dir);
        }
        
        if (strlen(last_commit) > 0) {
            send_response(client_fd, "200 OK", "text/plain", last_commit);
            // log_access(clurg_root, method, path, "200 OK", user_agent);
        } else {
            send_response(client_fd, "404 Not Found", "text/plain", "No commits found");
            // log_access(clurg_root, method, path, "404 Not Found", user_agent);
        }
    }
    else if (strcmp(path, "/metrics") == 0) {
        /* Show system metrics */
        char html[32768];
        char metrics_content[16384] = "{}";
        char metrics_path[MAX_PATH];
        FILE *fp;
        
        snprintf(metrics_path, sizeof(metrics_path), "%s/.clurg/metrics.json", clurg_root);
        fp = fopen(metrics_path, "r");
        if (fp) {
            size_t n = fread(metrics_content, 1, sizeof(metrics_content) - 1, fp);
            metrics_content[n] = '\0';
            fclose(fp);
        }
        
        /* Parse JSON metrics (simplified) */
        int total_commits = 0, total_projects = 0, total_backups = 0, recent_commits = 0;
        int clurg_mb = 0, commits_mb = 0, projects_mb = 0, backups_mb = 0;
        int uptime_seconds = 0, uptime_hours = 0, memory_total = 0, memory_available = 0;
        char load_avg[64] = "unknown";
        
        char *clurg_start = strstr(metrics_content, "\"clurg\":{");
        if (clurg_start) {
            sscanf(clurg_start, "\"clurg\":{%*[^:]:%*[^,],%*[^:]:%d%*[^,],%*[^:]:%d%*[^,],%*[^:]:%d%*[^,],%*[^:]:%d", 
                   &total_commits, &total_projects, &total_backups, &recent_commits);
        }
        
        char *storage_start = strstr(metrics_content, "\"storage\":{");
        if (storage_start) {
            sscanf(storage_start, "\"storage\":{%*[^:]:%d%*[^,],%*[^:]:%d%*[^,],%*[^:]:%d%*[^,],%*[^:]:%d", 
                   &clurg_mb, &commits_mb, &projects_mb, &backups_mb);
        }
        
        char *system_start = strstr(metrics_content, "\"system\":{");
        if (system_start) {
            sscanf(system_start, "\"system\":{%*[^:]:%d%*[^,],%*[^:]:%63[^,]", &uptime_seconds, load_avg);
            uptime_hours = uptime_seconds / 3600;
        }
        
        snprintf(html, sizeof(html),
            "<!DOCTYPE html><html><head><title>Métricas - Clurg</title>"
            "<style>"
            ":root {"
            "  --bg-primary: #1a1a1a;"
            "  --bg-secondary: #2d2d2d;"
            "  --bg-card: #333333;"
            "  --text-primary: #ffffff;"
            "  --text-secondary: #cccccc;"
            "  --accent: #9b59b6;"
            "  --success: #27ae60;"
            "  --border: #444444;"
            "  --shadow: rgba(0,0,0,0.3);"
            "}"
            "@media (prefers-color-scheme: light) {"
            "  :root {"
            "    --bg-primary: #f8f9fa;"
            "    --bg-secondary: #ffffff;"
            "    --bg-card: #ffffff;"
            "    --text-primary: #2c3e50;"
            "    --text-secondary: #7f8c8d;"
            "    --accent: #3498db;"
            "    --success: #27ae60;"
            "    --border: #ecf0f1;"
            "    --shadow: rgba(0,0,0,0.1);"
            "  }"
            "}"
            "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 20px; background: var(--bg-primary); color: var(--text-primary); }"
            "h1 { color: var(--text-primary); text-align: center; border-bottom: 3px solid var(--accent); padding-bottom: 10px; }"
            ".metrics-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin: 20px 0; }"
            ".metric-card { background: var(--bg-card); padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px var(--shadow); }"
            ".metric-title { font-size: 18px; font-weight: bold; color: var(--text-primary); margin-bottom: 15px; border-bottom: 2px solid var(--border); padding-bottom: 5px; }"
            ".metric-value { font-size: 32px; font-weight: bold; color: var(--accent); }"
            ".metric-label { color: var(--text-secondary); font-size: 14px; margin-top: 5px; }"
            ".metric-item { margin: 10px 0; padding: 8px; background: var(--bg-secondary); border-radius: 4px; }"
            ".metric-item strong { color: var(--text-primary); }"
            ".system-info { background: var(--bg-secondary); padding: 15px; border-radius: 8px; margin-top: 20px; }"
            ".back-link { display: inline-block; margin-top: 20px; padding: 10px 20px; background: var(--accent); color: white; text-decoration: none; border-radius: 5px; }"
            ".back-link:hover { background: #8e44ad; }"
            "@media (max-width: 768px) {"
            "  .metrics-grid { grid-template-columns: 1fr; }"
            "  body { padding: 10px; }"
            "  .metric-card { padding: 15px; }"
            "  .metric-value { font-size: 24px; }"
            "}"
            "</style>"
            "</head><body>"
            "<h1>📊 Métricas do Sistema Clurg</h1>"
            
            "<div class='metrics-grid'>"
            "<div class='metric-card'>"
            "<div class='metric-title'>📦 Repositório</div>"
            "<div class='metric-item'><strong>Commits Totais:</strong> <span class='metric-value'>%d</span></div>"
            "<div class='metric-item'><strong>Projetos:</strong> <span class='metric-value'>%d</span></div>"
            "<div class='metric-item'><strong>Commits (7 dias):</strong> %d</div>"
            "<div class='metric-item'><strong>Backups:</strong> %d</div>"
            "</div>"
            
            "<div class='metric-card'>"
            "<div class='metric-title'>💽 Armazenamento</div>"
            "<div class='metric-item'><strong>Total Clurg:</strong> <span class='metric-value'>%d MB</span></div>"
            "<div class='metric-item'><strong>Commits:</strong> %d MB</div>"
            "<div class='metric-item'><strong>Projetos:</strong> %d MB</div>"
            "<div class='metric-item'><strong>Backups:</strong> %d MB</div>"
            "</div>"
            
            "<div class='metric-card'>"
            "<div class='metric-title'>🖥️ Sistema</div>"
            "<div class='metric-item'><strong>Uptime:</strong> <span class='metric-value'>%d h</span></div>"
            "<div class='metric-item'><strong>Load Average:</strong> %s</div>"
            "<div class='metric-item'><strong>Memória Total:</strong> %d MB</div>"
            "<div class='metric-item'><strong>Memória Disponível:</strong> %d MB</div>"
            "</div>"
            "</div>"
            
            "<div class='system-info'>"
            "<h3>🔧 Última Atualização</h3>"
            "<p>Métricas coletadas em tempo real do sistema. Atualize a página para ver os dados mais recentes.</p>"
            "<p><em>Para coletar novas métricas: <code>./scripts/collect-metrics.sh</code></em></p>"
            "</div>"
            
            "<a href='/' class='back-link'>← Voltar ao Início</a>"
            "</body></html>",
            total_commits, total_projects, recent_commits, total_backups,
            clurg_mb, commits_mb, projects_mb, backups_mb,
            uptime_hours, load_avg, memory_total / 1024, memory_available / 1024);
        
        send_response(client_fd, "200 OK", "text/html; charset=utf-8", html);
        // log_access(clurg_root, method, path, "200 OK", user_agent);
    }
    else if (strcmp(path, "/projects") == 0) {
        /* List projects */
        char html[16384];
        char projects_list[8192] = "";
        DIR *dir;
        struct dirent *entry;
        char projects_dir[MAX_PATH];
        
        snprintf(projects_dir, sizeof(projects_dir), "%s/.clurg/projects", clurg_root);
        dir = opendir(projects_dir);
        if (dir) {
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    char project_link[1024];
                    snprintf(project_link, sizeof(project_link),
                        "<li><a href='/projects/%s'>%s</a></li>", entry->d_name, entry->d_name);
                    strncat(projects_list, project_link, sizeof(projects_list) - strlen(projects_list) - 1);
                }
            }
            closedir(dir);
        }
        
        snprintf(html, sizeof(html),
            "<!DOCTYPE html><html><head><title>Projetos - Clurg</title>"
            "<style>"
            ":root {"
            "  --bg-primary: #1a1a1a;"
            "  --bg-secondary: #2d2d2d;"
            "  --bg-card: #333333;"
            "  --text-primary: #ffffff;"
            "  --text-secondary: #cccccc;"
            "  --accent: #9b59b6;"
            "  --success: #27ae60;"
            "  --error: #e74c3c;"
            "  --border: #444444;"
            "  --shadow: rgba(0,0,0,0.3);"
            "}"
            "@media (prefers-color-scheme: light) {"
            "  :root {"
            "    --bg-primary: #f5f5f5;"
            "    --bg-secondary: #ffffff;"
            "    --bg-card: #ffffff;"
            "    --text-primary: #2c3e50;"
            "    --text-secondary: #7f8c8d;"
            "    --accent: #3498db;"
            "    --success: #27ae60;"
            "    --error: #e74c3c;"
            "    --border: #95a5a6;"
            "    --shadow: rgba(0,0,0,0.1);"
            "  }"
            "}"
            "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 20px; background: var(--bg-primary); color: var(--text-primary); }"
            "h1 { color: var(--text-primary); border-bottom: 3px solid var(--accent); padding-bottom: 10px; }"
            "ul { list-style: none; padding: 0; }"
            "li { background: var(--bg-card); margin: 10px 0; padding: 15px; border-radius: 8px; box-shadow: 0 2px 4px var(--shadow); }"
            "li a { text-decoration: none; color: var(--accent); font-weight: bold; font-size: 18px; }"
            "li a:hover { color: #8e44ad; }"
            ".back-link { display: inline-block; margin-top: 20px; padding: 10px 20px; background: var(--border); color: white; text-decoration: none; border-radius: 5px; }"
            ".back-link:hover { background: #7f8c8d; }"
            "@media (max-width: 768px) {"
            "  body { padding: 10px; }"
            "  li { padding: 12px; margin: 8px 0; }"
            "  li a { font-size: 16px; }"
            "}"
            "</style>"
            "</head><body>"
            "<h1>📁 Projetos Disponíveis</h1>"
            "<div style='text-align: center; margin: 20px 0;'>"
            "<a href='/upload' style='display: inline-block; padding: 12px 24px; background: var(--success); color: white; text-decoration: none; border-radius: 5px; font-size: 16px;'>📤 Upload Novo Projeto</a>"
            "</div>"
            "<ul>%s</ul>"
            "<a href='/' class='back-link'>← Voltar ao Início</a>"
            "</body></html>",
            projects_list);
        
        send_response(client_fd, "200 OK", "text/html; charset=utf-8", html);
        // log_access(clurg_root, method, path, "200 OK", user_agent);
    }
    else if (strncmp(path, "/projects/", 10) == 0 && strstr(path, "/commit/")) {
        /* Show details of a specific commit: /projects/<project>/commit/<commit_id> */
        char project_name[256];
        char commit_id[256];
        char html[16384];
        char meta_content[4096] = "";
        char meta_path[MAX_PATH];
        char archive_path[MAX_PATH];
        struct stat st;
        char file_size[64];
        
        if (sscanf(path + 10, "%255[^/]/commit/%255[^/]", project_name, commit_id) == 2) {
            snprintf(meta_path, sizeof(meta_path), "%s/.clurg/projects/%s/commits/%s.meta", clurg_root, project_name, commit_id);
            snprintf(archive_path, sizeof(archive_path), "%s/.clurg/projects/%s/commits/%s.tar.gz", clurg_root, project_name, commit_id);
            
            /* Read metadata.json if available */
            char metadata_content[4096] = "";
            char metadata_path[MAX_PATH];
            snprintf(metadata_path, sizeof(metadata_path), "%s/.clurg/projects/%s/commits/%s.metadata.json", clurg_root, project_name, commit_id);
            FILE *metadata_f = fopen(metadata_path, "r");
            if (metadata_f) {
                size_t n = fread(metadata_content, 1, sizeof(metadata_content) - 1, metadata_f);
                metadata_content[n] = '\0';
                fclose(metadata_f);
            }

            /* Read checksum from meta file for integrity check */
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

            /* Get file size */
            if (stat(archive_path, &st) == 0) {
                if (st.st_size < 1024) {
                    snprintf(file_size, sizeof(file_size), "%ld bytes", st.st_size);
                } else if (st.st_size < 1024 * 1024) {
                    snprintf(file_size, sizeof(file_size), "%.1f KB", st.st_size / 1024.0);
                } else {
                    snprintf(file_size, sizeof(file_size), "%.1f MB", st.st_size / (1024.0 * 1024.0));
                }
            } else {
                strcpy(file_size, "Tamanho desconhecido");
            }
            
            /* Check integrity */
            char integrity_status[256] = "❓ Integridade não verificada (sem checksum)";
            if (expected_checksum[0] != '\0') {
                if (verify_file_integrity(archive_path, expected_checksum)) {
                    snprintf(integrity_status, sizeof(integrity_status), "✅ Integridade OK - Checksum: %s", expected_checksum);
                } else {
                    snprintf(integrity_status, sizeof(integrity_status), "❌ CORRUPÇÃO DETECTADA! Checksum esperado: %s", expected_checksum);
                }
            }
            
            /* Format meta content as HTML */
            char formatted_meta[8192] = "";
            char *line = strtok(meta_content, "\n");
            while (line) {
                if (strncmp(line, "message: ", 9) == 0) {
                    strncat(formatted_meta, "<div class='meta-item'><strong>📝 Mensagem:</strong> ", sizeof(formatted_meta) - strlen(formatted_meta) - 1);
                    strncat(formatted_meta, line + 9, sizeof(formatted_meta) - strlen(formatted_meta) - 1);
                    strncat(formatted_meta, "</div>", sizeof(formatted_meta) - strlen(formatted_meta) - 1);
                } else if (strncmp(line, "timestamp: ", 11) == 0) {
                    strncat(formatted_meta, "<div class='meta-item'><strong>🕒 Data:</strong> ", sizeof(formatted_meta) - strlen(formatted_meta) - 1);
                    strncat(formatted_meta, line + 11, sizeof(formatted_meta) - strlen(formatted_meta) - 1);
                    strncat(formatted_meta, "</div>", sizeof(formatted_meta) - strlen(formatted_meta) - 1);
                } else if (strncmp(line, "cwd: ", 5) == 0) {
                    strncat(formatted_meta, "<div class='meta-item'><strong>📁 Diretório:</strong> ", sizeof(formatted_meta) - strlen(formatted_meta) - 1);
                    strncat(formatted_meta, line + 5, sizeof(formatted_meta) - strlen(formatted_meta) - 1);
                    strncat(formatted_meta, "</div>", sizeof(formatted_meta) - strlen(formatted_meta) - 1);
                } else {
                    strncat(formatted_meta, "<div class='meta-item'>", sizeof(formatted_meta) - strlen(formatted_meta) - 1);
                    strncat(formatted_meta, line, sizeof(formatted_meta) - strlen(formatted_meta) - 1);
                    strncat(formatted_meta, "</div>", sizeof(formatted_meta) - strlen(formatted_meta) - 1);
                }
                line = strtok(NULL, "\n");
            }
            
            snprintf(html, sizeof(html),
                "<!DOCTYPE html><html><head><title>Commit %s - %s</title>"
                "<style>"
                ":root {"
                "  --bg-primary: #1a1a1a;"
                "  --bg-secondary: #2d2d2d;"
                "  --bg-card: #333333;"
                "  --text-primary: #ffffff;"
                "  --text-secondary: #cccccc;"
                "  --accent: #9b59b6;"
                "  --success: #27ae60;"
                "  --error: #e74c3c;"
                "  --border: #444444;"
                "  --shadow: rgba(0,0,0,0.3);"
                "}"
                "@media (prefers-color-scheme: light) {"
                "  :root {"
                "    --bg-primary: #f5f5f5;"
                "    --bg-secondary: #ffffff;"
                "    --bg-card: #ffffff;"
                "    --text-primary: #2c3e50;"
                "    --text-secondary: #7f8c8d;"
                "    --accent: #9b59b6;"
                "    --success: #27ae60;"
                "    --error: #e74c3c;"
                "    --border: #95a5a6;"
                "    --shadow: rgba(0,0,0,0.1);"
                "  }"
                "}"
                "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 20px; background: var(--bg-primary); color: var(--text-primary); }"
                "h1 { color: var(--text-primary); border-bottom: 3px solid var(--accent); padding-bottom: 10px; }"
                ".commit-info { background: var(--bg-card); padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px var(--shadow); margin: 20px 0; }"
                ".commit-id { font-family: monospace; font-size: 20px; color: var(--accent); background: var(--bg-secondary); padding: 10px; border-radius: 4px; display: inline-block; }"
                ".file-size { color: var(--text-secondary); font-size: 16px; margin: 10px 0; }"
                ".integrity-status { margin: 10px 0; padding: 8px; border-radius: 4px; font-weight: bold; }"
                ".meta-section { background: var(--bg-card); padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px var(--shadow); }"
                ".metadata-section { background: var(--bg-card); padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px var(--shadow); margin-top: 20px; }"
                ".metadata-json { background: var(--bg-secondary); padding: 15px; border-radius: 4px; font-family: 'Courier New', monospace; font-size: 14px; overflow-x: auto; border: 1px solid var(--border); color: var(--text-primary); }"
                ".meta-item { margin: 10px 0; padding: 8px; background: var(--bg-secondary); border-radius: 4px; }"
                ".actions { margin-top: 20px; }"
                ".btn { display: inline-block; padding: 12px 24px; text-decoration: none; border-radius: 5px; font-size: 16px; margin-right: 10px; }"
                ".btn-download { background: var(--success); color: white; }"
                ".btn-download:hover { background: #229954; }"
                ".btn-back { background: var(--border); color: white; }"
                ".btn-back:hover { background: #7f8c8d; }"
                "@media (max-width: 768px) {"
                "  body { padding: 10px; }"
                "  .commit-info, .meta-section, .metadata-section { padding: 15px; }"
                "  .btn { padding: 10px 20px; font-size: 14px; margin-right: 5px; }"
                "}"
                "</style>"
                "</head><body>"
                "<h1>🔍 Detalhes do Commit</h1>"
                "<div class='commit-info'>"
                "<div><strong>Projeto:</strong> %s</div>"
                "<div><strong>ID do Commit:</strong> <span class='commit-id'>%s</span></div>"
                "<div class='file-size'>📦 Tamanho do arquivo: %s</div>"
                "<div class='integrity-status'>%s</div>"
                "</div>"
                "<div class='meta-section'>"
                "<h2>📋 Metadados</h2>"
                "%s"
                "</div>"
                "<div class='metadata-section'>"
                "<h2>🔧 Metadados Estruturados</h2>"
                "<pre class='metadata-json'>%s</pre>"
                "</div>"
                "<div class='actions'>"
                "<a href='/download/%s/%s' class='btn btn-download'>⬇️ Baixar Commit</a>"
                "<a href='/projects/%s' class='btn btn-back'>← Voltar aos Commits</a>"
                "</div>"
                "</body></html>",
                commit_id, project_name, project_name, commit_id, file_size, integrity_status, formatted_meta, metadata_content[0] != '\0' ? metadata_content : "Metadados estruturados não disponíveis", project_name, commit_id, project_name);
            
            send_response(client_fd, "200 OK", "text/html; charset=utf-8", html);
            // log_access(clurg_root, method, path, "200 OK", user_agent);
        } else {
            send_response(client_fd, "400 Bad Request", "text/plain", "Invalid commit path");
            // log_access(clurg_root, method, path, "400 Bad Request", user_agent);
        }
    }
    else if (strncmp(path, "/projects/", 10) == 0) {
        /* List commits for a specific project */
        char project_name[256];
        char html[16384];
        char commits_list[8192] = "";
        DIR *dir;
        struct dirent *entry;
        char project_commits_dir[MAX_PATH];
        
        sscanf(path + 10, "%255[^/]", project_name);
        snprintf(project_commits_dir, sizeof(project_commits_dir), "%s/.clurg/projects/%s/commits", clurg_root, project_name);
        
        dir = opendir(project_commits_dir);
        if (dir) {
            while ((entry = readdir(dir)) != NULL) {
                if (strstr(entry->d_name, ".tar.gz")) {
                    char commit_id[256];
                    char meta_path[MAX_PATH];
                    char message[256] = "Sem mensagem";
                    char timestamp[256] = "Data desconhecida";
                    
                    sscanf(entry->d_name, "%255[^.]", commit_id);
                    snprintf(meta_path, sizeof(meta_path), "%s/%s.meta", project_commits_dir, commit_id);
                    
                    /* Read meta file */
                    FILE *f = fopen(meta_path, "r");
                    if (f) {
                        char line[512];
                        while (fgets(line, sizeof(line), f)) {
                            if (strncmp(line, "message: ", 9) == 0) {
                                strncpy(message, line + 9, sizeof(message) - 1);
                                message[strcspn(message, "\n")] = 0;
                            } else if (strncmp(line, "timestamp: ", 11) == 0) {
                                strncpy(timestamp, line + 11, sizeof(timestamp) - 1);
                                timestamp[strcspn(timestamp, "\n")] = 0;
                            }
                        }
                        fclose(f);
                    }
                    
                    char item[2048];
                    snprintf(item, sizeof(item),
                        "<li>"
                        "<div class='commit-header'>"
                        "<strong class='commit-id'>%s</strong>"
                        "<span class='commit-date'>%s</span>"
                        "</div>"
                        "<div class='commit-message'>%s</div>"
                        "<div class='commit-actions'>"
                        "<a href='/projects/%s/commit/%s' class='btn-details'>📋 Detalhes</a>"
                        "<a href='/download/%s/%s' class='btn-download'>⬇️ Baixar</a>"
                        "</div>"
                        "</li>",
                        commit_id, timestamp, message, project_name, commit_id, project_name, commit_id);
                    strncat(commits_list, item, sizeof(commits_list) - strlen(commits_list) - 1);
                }
            }
            closedir(dir);
        }
        
        snprintf(html, sizeof(html),
            "<!DOCTYPE html><html><head><title>Projeto: %s - Clurg</title>"
            "<style>"
            ":root {"
            "  --bg-primary: #1a1a1a;"
            "  --bg-secondary: #2d2d2d;"
            "  --bg-card: #333333;"
            "  --text-primary: #ffffff;"
            "  --text-secondary: #cccccc;"
            "  --accent: #9b59b6;"
            "  --success: #27ae60;"
            "  --error: #e74c3c;"
            "  --border: #444444;"
            "  --shadow: rgba(0,0,0,0.3);"
            "}"
            "@media (prefers-color-scheme: light) {"
            "  :root {"
            "    --bg-primary: #f5f5f5;"
            "    --bg-secondary: #ffffff;"
            "    --bg-card: #ffffff;"
            "    --text-primary: #2c3e50;"
            "    --text-secondary: #7f8c8d;"
            "    --accent: #3498db;"
            "    --success: #27ae60;"
            "    --error: #e74c3c;"
            "    --border: #95a5a6;"
            "    --shadow: rgba(0,0,0,0.1);"
            "  }"
            "}"
            "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 20px; background: var(--bg-primary); color: var(--text-primary); }"
            "h1 { color: var(--text-primary); border-bottom: 3px solid var(--error); padding-bottom: 10px; }"
            "ul { list-style: none; padding: 0; }"
            "li { background: var(--bg-card); margin: 15px 0; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px var(--shadow); }"
            ".commit-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; }"
            ".commit-id { font-family: monospace; font-size: 16px; color: var(--error); background: var(--bg-secondary); padding: 5px 10px; border-radius: 4px; }"
            ".commit-date { color: var(--text-secondary); font-size: 14px; }"
            ".commit-message { color: var(--text-secondary); margin: 10px 0; font-style: italic; }"
            ".commit-actions { margin-top: 15px; }"
            ".btn-details, .btn-download { display: inline-block; margin-right: 10px; padding: 8px 16px; text-decoration: none; border-radius: 5px; font-size: 14px; }"
            ".btn-details { background: var(--accent); color: white; }"
            ".btn-details:hover { background: #8e44ad; }"
            ".btn-download { background: var(--success); color: white; }"
            ".btn-download:hover { background: #229954; }"
            ".back-link { display: inline-block; margin-top: 20px; padding: 10px 20px; background: var(--border); color: white; text-decoration: none; border-radius: 5px; }"
            ".back-link:hover { background: #7f8c8d; }"
            "@media (max-width: 768px) {"
            "  body { padding: 10px; }"
            "  li { padding: 15px; margin: 10px 0; }"
            "  .commit-header { flex-direction: column; align-items: flex-start; gap: 5px; }"
            "  .btn-details, .btn-download { padding: 6px 12px; font-size: 12px; margin-right: 5px; }"
            "}"
            "</style>"
            "</head><body>"
            "<h1>📦 Commits do Projeto: %s</h1>"
            "<ul>%s</ul>"
            "<a href='/projects' class='back-link'>← Voltar aos Projetos</a>"
            "</body></html>",
            project_name, project_name, commits_list);
        
        send_response(client_fd, "200 OK", "text/html; charset=utf-8", html);
    }
    else if (strncmp(path, "/download/", 10) == 0) {
        /* Download commit: /download/<project>/<commit_id> */
        char project_name[256];
        char commit_id[256];
        char archive_path[MAX_PATH];
        char meta_path[MAX_PATH];
        
        if (sscanf(path + 10, "%255[^/]/%255[^/]", project_name, commit_id) == 2) {
            snprintf(archive_path, sizeof(archive_path), "%s/.clurg/projects/%s/commits/%s.tar.gz", clurg_root, project_name, commit_id);
            snprintf(meta_path, sizeof(meta_path), "%s/.clurg/projects/%s/commits/%s.meta", clurg_root, project_name, commit_id);
            
            if (access(archive_path, F_OK) == 0) {
                /* Verificar integridade antes de enviar */
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
                    /* Verificar integridade */
                    if (!verify_file_integrity(archive_path, expected_checksum)) {
                        send_response(client_fd, "500 Internal Server Error", "text/plain", 
                            "ERRO DE INTEGRIDADE: O arquivo foi corrompido! Checksum não corresponde.");
                        // log_access(clurg_root, method, path, "500 Internal Server Error", user_agent);
                        return;
                    }
                }
                
                send_file_response(client_fd, "200 OK", "application/gzip", archive_path);
                // log_access(clurg_root, method, path, "200 OK", user_agent);
            } else {
                send_response(client_fd, "404 Not Found", "text/plain", "Commit not found");
                // log_access(clurg_root, method, path, "404 Not Found", user_agent);
            }
        } else {
            send_response(client_fd, "400 Bad Request", "text/plain", "Invalid download path");
            // log_access(clurg_root, method, path, "400 Bad Request", user_agent);
        }
    }
    else if (strcmp(path, "/") == 0 || strncmp(path, "/index.html", 11) == 0 || strncmp(path, "/?", 2) == 0) {
        char html[16384];
        char log_list[8192];
        char refresh_meta[128] = "";
        int refresh_seconds = 0;
        
        /* Verificar parâmetro refresh na query string */
        char *query_start = strchr(path, '?');
        if (query_start) {
            char query[256];
            strncpy(query, query_start + 1, sizeof(query) - 1);
            query[sizeof(query) - 1] = '\0';
            
            if (strncmp(query, "refresh=", 8) == 0) {
                refresh_seconds = atoi(query + 8);
                if (refresh_seconds > 0 && refresh_seconds <= 300) {
                    snprintf(refresh_meta, sizeof(refresh_meta), 
                        "<meta http-equiv='refresh' content='%d'>", refresh_seconds);
                }
            }
        }
        
        list_log_files(log_list, sizeof(log_list), logs_dir);
        
        snprintf(html, sizeof(html),
            "<!DOCTYPE html>\n"
            "<html><head>"
            "<title>Clurg - Gerenciador de Projetos</title>"
            "<meta charset='UTF-8'>"
            "%s"
            "<style>"
            ":root {"
            "  --bg-primary: #1a1a1a;"
            "  --bg-secondary: #2d2d2d;"
            "  --bg-card: #333333;"
            "  --text-primary: #ffffff;"
            "  --text-secondary: #cccccc;"
            "  --accent: #9b59b6;"
            "  --success: #27ae60;"
            "  --error: #e74c3c;"
            "  --border: #444444;"
            "  --shadow: rgba(0,0,0,0.3);"
            "}"
            "@media (prefers-color-scheme: light) {"
            "  :root {"
            "    --bg-primary: #f5f5f5;"
            "    --bg-secondary: #ffffff;"
            "    --bg-card: #ffffff;"
            "    --text-primary: #2c3e50;"
            "    --text-secondary: #7f8c8d;"
            "    --accent: #3498db;"
            "    --success: #27ae60;"
            "    --error: #e74c3c;"
            "    --border: #95a5a6;"
            "    --shadow: rgba(0,0,0,0.1);"
            "  }"
            "}"
            "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 20px; background: var(--bg-primary); color: var(--text-primary); max-width: 1200px; margin: 0 auto; }"
            "h1 { color: var(--text-primary); border-bottom: 3px solid var(--accent); padding-bottom: 10px; }"
            ".hero { background: var(--bg-card); padding: 30px; border-radius: 8px; box-shadow: 0 2px 4px var(--shadow); margin: 20px 0; }"
            ".hero p { font-size: 18px; color: var(--text-secondary); margin: 10px 0; }"
            ".btn-primary { display: inline-block; padding: 15px 30px; background: var(--accent); color: white; text-decoration: none; border-radius: 5px; font-size: 18px; font-weight: bold; }"
            ".btn-primary:hover { background: #8e44ad; }"
            ".project-info { background: var(--bg-card); padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px var(--shadow); margin: 20px 0; }"
            ".project-info strong { color: var(--error); }"
            "h2 { color: var(--text-primary); border-bottom: 2px solid var(--border); padding-bottom: 5px; }"
            "ul { list-style: none; padding: 0; }"
            "li { background: var(--bg-card); padding: 12px; margin: 8px 0; border-radius: 4px; box-shadow: 0 1px 2px var(--shadow); }"
            "a { color: var(--accent); text-decoration: none; }"
            "a:hover { color: #8e44ad; text-decoration: underline; }"
            "pre { background: var(--bg-secondary); color: var(--text-primary); padding: 20px; border-radius: 4px; overflow-x: auto; }"
            ".status-ok { color: var(--success); }"
            ".status-fail { color: var(--error); }"
            ".refresh-controls { margin: 20px 0; }"
            ".refresh-controls a { display: inline-block; margin-right: 10px; padding: 8px 16px; background: var(--border); color: white; text-decoration: none; border-radius: 4px; }"
            ".refresh-controls a:hover { background: #7f8c8d; }"
            "@media (max-width: 768px) {"
            "  body { padding: 10px; }"
            "  .hero { padding: 20px; }"
            "  .btn-primary { padding: 12px 24px; font-size: 16px; }"
            "  li { padding: 10px; }"
            "}"
            "</style>"
            "</head><body>"
            "<h1>🏠 Clurg - Gerenciador de Projetos</h1>"
            "<div class='hero'>"
            "<p>Clurg é um sistema de controle de versão pessoal e educacional, focado em simplicidade e aprendizado.</p>"
            "<p>Gerencie seus projetos com commits versionados e backup confiável.</p>"
            "<a href='/projects' class='btn-primary'>📁 Explorar Projetos</a>"
            "</div>"
            "<div class='project-info'>"
            "<h2>📊 Status do Sistema</h2>"
            "<p>Diretório raiz: <strong>%s</strong></p>"
            "</div>"
            "<h2>📋 Logs de CI Recentes</h2>"
            "%s"
            "</body></html>",
            refresh_meta, clurg_root, log_list);
        
        send_response(client_fd, "200 OK", "text/html; charset=utf-8", html);
        // log_access(clurg_root, method, path, "200 OK", user_agent);
    }
    else if (strncmp(path, "/logs/", 6) == 0) {
        char log_file[MAX_PATH];
        char *log_name = (char *)path + 6; /* Pular "/logs/" */
        
        /* Segurança: evitar path traversal */
        /* Rejeitar: "..", "/", "\", caracteres de controle */
        if (strstr(log_name, "..") != NULL || 
            strchr(log_name, '/') != NULL ||
            strchr(log_name, '\\') != NULL ||
            strpbrk(log_name, "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f") != NULL) {
            send_response(client_fd, "403 Forbidden", "text/html; charset=utf-8", 
                "<html><body><h1>403 Forbidden</h1><p>Path traversal attempt blocked.</p></body></html>");
            return;
        }
        
        /* Validar comprimento do nome do arquivo */
        if (strlen(log_name) == 0 || strlen(log_name) > 255) {
            send_response(client_fd, "400 Bad Request", "text/html; charset=utf-8",
                "<html><body><h1>400 Bad Request</h1><p>Invalid log file name.</p></body></html>");
            return;
        }
        
        snprintf(log_file, sizeof(log_file), "%s/%s", logs_dir, log_name);
        
        /* Verificar se arquivo existe e é regular */
        struct stat st;
        if (stat(log_file, &st) != 0 || !S_ISREG(st.st_mode)) {
            send_response(client_fd, "404 Not Found", "text/html; charset=utf-8",
                "<html><body><h1>404 Not Found</h1><p>Log file not found.</p><a href='/'>← Voltar</a></body></html>");
            return;
        }
        
        /* Limitar tamanho máximo de arquivo (1MB) */
        if (st.st_size > 1024 * 1024) {
            send_response(client_fd, "413 Payload Too Large", "text/html; charset=utf-8",
                "<html><body><h1>413 Payload Too Large</h1><p>Log file too large to display (max 1MB).</p><a href='/'>← Voltar</a></body></html>");
            return;
        }
        
        char html[16384];
        char log_content[12288];
        
        read_log_file(log_content, sizeof(log_content), log_file, st.st_size);
        
        snprintf(html, sizeof(html),
            "<!DOCTYPE html>\n"
            "<html><head>"
            "<title>Log: %s - Clurg</title>"
            "<meta charset='UTF-8'>"
            "<style>"
            "body { font-family: sans-serif; max-width: 1200px; margin: 40px auto; padding: 20px; background: #f5f5f5; }"
            "h1 { color: #333; }"
            "pre { background: #1e1e1e; color: #d4d4d4; padding: 20px; border-radius: 4px; overflow-x: auto; }"
            "a { color: #0066cc; text-decoration: none; }"
            "a:hover { text-decoration: underline; }"
            "</style>"
            "</head><body>"
            "<h1>Log: %s</h1>"
            "<p><a href='/'>← Voltar</a></p>"
            "%s"
            "</body></html>",
            log_name, log_name, log_content);
        
        send_response(client_fd, "200 OK", "text/html; charset=utf-8", html);
    }
    /* API REST Routes */
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/system/status") == 0) {
        api_response_t response;
        api_system_status(&response, clurg_root);
        send_response(client_fd, response.status_code == 200 ? "200 OK" : "500 Internal Server Error",
                     response.content_type, response.body);
        // log_access(clurg_root, method, path, response.status_code == 200 ? "200 OK" : "500 Internal Server Error", user_agent);
    }
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/projects") == 0) {
        api_response_t response;
        api_projects_list(&response, clurg_root);
        send_response(client_fd, response.status_code == 200 ? "200 OK" : "404 Not Found",
                     response.content_type, response.body);
        // log_access(clurg_root, method, path, response.status_code == 200 ? "200 OK" : "404 Not Found", user_agent);
    }
    else if (strcmp(method, "GET") == 0 && strncmp(path, "/api/v1/projects/", 17) == 0 && !strstr(path, "/commits") && !strstr(path, "/deploy")) {
        char project_name[256];
        sscanf(path + 17, "%255[^/]", project_name);
        api_response_t response;
        api_project_get(&response, clurg_root, project_name);
        send_response(client_fd, response.status_code == 200 ? "200 OK" : "404 Not Found",
                     response.content_type, response.body);
        // log_access(clurg_root, method, path, response.status_code == 200 ? "200 OK" : "404 Not Found", user_agent);
    }
    else if (strcmp(method, "GET") == 0 && strncmp(path, "/api/v1/projects/", 17) == 0 && strstr(path, "/commits")) {
        char project_name[256];
        sscanf(path + 17, "%255[^/]", project_name);
        api_response_t response;
        api_project_commits(&response, clurg_root, project_name);
        send_response(client_fd, response.status_code == 200 ? "200 OK" : "404 Not Found",
                     response.content_type, response.body);
        // log_access(clurg_root, method, path, response.status_code == 200 ? "200 OK" : "404 Not Found", user_agent);
    }
    else if (strcmp(method, "POST") == 0 && strncmp(path, "/api/v1/projects/", 17) == 0 && strstr(path, "/deploy")) {
        char project_name[256];
        sscanf(path + 17, "%255[^/]", project_name);
        
        // Parse request body
        char *body = strstr(request, "\r\n\r\n");
        if (body) {
            body += 4;
        } else {
            body = "";
        }
        
        api_response_t response;
        api_project_deploy(&response, clurg_root, project_name, body);
        send_response(client_fd, response.status_code == 200 ? "200 OK" : 
                     (response.status_code == 400 ? "400 Bad Request" : "404 Not Found"),
                     response.content_type, response.body);
        // log_access(clurg_root, method, path, response.status_code == 200 ? "200 OK" : 
        //            (response.status_code == 400 ? "400 Bad Request" : "404 Not Found"), user_agent);
    }
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/projects") == 0) {
        // Parse request body
        char *body = strstr(request, "\r\n\r\n");
        if (body) {
            body += 4;
        } else {
            body = "";
        }
        
        api_response_t response;
        api_projects_create(&response, clurg_root, body);
        send_response(client_fd, response.status_code == 200 ? "201 Created" : 
                     (response.status_code == 400 ? "400 Bad Request" : 
                     (response.status_code == 409 ? "409 Conflict" : "500 Internal Server Error")),
                     response.content_type, response.body);
    }
    else if (strcmp(method, "DELETE") == 0 && strncmp(path, "/api/v1/projects/", 17) == 0) {
        char *remaining = path + 17;
        if (!strchr(remaining, '/')) {
            char project_name[256];
            sscanf(remaining, "%255s", project_name);
            api_response_t response;
            api_projects_delete(&response, clurg_root, project_name);
            send_response(client_fd, response.status_code == 200 ? "200 OK" : "404 Not Found",
                         response.content_type, response.body);
        } else {
            send_response(client_fd, "404 Not Found", "text/html; charset=utf-8",
                "<html><body><h1>404 Not Found</h1><p>The requested resource was not found.</p><a href='/'>← Voltar</a></body></html>");
        }
    }
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/system/metrics") == 0) {
        api_response_t response;
        api_system_metrics(&response, clurg_root);
        send_response(client_fd, response.status_code == 200 ? "200 OK" : "500 Internal Server Error",
                     response.content_type, response.body);
    }
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/system/logs") == 0) {
        api_response_t response;
        api_system_logs(&response, clurg_root);
        send_response(client_fd, response.status_code == 200 ? "200 OK" : "500 Internal Server Error",
                     response.content_type, response.body);
    }
    else if (strcmp(method, "POST") == 0 && strncmp(path, "/api/v1/projects/", 17) == 0 && strstr(path, "/ci/run")) {
        char project_name[256];
        sscanf(path + 17, "%255[^/]", project_name);
        api_response_t response;
        api_project_ci_run(&response, clurg_root, project_name);
        send_response(client_fd, response.status_code == 200 ? "200 OK" : "404 Not Found",
                     response.content_type, response.body);
    }
    else if (strcmp(method, "GET") == 0 && strncmp(path, "/api/v1/projects/", 17) == 0 && strstr(path, "/ci/status")) {
        char project_name[256];
        sscanf(path + 17, "%255[^/]", project_name);
        api_response_t response;
        api_project_ci_status(&response, clurg_root, project_name);
        send_response(client_fd, response.status_code == 200 ? "200 OK" : "404 Not Found",
                     response.content_type, response.body);
    }
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/webhooks/test") == 0) {
        // Parse request body
        char *body = strstr(request, "\r\n\r\n");
        if (body) {
            body += 4;
        } else {
            body = "";
        }
        
        api_response_t response;
        api_webhooks_test(&response, clurg_root, body);
        send_response(client_fd, response.status_code == 200 ? "200 OK" : "400 Bad Request",
                     response.content_type, response.body);
    }
    else {
        /* 404 Not Found com HTML apropriado */
        send_response(client_fd, "404 Not Found", "text/html; charset=utf-8",
            "<html><body><h1>404 Not Found</h1><p>The requested resource was not found.</p><a href='/'>← Voltar</a></body></html>");
    }
}

static char *get_clurg_root(void) {
    static char root[PATH_MAX];
    char cwd[PATH_MAX];
    char *p;
    
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return NULL;
    }
    
    strncpy(root, cwd, sizeof(root) - 1);
    root[sizeof(root) - 1] = '\0';
    
    while (1) {
        char test_path[PATH_MAX];
        snprintf(test_path, sizeof(test_path), "%s/.clurg", root);
        
        if (access(test_path, F_OK) == 0) {
            return root;
        }
        
        p = strrchr(root, '/');
        if (!p || p == root) {
            return getcwd(root, sizeof(root)) ? root : NULL;
        }
        
        *p = '\0';
    }
}

int main(int argc, char *argv[]) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char *clurg_root;
    int port = PORT;
    
    if (argc >= 2) {
        char *endptr;
        long port_long = strtol(argv[1], &endptr, 10);
        
        /* Validar: número válido, range correto (1-65535), sem caracteres extras */
        if (*endptr != '\0' || port_long <= 0 || port_long > 65535) {
            fprintf(stderr, "Erro: Porta inválida: %s\n", argv[1]);
            fprintf(stderr, "Porta deve ser um número entre 1 e 65535\n");
            return 1;
        }
        
        port = (int)port_long;
        
        /* Portas privilegiadas requerem root (avisar mas permitir se usuário souber o que faz) */
        if (port < 1024) {
            fprintf(stderr, "Aviso: Porta %d requer privilégios de root (portas < 1024)\n", port);
        }
    }
    
    clurg_root = get_clurg_root();
    if (!clurg_root) {
        fprintf(stderr, "erro: não foi possível determinar raiz do projeto Clurg\n");
        return 1;
    }
    
    printf("Clurg Web Server iniciando...\n");
    printf("Raiz do projeto: %s\n", clurg_root);
    printf("Servindo na porta: %d\n", port);
    printf("Acesse: http://localhost:%d\n", port);
    
    /* Criar socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }
    
    /* Configurar opções do socket */
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    /* Configurar endereço */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    /* Bind */
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }
    
    /* Listen */
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }
    
    /* Loop principal */
    while (1) {
        char buffer[BUFFER_SIZE];
        ssize_t n;
        
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        
        /* Ler requisição */
        n = read(client_fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            handle_request(client_fd, buffer, clurg_root);
        }
        
        close(client_fd);
    }
    
    close(server_fd);
    return 0;
}

