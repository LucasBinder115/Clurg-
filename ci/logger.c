#include "ci.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>

static FILE *log_file = NULL;
static char log_file_path[512];

int logger_init(const char *log_dir) {
    struct stat st = {0};

    /* Criar diretório de logs recursivamente (mkdir -p) */
    if (stat(log_dir, &st) == -1) {
        char tmp[512];
        char *p;

        strncpy(tmp, log_dir, sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = '\0';

        for (p = tmp + 1; *p; p++) {
            if (*p == '/') {
                *p = '\0';
                if (mkdir(tmp, 0755) != 0) {
                    if (errno != EEXIST) {
                        perror("mkdir log_dir");
                        return -1;
                    }
                }
                *p = '/';
            }
        }
        /* criar o último componente */
        if (mkdir(tmp, 0755) != 0) {
            if (errno != EEXIST) {
                perror("mkdir log_dir");
                return -1;
            }
        }
    }
    
    /* Gerar nome do arquivo de log com timestamp */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    snprintf(log_file_path, sizeof(log_file_path), 
             "%s/ci_%04d%02d%02d_%02d%02d%02d.log",
             log_dir,
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec);
    
    log_file = fopen(log_file_path, "w");
    if (!log_file) {
        perror("fopen log_file");
        return -1;
    }
    
    return 0;
}

void logger_log_step(const char *step_name, int status, int exit_code) {
    if (!log_file) return;
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    /* Formato: [2025-01-12 20:14:03] build: OK */
    /* ou: [2025-01-12 20:14:05] test: FAIL (exit 1) */
    
    if (status == 0) {
        fprintf(log_file, "[%04d-%02d-%02d %02d:%02d:%02d] %s: OK\n",
                tm_info->tm_year + 1900,
                tm_info->tm_mon + 1,
                tm_info->tm_mday,
                tm_info->tm_hour,
                tm_info->tm_min,
                tm_info->tm_sec,
                step_name);
        /* Também imprimir no stdout */
        printf("[%04d-%02d-%02d %02d:%02d:%02d] %s: OK\n",
               tm_info->tm_year + 1900,
               tm_info->tm_mon + 1,
               tm_info->tm_mday,
               tm_info->tm_hour,
               tm_info->tm_min,
               tm_info->tm_sec,
               step_name);
    } else {
        fprintf(log_file, "[%04d-%02d-%02d %02d:%02d:%02d] %s: FAIL (exit %d)\n",
                tm_info->tm_year + 1900,
                tm_info->tm_mon + 1,
                tm_info->tm_mday,
                tm_info->tm_hour,
                tm_info->tm_min,
                tm_info->tm_sec,
                step_name,
                exit_code);
        /* Também imprimir no stderr */
        fprintf(stderr, "[%04d-%02d-%02d %02d:%02d:%02d] %s: FAIL (exit %d)\n",
                tm_info->tm_year + 1900,
                tm_info->tm_mon + 1,
                tm_info->tm_mday,
                tm_info->tm_hour,
                tm_info->tm_min,
                tm_info->tm_sec,
                step_name,
                exit_code);
    }
    
    fflush(log_file);
}

void logger_cleanup(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

