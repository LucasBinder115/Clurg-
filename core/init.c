#include "init.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

// Scripts embedded content
const char *SCRIPT_COMMIT = 
"#!/bin/bash\n"
"# Clurg Commit Script\n"
"set -e\n"
"\n"
"MESSAGE=\"${1:-no message}\"\n"
"AUTHOR=\"${USER:-unknown}\"\n"
"TIMESTAMP=$(date +%Y-%m-%d\\ %H:%M:%S)\n"
"ID=$(date +%Y%m%d%H%M%S)\n"
"\n"
"# Structure check\n"
"if [ ! -d \".clurg\" ]; then\n"
"  echo \"Erro: Repositório não inicializado.\"\n"
"  exit 1\n"
"fi\n"
"\n"
"mkdir -p .clurg/commits\n"
"\n"
"echo \"📦 Criando snapshot $ID...\"\n"
"\n"
"# Create tarball\n"
"TAR_FILE=\".clurg/commits/$ID.tar.gz\"\n"
"tar -czf \"$TAR_FILE\" --exclude .clurg .\n"
"\n"
"# Calculate checksum\n"
"CHECKSUM=$(sha256sum \"$TAR_FILE\" | cut -d' ' -f1)\n"
"SIZE=$(stat -c%s \"$TAR_FILE\")\n"
"\n"
"# Create Meta file (Key: Value format compatible with clone.c)\n"
"cat > \".clurg/commits/$ID.meta\" <<EOF\n"
"id: $ID\n"
"timestamp: $TIMESTAMP\n"
"author: $AUTHOR\n"
"message: $MESSAGE\n"
"size_bytes: $SIZE\n"
"checksum: $CHECKSUM\n"
"EOF\n"
"\n"
"# Update HEAD\n"
"echo \"$ID\" > .clurg/HEAD\n"
"\n"
"echo \"✅ Commit $ID realizado com sucesso!\"\n"
"echo \"   Mensagem: $MESSAGE\"\n"
"echo \"   Checksum: $CHECKSUM\"\n";

const char *SCRIPT_STATUS = 
"#!/bin/bash\n"
"# Clurg Status Script\n"
"\n"
"if [ ! -d \".clurg\" ]; then\n"
"    echo \"❌ Repositório não inicializado.\"\n"
"    exit 1\n"
"fi\n"
"\n"
"if [ -f \".clurg/HEAD\" ]; then\n"
"    HEAD=$(cat .clurg/HEAD)\n"
"    if [ -z \"$HEAD\" ]; then\n"
"        echo \"📂 Repositório inicializado (sem commits).\"\n"
"    else\n"
"        echo \"🔖 HEAD atual: $HEAD\"\n"
"        echo \"📂 Working directory: $(pwd)\"\n"
"    fi\n"
"else\n"
"    echo \"⚠️  HEAD não encontrado (estado inconsistente).\"\n"
"fi\n";

const char *SCRIPT_LOG = 
"#!/bin/bash\n"
"# Clurg Log Script\n"
"\n"
"if [ ! -d \".clurg/commits\" ]; then\n"
"    echo \"❌ Sem commits encontrados.\"\n"
"    exit 0\n"
"fi\n"
"\n"
"echo \"📜 Histórico de Commits:\"\n"
"echo \"========================\"\n"
"\n"
"# Listar .meta files, ordenados reverso (mais novo primeiro)\n"
"ls -1r .clurg/commits/*.meta 2>/dev/null | while read metafile; do\n"
"    id=$(grep \"^id: \" \"$metafile\" | cut -d: -f2- | xargs)\n"
"    date=$(grep \"^timestamp: \" \"$metafile\" | cut -d: -f2- | xargs)\n"
"    author=$(grep \"^author: \" \"$metafile\" | cut -d: -f2- | xargs)\n"
"    msg=$(grep \"^message: \" \"$metafile\" | cut -d: -f2- | xargs)\n"
"    \n"
"    echo \"Commit: $id\"\n"
"    echo \"Data:   $date\"\n"
"    echo \"Autor:  $author\"\n"
"    echo \"    $msg\"\n"
"    echo \"\"\n"
"done\n";

static int write_script(const char *path, const char *content) {
    FILE *fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "Erro ao criar script: %s\n", path);
        return -1;
    }
    fputs(content, fp);
    fclose(fp);
    if (chmod(path, 0755) != 0) {
        fprintf(stderr, "Erro ao dar permissão de execução: %s\n", path);
        return -1;
    }
    return 0;
}

int clurg_init(void) {
    char *dirs[] = {
        ".clurg",
        ".clurg/commits",
        ".clurg/logs",
        ".clurg/deploy",
        ".clurg/plugins",
        ".clurg/scripts"
    };

    printf("Iniciando repositório Clurg...\n");

    for (int i = 0; i < 6; i++) {
        if (mkdir(dirs[i], 0755) != 0) {
            if (errno == EEXIST) {
                if (i == 0) printf("Aviso: repositório .clurg já existe.\n");
            } else {
                fprintf(stderr, "Erro ao criar diretório %s: %s\n", dirs[i], strerror(errno));
                return 1;
            }
        } else {
            printf("Criado diretório: %s\n", dirs[i]);
        }
    }

    // Criar HEAD se não existir
    if (access(".clurg/HEAD", F_OK) != 0) {
        FILE *fp = fopen(".clurg/HEAD", "w");
        if (!fp) {
             fprintf(stderr, "Erro ao criar .clurg/HEAD\n");
             return 1;
        }
        fclose(fp);
        printf("Criado arquivo: .clurg/HEAD\n");
    }
    
    // Escrever scripts
    write_script(".clurg/scripts/commit.sh", SCRIPT_COMMIT);
    write_script(".clurg/scripts/status.sh", SCRIPT_STATUS);
    write_script(".clurg/scripts/log.sh", SCRIPT_LOG);
    printf("Scripts internos instalados em .clurg/scripts/\n");

    printf("Repositório Clurg inicializado com sucesso!\n");
    return 0;
}
