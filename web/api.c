#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <jansson.h>  // Para JSON
#include "api.h"

// Função auxiliar para criar resposta JSON de sucesso
static void api_success(api_response_t *response, json_t *data) {
    json_t *root = json_object();
    json_object_set_new(root, "success", json_true());
    json_object_set(root, "data", data);

    // Adicionar metadata
    json_t *meta = json_object();
    char timestamp[32];
    time_t now = time(NULL);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
    json_object_set_new(meta, "timestamp", json_string(timestamp));
    json_object_set_new(root, "meta", meta);

    char *json_str = json_dumps(root, JSON_COMPACT);
    if (json_str) {
        response->status_code = 200;
        strcpy(response->content_type, "application/json");
        strncpy(response->body, json_str, sizeof(response->body) - 1);
        free(json_str);
    }
    json_decref(root);
}

// Função auxiliar para criar resposta JSON de erro
static void api_error(api_response_t *response, int status_code, const char *code, const char *message) {
    json_t *root = json_object();
    json_object_set_new(root, "success", json_false());

    json_t *error = json_object();
    json_object_set_new(error, "code", json_string(code));
    json_object_set_new(error, "message", json_string(message));
    json_object_set_new(root, "error", error);

    // Adicionar metadata
    json_t *meta = json_object();
    char timestamp[32];
    time_t now = time(NULL);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
    json_object_set_new(meta, "timestamp", json_string(timestamp));
    json_object_set_new(root, "meta", meta);

    char *json_str = json_dumps(root, JSON_COMPACT);
    if (json_str) {
        response->status_code = status_code;
        strcpy(response->content_type, "application/json");
        strncpy(response->body, json_str, sizeof(response->body) - 1);
        free(json_str);
    }
    json_decref(root);
}

// GET /api/v1/system/status
void api_system_status(api_response_t *response, const char *clurg_root) {
    json_t *data = json_object();
    json_object_set_new(data, "status", json_string("healthy"));
    json_object_set_new(data, "version", json_string("1.0.0"));
    json_object_set_new(data, "uptime", json_integer(123456)); // TODO: calcular uptime real

    api_success(response, data);
}

// GET /api/v1/projects
void api_projects_list(api_response_t *response, const char *clurg_root) {
    json_t *projects = json_array();
    char projects_dir[MAX_PATH];

    snprintf(projects_dir, sizeof(projects_dir), "%s/.clurg/projects", clurg_root);

    DIR *dir = opendir(projects_dir);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] != '.' && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                json_t *project = json_object();
                json_object_set_new(project, "name", json_string(entry->d_name));

                // Verificar se tem commits
                char commits_dir[MAX_PATH];
                snprintf(commits_dir, sizeof(commits_dir), "%s/%s/commits", projects_dir, entry->d_name);
                DIR *commits_d = opendir(commits_dir);
                if (commits_d) {
                    int commit_count = 0;
                    struct dirent *c_entry;
                    while ((c_entry = readdir(commits_d)) != NULL) {
                        if (strstr(c_entry->d_name, ".tar.gz")) {
                            commit_count++;
                        }
                    }
                    closedir(commits_d);
                    json_object_set_new(project, "commits", json_integer(commit_count));
                }

                json_array_append_new(projects, project);
            }
        }
        closedir(dir);
    }

    json_t *data = json_object();
    json_object_set_new(data, "projects", projects);
    json_object_set_new(data, "total", json_integer(json_array_size(projects)));

    api_success(response, data);
}

// GET /api/v1/projects/{name}
void api_project_get(api_response_t *response, const char *clurg_root, const char *project_name) {
    char project_dir[MAX_PATH];
    json_t *data = json_object();

    snprintf(project_dir, sizeof(project_dir), "%s/.clurg/projects/%s", clurg_root, project_name);

    // Verificar se projeto existe
    if (access(project_dir, F_OK) != 0) {
        api_error(response, 404, "PROJECT_NOT_FOUND", "Project not found");
        return;
    }

    json_object_set_new(data, "name", json_string(project_name));

    // Contar commits
    char commits_dir[MAX_PATH];
    snprintf(commits_dir, sizeof(commits_dir), "%s/commits", project_dir);
    DIR *dir = opendir(commits_dir);
    if (dir) {
        int commit_count = 0;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strstr(entry->d_name, ".tar.gz")) {
                commit_count++;
            }
        }
        closedir(dir);
        json_object_set_new(data, "commits", json_integer(commit_count));
    }

    // Verificar CI status
    char ci_status_file[MAX_PATH];
    snprintf(ci_status_file, sizeof(ci_status_file), "%s/.clurg/projects/%s/ci/last_status", clurg_root, project_name);
    FILE *fp = fopen(ci_status_file, "r");
    if (fp) {
        char status[16];
        if (fgets(status, sizeof(status), fp)) {
            status[strcspn(status, "\n")] = 0;
            json_object_set_new(data, "ci_status", json_string(status));
        }
        fclose(fp);
    }

    api_success(response, data);
}

// GET /api/v1/projects/{name}/commits
void api_project_commits(api_response_t *response, const char *clurg_root, const char *project_name) {
    json_t *commits = json_array();
    char commits_dir[MAX_PATH];

    snprintf(commits_dir, sizeof(commits_dir), "%s/.clurg/projects/%s/commits", clurg_root, project_name);

    DIR *dir = opendir(commits_dir);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strstr(entry->d_name, ".tar.gz")) {
                char commit_id[256];
                sscanf(entry->d_name, "%255[^.]", commit_id);

                json_t *commit = json_object();
                json_object_set_new(commit, "id", json_string(commit_id));

                // Ler metadados
                char meta_path[MAX_PATH];
                snprintf(meta_path, sizeof(meta_path), "%s/%s.meta", commits_dir, commit_id);
                FILE *fp = fopen(meta_path, "r");
                if (fp) {
                    char line[512];
                    while (fgets(line, sizeof(line), fp)) {
                        if (strncmp(line, "message: ", 9) == 0) {
                            char *msg = line + 9;
                            msg[strcspn(msg, "\n")] = 0;
                            json_object_set_new(commit, "message", json_string(msg));
                        } else if (strncmp(line, "timestamp: ", 11) == 0) {
                            char *ts = line + 11;
                            ts[strcspn(ts, "\n")] = 0;
                            json_object_set_new(commit, "timestamp", json_string(ts));
                        }
                    }
                    fclose(fp);
                }

                json_array_append_new(commits, commit);
            }
        }
        closedir(dir);
    }

    json_t *data = json_object();
    json_object_set_new(data, "commits", commits);
    json_object_set_new(data, "total", json_integer(json_array_size(commits)));

    api_success(response, data);
}

// POST /api/v1/projects/{name}/deploy
void api_project_deploy(api_response_t *response, const char *clurg_root, const char *project_name, const char *body) {
    // Parse JSON body
    json_error_t error;
    json_t *root = json_loads(body, 0, &error);

    if (!root) {
        api_error(response, 400, "INVALID_JSON", "Invalid JSON in request body");
        return;
    }

    const char *environment = json_string_value(json_object_get(root, "environment"));
    const char *commit_id = json_string_value(json_object_get(root, "commit_id"));

    if (!environment || !commit_id) {
        api_error(response, 400, "MISSING_FIELDS", "environment and commit_id are required");
        json_decref(root);
        return;
    }

    // Executar deploy (simplificado - apenas registra)
    char log_file[MAX_PATH];
    char timestamp[32];
    time_t now = time(NULL);
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&now));

    // Criar diretório de logs se não existir
    char log_dir[MAX_PATH];
    snprintf(log_dir, sizeof(log_dir), "%s/.clurg/projects/%s/deploy/%s/logs", clurg_root, project_name, environment);
    // mkdir -p equivalente
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", log_dir);
    system(cmd);

    // Criar log
    snprintf(log_file, sizeof(log_file), "%s/deploy_%s.log", log_dir, timestamp);
    FILE *fp = fopen(log_file, "w");
    if (fp) {
        fprintf(fp, "[START] %s\n", timestamp);
        fprintf(fp, "[ENV] %s\n", environment);
        fprintf(fp, "[COMMIT] %s\n", commit_id);
        fprintf(fp, "[STATUS] SUCCESS\n");
        fprintf(fp, "[MESSAGE] Deploy triggered via API\n");
        fprintf(fp, "[END]\n");
        fclose(fp);
    }

    json_t *data = json_object();
    json_object_set_new(data, "deploy_id", json_string(timestamp));
    json_object_set_new(data, "environment", json_string(environment));
    json_object_set_new(data, "commit_id", json_string(commit_id));
    json_object_set_new(data, "status", json_string("initiated"));

    api_success(response, data);
    json_decref(root);
}

// POST /api/v1/projects - Create new project
void api_projects_create(api_response_t *response, const char *clurg_root, const char *body) {
    // Parse JSON body
    json_error_t error;
    json_t *root = json_loads(body, 0, &error);

    if (!root) {
        api_error(response, 400, "INVALID_JSON", "Invalid JSON in request body");
        return;
    }

    const char *name = json_string_value(json_object_get(root, "name"));
    const char *description = json_string_value(json_object_get(root, "description"));

    if (!name || strlen(name) == 0) {
        api_error(response, 400, "MISSING_NAME", "Project name is required");
        json_decref(root);
        return;
    }

    // Validate project name (no special characters, etc.)
    if (strchr(name, '/') || strchr(name, '\\') || strchr(name, '..')) {
        api_error(response, 400, "INVALID_NAME", "Project name contains invalid characters");
        json_decref(root);
        return;
    }

    // Check if project already exists
    char project_dir[MAX_PATH];
    snprintf(project_dir, sizeof(project_dir), "%s/.clurg/projects/%s", clurg_root, name);

    if (access(project_dir, F_OK) == 0) {
        api_error(response, 409, "PROJECT_EXISTS", "Project already exists");
        json_decref(root);
        return;
    }

    // Create project directory structure
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s/commits\" \"%s/deploy/staging/current\" \"%s/deploy/production/current\" \"%s/ci\"", 
             project_dir, project_dir, project_dir, project_dir);
    
    if (system(cmd) != 0) {
        api_error(response, 500, "CREATE_FAILED", "Failed to create project directories");
        json_decref(root);
        return;
    }

    // Create metadata file
    char meta_file[MAX_PATH];
    snprintf(meta_file, sizeof(meta_file), "%s/metadata.json", project_dir);
    
    FILE *fp = fopen(meta_file, "w");
    if (fp) {
        fprintf(fp, "{\n");
        fprintf(fp, "  \"name\": \"%s\",\n", name);
        fprintf(fp, "  \"description\": \"%s\",\n", description ? description : "");
        fprintf(fp, "  \"created_at\": \"%s\",\n", "2025-12-24T00:00:00Z"); // Would use current time
        fprintf(fp, "  \"commits\": 0\n");
        fprintf(fp, "}\n");
        fclose(fp);
    }

    json_t *data = json_object();
    json_object_set_new(data, "name", json_string(name));
    json_object_set_new(data, "description", json_string(description ? description : ""));
    json_object_set_new(data, "status", json_string("created"));

    api_success(response, data);
    json_decref(root);
}

// DELETE /api/v1/projects/{name} - Delete project
void api_projects_delete(api_response_t *response, const char *clurg_root, const char *project_name) {
    char project_dir[MAX_PATH];
    snprintf(project_dir, sizeof(project_dir), "%s/.clurg/projects/%s", clurg_root, project_name);

    if (access(project_dir, F_OK) != 0) {
        api_error(response, 404, "PROJECT_NOT_FOUND", "Project not found");
        return;
    }

    // Remove project directory (be careful!)
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", project_dir);
    
    if (system(cmd) != 0) {
        api_error(response, 500, "DELETE_FAILED", "Failed to delete project");
        return;
    }

    json_t *data = json_object();
    json_object_set_new(data, "name", json_string(project_name));
    json_object_set_new(data, "status", json_string("deleted"));

    api_success(response, data);
}

// GET /api/v1/system/metrics - System metrics
void api_system_metrics(api_response_t *response, const char *clurg_root) {
    json_t *metrics = json_object();
    
    // Basic metrics
    json_object_set_new(metrics, "total_projects", json_integer(0));
    json_object_set_new(metrics, "total_commits", json_integer(0));
    json_object_set_new(metrics, "active_deploys", json_integer(0));
    
    // Read from metrics file if exists
    char metrics_file[MAX_PATH];
    snprintf(metrics_file, sizeof(metrics_file), "%s/.clurg/metrics.json", clurg_root);
    
    FILE *fp = fopen(metrics_file, "r");
    if (fp) {
        json_error_t error;
        json_t *stored_metrics = json_loadf(fp, 0, &error);
        if (stored_metrics) {
            // Merge stored metrics
            const char *key;
            json_t *value;
            json_object_foreach(stored_metrics, key, value) {
                json_object_set(metrics, key, value);
            }
            json_decref(stored_metrics);
        }
        fclose(fp);
    }
    
    json_t *data = json_object();
    json_object_set_new(data, "metrics", metrics);
    
    api_success(response, data);
}

// GET /api/v1/system/logs - System logs
void api_system_logs(api_response_t *response, const char *clurg_root) {
    json_t *logs = json_array();
    
    // Read recent access logs
    char access_log[MAX_PATH];
    snprintf(access_log, sizeof(access_log), "%s/.clurg/access.log", clurg_root);
    
    FILE *fp = fopen(access_log, "r");
    if (fp) {
        char line[512];
        int count = 0;
        // Read last 50 lines
        while (fgets(line, sizeof(line), fp) && count < 50) {
            line[strcspn(line, "\n")] = 0;
            json_array_append_new(logs, json_string(line));
            count++;
        }
        fclose(fp);
    }
    
    json_t *data = json_object();
    json_object_set_new(data, "logs", logs);
    json_object_set_new(data, "total", json_integer(json_array_size(logs)));
    
    api_success(response, data);
}

// POST /api/v1/projects/{name}/ci/run - Run CI pipeline
void api_project_ci_run(api_response_t *response, const char *clurg_root, const char *project_name) {
    char ci_script[MAX_PATH];
    snprintf(ci_script, sizeof(ci_script), "%s/.clurg/projects/%s/clurg.ci", clurg_root, project_name);
    
    if (access(ci_script, F_OK) != 0) {
        api_error(response, 404, "CI_NOT_CONFIGURED", "CI pipeline not configured for this project");
        return;
    }
    
    // Generate job ID
    char job_id[32];
    time_t now = time(NULL);
    strftime(job_id, sizeof(job_id), "%Y%m%d_%H%M%S", localtime(&now));
    
    // Create job directory
    char job_dir[MAX_PATH];
    snprintf(job_dir, sizeof(job_dir), "%s/.clurg/projects/%s/ci/runs/%s", clurg_root, project_name, job_id);
    
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", job_dir);
    system(cmd);
    
    // Run CI in background (simplified)
    char run_cmd[2048];
    snprintf(run_cmd, sizeof(run_cmd), "cd \"%s/.clurg/projects/%s\" && \"%s/bin/clurg-ci\" run \"%s\" > \"%s/output.log\" 2>&1 &", 
             clurg_root, project_name, clurg_root, ci_script, job_dir);
    
    system(run_cmd);
    
    json_t *data = json_object();
    json_object_set_new(data, "job_id", json_string(job_id));
    json_object_set_new(data, "status", json_string("running"));
    json_object_set_new(data, "project", json_string(project_name));
    
    api_success(response, data);
}

// GET /api/v1/projects/{name}/ci/status - Get CI status
void api_project_ci_status(api_response_t *response, const char *clurg_root, const char *project_name) {
    char status_file[MAX_PATH];
    snprintf(status_file, sizeof(status_file), "%s/.clurg/projects/%s/ci/last_status", clurg_root, project_name);
    
    json_t *data = json_object();
    json_object_set_new(data, "project", json_string(project_name));
    
    FILE *fp = fopen(status_file, "r");
    if (fp) {
        char status[16];
        if (fgets(status, sizeof(status), fp)) {
            status[strcspn(status, "\n")] = 0;
            json_object_set_new(data, "status", json_string(status));
        }
        fclose(fp);
    } else {
        json_object_set_new(data, "status", json_string("unknown"));
    }
    
    api_success(response, data);
}

// Webhook system
typedef struct {
    char url[512];
    char secret[128];
    char events[256]; // comma-separated list of events
} webhook_t;

static webhook_t *webhooks = NULL;
static int webhook_count = 0;

// Load webhooks from config
static void load_webhooks(const char *clurg_root) {
    char config_path[MAX_PATH];
    snprintf(config_path, sizeof(config_path), "%s/.clurg/webhooks.conf", clurg_root);
    
    FILE *fp = fopen(config_path, "r");
    if (!fp) return;
    
    char line[512];
    int count = 0;
    
    // Count webhooks
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "WEBHOOK_URL=")) count++;
    }
    
    if (count == 0) {
        fclose(fp);
        return;
    }
    
    webhooks = malloc(count * sizeof(webhook_t));
    if (!webhooks) {
        fclose(fp);
        return;
    }
    
    rewind(fp);
    int idx = 0;
    
    while (fgets(line, sizeof(line), fp) && idx < count) {
        line[strcspn(line, "\n")] = 0;
        
        if (strstr(line, "WEBHOOK_URL=")) {
            char *url = strchr(line, '=') + 1;
            if (url) {
                // Parse URL:SECRET:EVENTS format
                char *secret = strchr(url, ':');
                if (secret) {
                    *secret = '\0';
                    secret++;
                    
                    char *events = strchr(secret, ':');
                    if (events) {
                        *events = '\0';
                        events++;
                        
                        strncpy(webhooks[idx].url, url, sizeof(webhooks[idx].url) - 1);
                        strncpy(webhooks[idx].secret, secret, sizeof(webhooks[idx].secret) - 1);
                        strncpy(webhooks[idx].events, events, sizeof(webhooks[idx].events) - 1);
                        idx++;
                    }
                }
            }
        }
    }
    
    webhook_count = idx;
    fclose(fp);
}

// Trigger webhook for event
static void trigger_webhook(const char *event, const char *payload) {
    if (!webhooks) return;
    
    for (int i = 0; i < webhook_count; i++) {
        if (strstr(webhooks[i].events, event) || strstr(webhooks[i].events, "*")) {
            // Send webhook in background
            char cmd[2048];
            snprintf(cmd, sizeof(cmd), 
                "curl -X POST -H 'Content-Type: application/json' -H 'X-Clurg-Event: %s' -H 'X-Hub-Signature-256: %s' -d '%s' '%s' >/dev/null 2>&1 &",
                event, webhooks[i].secret, payload, webhooks[i].url);
            system(cmd);
        }
    }
}

// POST /api/v1/webhooks/test - Test webhook
void api_webhooks_test(api_response_t *response, const char *clurg_root, const char *body) {
    // Parse JSON body
    json_error_t error;
    json_t *root = json_loads(body, 0, &error);

    if (!root) {
        api_error(response, 400, "INVALID_JSON", "Invalid JSON in request body");
        return;
    }

    const char *event = json_string_value(json_object_get(root, "event"));
    const char *test_payload = json_string_value(json_object_get(root, "payload"));

    if (!event) {
        api_error(response, 400, "MISSING_EVENT", "event is required");
        json_decref(root);
        return;
    }

    // Load webhooks if not loaded
    if (!webhooks) {
        load_webhooks(clurg_root);
    }

    // Trigger test webhook
    trigger_webhook(event, test_payload ? test_payload : "{}");

    json_t *data = json_object();
    json_object_set_new(data, "event", json_string(event));
    json_object_set_new(data, "status", json_string("triggered"));
    json_object_set_new(data, "webhooks_triggered", json_integer(webhook_count));

    api_success(response, data);
    json_decref(root);
}