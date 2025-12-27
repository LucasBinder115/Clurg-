CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
LDFLAGS = -ljansson

# Diretórios
BIN_DIR = bin
CORE_DIR = core
CI_DIR = ci

# Binários
CLURG = $(BIN_DIR)/clurg
CLURG_CI = $(BIN_DIR)/clurg-ci


# Arquivos fonte
CORE_SOURCES = $(CORE_DIR)/main.c \
               $(CORE_DIR)/commit.c \
               $(CORE_DIR)/push.c \
               $(CORE_DIR)/clone.c \
               $(CORE_DIR)/deploy.c \
               $(CORE_DIR)/init.c
CI_SOURCES = $(CI_DIR)/clurg-ci.c \
             $(CI_DIR)/config.c \
             $(CI_DIR)/executor.c \
             $(CI_DIR)/logger.c \
             $(CI_DIR)/workspace.c \
             $(CI_DIR)/library.c

# Objetos
CORE_OBJECTS = $(CORE_SOURCES:.c=.o)
CI_OBJECTS = $(CI_SOURCES:.c=.o)
CI_LIB_OBJECTS = $(filter-out $(CI_DIR)/clurg-ci.o, $(CI_OBJECTS))  # Excluir main

# Biblioteca CI
CI_LIB = $(BIN_DIR)/libci.a

.PHONY: all clean clurg clurg-ci test

all: $(CLURG) $(CLURG_CI)

# Criar biblioteca CI
$(CI_LIB): $(CI_LIB_OBJECTS) | $(BIN_DIR)
	ar rcs $@ $^

clurg: $(CLURG)

clurg-ci: $(CLURG_CI)

# Compilar clurg
$(CLURG): $(CORE_OBJECTS) $(CI_LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(CORE_OBJECTS) -L$(BIN_DIR) -lci $(LDFLAGS)

# Compilar clurg-ci
$(CLURG_CI): $(CI_OBJECTS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Criar diretório bin se não existir
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Regra genérica para compilar .c em .o
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(CORE_OBJECTS) $(CI_OBJECTS)
	rm -f $(CLURG) $(CLURG_CI)
	rm -f $(BIN_DIR)/libci.a

# Instalação
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

install: clurg
	@echo "Instalando clurg em $(BINDIR)..."
	@install -d $(BINDIR)
	@install $(CLURG) $(BINDIR)
	@echo "Instalação concluída. Tente rodar: clurg --help"

uninstall:
	@echo "Removendo clurg de $(BINDIR)..."
	@rm -f $(BINDIR)/clurg
	@echo "Desinstalação concluída."


test: $(CLURG_CI)
	@echo "Executando pipeline de teste..."
	$(CLURG_CI) run pipelines/default.ci

# Linting com clang-tidy
lint:
	@echo "Executando clang-tidy..."
	@find core ci -name "*.c" -exec clang-tidy {} -- $(CFLAGS) \; 2>/dev/null || true

# Formatação com clang-format
format:
	@echo "Formatando código com clang-format..."
	@find core ci -name "*.c" -exec clang-format -i {} \;
	@find core ci -name "*.h" -exec clang-format -i {} \;

# Verificar formatação (sem modificar arquivos)
format-check:
	@echo "Verificando formatação..."
	@find core ci -name "*.c" -exec clang-format --dry-run --Werror {} \;
	@find core ci -name "*.h" -exec clang-format --dry-run --Werror {} \;

# Testes básicos
test-basic:
	@echo "Executando testes básicos..."
	./tests/run_basic.sh

# Testes abrangentes
test: test-basic
	@echo "Executando testes abrangentes..."
	./tests/run_comprehensive.sh

# Testes de qualidade (lint + format + test)
quality: lint format-check test
	@echo "✓ Verificação de qualidade completa!"

.PHONY: lint format format-check test-basic test quality

