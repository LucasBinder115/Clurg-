# Arquitetura do Clurg

Este documento descreve a arquitetura técnica do sistema Clurg em detalhes.

## Visão Geral

Clurg é composto por três componentes principais, cada um com responsabilidades bem definidas:

1. **clurg** - Sistema de versionamento (núcleo)
2. **clurg-ci** - Executor de pipelines CI/CD
3. **clurg-web** - Interface web para visualização

## Componentes

### 1. clurg (Core)

Localização: `core/`

**Responsabilidades:**
- Gerenciar commits
- Integrar com CI automaticamente
- Ponto de entrada principal do sistema

**Arquivos principais:**
- `main.c` - Ponto de entrada, parsing de argumentos
- `commit.c` - Lógica de commit, integração com CI
- `commit.h` - Interface pública

**Fluxo de commit:**
```
clurg commit "mensagem"
  └─> clurg_commit()
       └─> Executa clurg-ci run
       └─> Registra commit (futuro)
```

### 2. clurg-ci (CI/CD)

Localização: `ci/`

**Responsabilidades:**
- Parsear arquivos de pipeline (.ci)
- Executar steps sequencialmente
- Gerenciar workspaces isolados
- Gerar logs estruturados

**Arquivos principais:**

#### `config.c` - Parser de Pipeline

Parseia arquivos `.ci` com DSL própria:

```
pipeline "nome"
step "step-name" {
  run: "comando"
}
```

**Estruturas de dados:**
```c
typedef struct {
    char name[64];
    char command[256];
} ci_step_t;

typedef struct {
    char name[64];
    ci_step_t steps[MAX_STEPS];
    size_t step_count;
} ci_pipeline_t;
```

#### `executor.c` - Executor de Steps

- Usa `fork()` para criar processos filhos
- Usa `execvp()` para executar comandos
- Expande wildcards usando `glob()`
- Captura exit codes e status

**Fluxo de execução:**
```
executor_run_step()
  └─> fork()
       ├─> filho: chdir(workspace) + execvp()
       └─> pai: waitpid() + captura status
```

#### `workspace.c` - Gerenciamento de Workspaces

- Cria diretórios temporários com `mkdtemp()`
- Copia estado do repositório recursivamente
- Isola execução do pipeline
- Limpa workspaces após execução

#### `logger.c` - Sistema de Logs

- Cria logs estruturados com timestamps
- Formato: `[YYYY-MM-DD HH:MM:SS] step-name: OK/FAIL`
- Salva em `.clurg/ci/logs/`
- Um arquivo de log por execução

### 3. clurg-web (Interface Web)

Localização: `web/`

**Responsabilidades:**
- Servir interface web via HTTP
- Listar logs de CI disponíveis
- Exibir conteúdo de logs específicos
- Prover visualização simples dos resultados

**Implementação:**

Servidor HTTP simples usando:
- `socket()` - Criação de socket TCP
- `bind()` / `listen()` / `accept()` - Servidor básico
- Parsing manual de requisições HTTP
- Respostas HTML simples

**Rotas:**
- `GET /` - Lista de logs
- `GET /logs/<arquivo>` - Visualização de log específico

**Características:**
- Sem dependências externas
- HTML/CSS inline
- Escapamento de HTML para segurança
- Proteção básica contra path traversal

## Fluxo de Dados

### Execução de Pipeline

```
clurg commit "msg"
  └─> clurg_commit()
       └─> system("clurg-ci run")
            └─> clurg-ci main()
                 ├─> config_parse() → pipeline_t
                 ├─> workspace_create() → /tmp/clurg-ci-XXXXXX
                 ├─> workspace_setup() → copia repo
                 ├─> logger_init() → .clurg/ci/logs/
                 ├─> Para cada step:
                 │    └─> executor_run_step()
                 │         └─> fork() + execvp()
                 ├─> workspace_cleanup()
                 └─> logger_cleanup()
```

### Visualização Web

```
clurg-web
  └─> socket() + bind() + listen()
       └─> accept() loop
            └─> handle_request()
                 ├─> Parse HTTP request
                 ├─> Lista logs: opendir(.clurg/ci/logs/)
                 └─> Envia HTML resposta
```

## Estruturas de Dados Principais

### Pipeline

```c
typedef struct {
    char name[MAX_PIPELINE_NAME];
    ci_step_t steps[MAX_STEPS];
    size_t step_count;
} ci_pipeline_t;
```

### Step

```c
typedef struct {
    char name[MAX_STEP_NAME];
    char command[MAX_COMMAND];
} ci_step_t;
```

## Padrões de Design

1. **Separação de Responsabilidades**: Cada módulo tem uma função clara
2. **Composição**: Componentes são compostos, não acoplados
3. **Simplicidade**: Nada de padrões complexos desnecessários
4. **Processos Unix**: Uso direto de syscalls quando apropriado

## Dependências

**Externas:**
- Nenhuma (apenas libc padrão)

**Internas:**
- `ci/` depende apenas de estruturas definidas em `ci.h`
- `web/` é independente
- `core/` depende apenas de suas próprias funções

## Limitações Conhecidas

1. **Workspace**: Cópia simples de arquivos, sem snapshot eficiente
2. **Parser**: Parsing manual simples, não robusto para casos extremos
3. **Web Server**: Servidor single-threaded, bloqueante
4. **Segurança**: Proteções básicas, não adequado para produção sem revisão

## Decisões Arquiteturais

### Por que C puro?

- Aprendizado de fundamentos
- Sem abstrações que escondem complexidade
- Controle total sobre execução

### Por que DSL própria ao invés de YAML?

- Fácil de parsear em C
- Sem dependências externas
- Controle total sobre formato

### Por que servidor HTTP próprio?

- Aprendizado de protocolos
- Sem dependências de frameworks
- Simplicidade máxima

## Extensibilidade

O sistema foi projetado para ser extensível:

- Novos steps podem ser adicionados ao pipeline
- Novos módulos podem ser adicionados sem modificar existentes
- Interface web pode ser estendida com novas rotas
- Workspace pode ser melhorado (snapshots, etc)

