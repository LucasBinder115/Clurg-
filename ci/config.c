#include "ci.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void skip_whitespace(FILE *f) {
    int c;
    while ((c = fgetc(f)) != EOF && isspace(c)) {
        /* skip todos os espaços, incluindo newlines */
    }
    if (c != EOF) {
        ungetc(c, f);
    }
}

static int read_word(FILE *f, const char *word) {
    const char *p = word;
    int c;
    
    while (*p) {
        c = fgetc(f);
        if (c != *p) {
            /* Restaurar caracteres lidos */
            ungetc(c, f);
            while (p > word) {
                ungetc(*--p, f);
            }
            return -1;
        }
        p++;
    }
    
    return 0;
}

static int read_quoted_string(FILE *f, char *buf, size_t buf_size) {
    int c = fgetc(f);
    if (c != '"') {
        return -1;
    }
    
    size_t i = 0;
    while ((c = fgetc(f)) != EOF && c != '"' && i < buf_size - 1) {
        buf[i++] = c;
    }
    buf[i] = '\0';
    
    if (c != '"') {
        return -1; /* String não fechada */
    }
    
    return 0;
}

static int parse_step(FILE *f, ci_step_t *step) {
    int c;
    
    skip_whitespace(f);
    
    /* Ler "step" */
    if (read_word(f, "step") != 0) {
        return -1;
    }
    
    skip_whitespace(f);
    
    /* Ler nome do step entre aspas */
    if (read_quoted_string(f, step->name, MAX_STEP_NAME) != 0) {
        return -1;
    }
    
    skip_whitespace(f);
    
    /* Ler "{" */
    c = fgetc(f);
    if (c != '{') {
        return -1;
    }
    
    skip_whitespace(f);
    
    /* Ler "run:" */
    if (read_word(f, "run:") != 0) {
        return -1;
    }
    
    skip_whitespace(f);
    
    /* Ler comando entre aspas */
    if (read_quoted_string(f, step->command, MAX_COMMAND) != 0) {
        return -1;
    }
    
    skip_whitespace(f);
    
    /* Ler "}" */
    c = fgetc(f);
    if (c != '}') {
        return -1;
    }
    
    return 0;
}

int config_parse(const char *config_file, ci_pipeline_t *pipeline) {
    FILE *f;
    int c;
    
    /* Inicializar pipeline */
    memset(pipeline, 0, sizeof(ci_pipeline_t));
    pipeline->step_count = 0;
    
    f = fopen(config_file, "r");
    if (!f) {
        perror("fopen config_file");
        return -1;
    }
    
    skip_whitespace(f);
    
    /* Ler "pipeline" */
    if (read_word(f, "pipeline") != 0) {
        fprintf(stderr, "esperado 'pipeline' no início do arquivo\n");
        fclose(f);
        return -1;
    }
    
    skip_whitespace(f);
    
    /* Ler nome do pipeline entre aspas */
    if (read_quoted_string(f, pipeline->name, MAX_PIPELINE_NAME) != 0) {
        fprintf(stderr, "erro ao ler nome do pipeline\n");
        fclose(f);
        return -1;
    }
    
    /* Ler steps até o final do arquivo */
    skip_whitespace(f);
    
    while (pipeline->step_count < MAX_STEPS) {
        /* Verificar se há mais conteúdo */
        c = fgetc(f);
        if (c == EOF) {
            break;
        }
        ungetc(c, f);
        
        skip_whitespace(f);
        
        /* Verificar novamente se chegou ao fim */
        c = fgetc(f);
        if (c == EOF) {
            break;
        }
        ungetc(c, f);
        
        /* Tentar ler um step */
        if (parse_step(f, &pipeline->steps[pipeline->step_count]) == 0) {
            pipeline->step_count++;
        } else {
            /* Erro ao parsear ou fim do arquivo */
            break;
        }
    }
    
    fclose(f);
    return 0;
}

void config_free(ci_pipeline_t *pipeline) {
    /* Por enquanto não há memória alocada dinamicamente */
    /* Esta função existe para manter a interface consistente */
    (void)pipeline;
}

