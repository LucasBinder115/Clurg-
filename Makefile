CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -I/usr/include/postgresql
LDFLAGS = -ljansson -lpq

# Diretórios
BIN_DIR = bin
CORE_DIR = core
CI_DIR = ci

# Binários
CLURG = $(BIN_DIR)/clurg
CLURG_CI = $(BIN_DIR)/clurg-ci
CLURG_WEB = $(BIN_DIR)/clurg-web

# Diretórios
WEB_DIR = web

# Arquivos fonte
CORE_SOURCES = $(CORE_DIR)/main.c \
               $(CORE_DIR)/commit.c \
               $(CORE_DIR)/push.c \
               $(CORE_DIR)/clone.c \
               $(CORE_DIR)/deploy.c \
               $(CORE_DIR)/db.c
CI_SOURCES = $(CI_DIR)/clurg-ci.c \
             $(CI_DIR)/config.c \
             $(CI_DIR)/executor.c \
             $(CI_DIR)/logger.c \
             $(CI_DIR)/workspace.c
WEB_SOURCES = $(WEB_DIR)/server.c \
              $(WEB_DIR)/api.c

# Objetos
CORE_OBJECTS = $(CORE_SOURCES:.c=.o)
CI_OBJECTS = $(CI_SOURCES:.c=.o)
WEB_OBJECTS = $(WEB_SOURCES:.c=.o)

.PHONY: all clean clurg clurg-ci clurg-web test

all: $(CLURG) $(CLURG_CI) $(CLURG_WEB)

clurg: $(CLURG)

clurg-ci: $(CLURG_CI)

clurg-web: $(CLURG_WEB)

# Compilar clurg
$(CLURG): $(CORE_OBJECTS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compilar clurg-ci
$(CLURG_CI): $(CI_OBJECTS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compilar clurg-web
$(CLURG_WEB): $(WEB_OBJECTS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Criar diretório bin se não existir
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Regra genérica para compilar .c em .o
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(CORE_OBJECTS) $(CI_OBJECTS) $(WEB_OBJECTS)
	rm -f $(CLURG) $(CLURG_CI) $(CLURG_WEB)

test: $(CLURG_CI)
	@echo "Executando pipeline de teste..."
	$(CLURG_CI) run pipelines/default.ci

