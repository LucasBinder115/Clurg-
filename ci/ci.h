#ifndef CI_H
#define CI_H

#include <stddef.h>

#define MAX_STEPS 64
#define MAX_STEP_NAME 64
#define MAX_COMMAND 256
#define MAX_PIPELINE_NAME 64

typedef struct {
    char name[MAX_STEP_NAME];
    char command[MAX_COMMAND];
} ci_step_t;

typedef struct {
    char name[MAX_PIPELINE_NAME];
    ci_step_t steps[MAX_STEPS];
    size_t step_count;
} ci_pipeline_t;

/* Logger */
int logger_init(const char *log_dir);
void logger_log_step(const char *step_name, int status, int exit_code);
void logger_cleanup(void);

/* Workspace */
int workspace_create(char *workspace_path, size_t path_size);
int workspace_cleanup(const char *workspace_path);
int workspace_setup(const char *workspace_path, const char *repo_path);

/* Config (Parser) */
int config_parse(const char *config_file, ci_pipeline_t *pipeline);
void config_free(ci_pipeline_t *pipeline);

/* Executor */
int executor_run_step(const ci_step_t *step, const char *workspace_path);

#endif /* CI_H */

