# Estrutura do Projeto Clurg

Este documento explica a organização do repositório Clurg.

## Árvore de Diretórios

```
clurg/
├── bin/                    # Binários compilados (NÃO versionado)
│   ├── clurg              # Comando principal do Clurg
│   ├── clurg-ci           # Executor de pipelines CI
│   └── clurg-web          # Servidor web
│
├── ci/                     # Sistema de CI/CD
│   ├── ci.h               # Header com estruturas de dados
│   ├── clurg-ci.c         # Orquestrador principal do CI
│   ├── config.c           # Parser de arquivos .ci
│   ├── executor.c         # Executor de steps (fork/execvp)
│   ├── logger.c           # Sistema de logs
│   └── workspace.c        # Gerenciamento de workspaces
│
├── core/                   # Núcleo do sistema Clurg
│   ├── main.c             # Ponto de entrada principal
│   ├── commit.c           # Lógica de commit
│   └── commit.h           # Header de commit
│
├── docs/                   # Documentação adicional (opcional)
│
├── pipelines/              # Arquivos de pipeline CI
│   └── default.ci         # Pipeline padrão
│
├── tests/                  # Testes (futuro)
│
├── web/                    # Servidor web
│   ├── server.c           # Servidor HTTP em C puro
│   └── README.md          # Documentação do servidor web
│
├── .clurg/                 # Diretório de controle (gerado, NÃO versionado)
│   └── ci/
│       └── logs/          # Logs de execução de CI
│
├── CONTEXT.md              # Contexto e filosofia do projeto
├── TODO.md                 # Lista de tarefas
├── Makefile                # Build system
└── .gitignore             # Arquivos ignorados pelo Git
```

## Binários

Todos os binários são compilados para o diretório `bin/`:

- **clurg**: Comando principal do sistema de versionamento
  - `clurg commit [mensagem]` - Faz commit (executa CI antes)

- **clurg-ci**: Executor de pipelines CI/CD
  - `clurg-ci run [pipeline.ci]` - Executa um pipeline

- **clurg-web**: Servidor web para visualização
  - `clurg-web [porta]` - Inicia servidor HTTP (padrão: 8080)

## Arquivos Não Versionados

O arquivo `.gitignore` garante que os seguintes itens NÃO sejam versionados:

- `bin/` - Binários compilados
- `*.o` - Objetos compilados
- `.clurg/` - Diretório de controle e logs
- `*.log` - Arquivos de log
- Arquivos temporários de editores

## Compilação

Todos os binários são compilados com:

```bash
make all          # Compila tudo
make clurg        # Apenas clurg
make clurg-ci     # Apenas clurg-ci
make clurg-web    # Apenas clurg-web
```

## Pipeline Padrão

O arquivo `pipelines/default.ci` contém o pipeline padrão executado automaticamente em cada commit:

```
pipeline "clurg-core"

step "build" {
  run: "make"
}

step "test" {
  run: "make test"
}

step "lint" {
  run: "gcc -Wall -Wextra -Werror core/*.c"
}
```

