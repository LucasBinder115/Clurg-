# Design do Sistema CI/CD do Clurg

Este documento descreve o design e as decisões por trás do sistema de CI/CD do Clurg.

## Visão Geral

O sistema CI/CD do Clurg (clurg-ci) é um executor de pipelines local e determinístico. Foi projetado para ser:

- **Simples**: Fácil de entender e modificar
- **Local**: Tudo roda na máquina local
- **Determinístico**: Mesmas entradas produzem mesmas saídas
- **Observável**: Logs claros de cada execução

## Filosofia de Design

### 1. Pipeline como Contrato

O arquivo `.ci` é um contrato claro:
- Define o que deve ser executado
- Em que ordem
- Com que comandos

Não há variáveis de ambiente mágicas, configuração implícita ou dependências ocultas.

### 2. Execução Explícita

Cada step é executado explicitamente:
- Processo separado (fork)
- Comando explícito (execvp)
- Exit code capturado
- Sem shell mágico

### 3. Isolamento via Workspace

Cada execução de pipeline:
- Copia o repositório para um diretório temporário
- Executa no workspace isolado
- Não modifica o repositório original
- Limpa após execução

## Componentes

### Parser (config.c)

**Responsabilidade**: Transformar arquivo `.ci` em estrutura de dados.

**DSL:**
```
pipeline "nome-do-pipeline"

step "nome-do-step" {
  run: "comando a executar"
}
```

**Decisões:**
- Formato simples, fácil de parsear
- Sem variáveis ou templates (por enquanto)
- Comandos são strings simples entre aspas
- Ordem dos steps é sequencial no arquivo

**Limitações conhecidas:**
- Não suporta comentários (ainda)
- Não suporta variáveis
- Parsing não é muito robusto a erros de sintaxe

### Executor (executor.c)

**Responsabilidade**: Executar comandos de forma isolada e controlada.

**Características:**

1. **Expansão de Wildcards**
   - Usa `glob()` para expandir `*`, `?`, `[ ]`
   - Expansão acontece antes de `execvp()`
   - Permite comandos como `gcc core/*.c`

2. **Isolamento**
   - Cada step roda em processo separado
   - Workspace isolado (chdir)
   - Exit code capturado

3. **Sem Shell**
   - Não usa `/bin/sh` ou similar
   - Parsing manual de comandos
   - Execução direta via `execvp()`

**Fluxo:**
```
executor_run_step(step, workspace)
  ├─> split_command() → argv[]
  ├─> expand_argv_wildcards() → expande wildcards
  ├─> fork()
  │   ├─> filho: chdir(workspace) + execvp()
  │   └─> pai: waitpid() + captura status
  └─> retorna exit_code
```

### Workspace (workspace.c)

**Responsabilidade**: Criar e gerenciar ambientes isolados para execução.

**Fluxo:**
```
workspace_create() → /tmp/clurg-ci-XXXXXX
workspace_setup(repo_path) → copia arquivos
  └─> copy_dir_recursive()
       ├─> Ignora .clurg, .git
       └─> Copia resto recursivamente
```

**Decisões:**
- Diretório temporário com `mkdtemp()`
- Cópia completa do repositório
- Ignora apenas diretórios de controle
- Limpeza após uso

**Melhorias futuras possíveis:**
- Snapshots mais eficientes
- Compartilhamento de base comum
- Cache de dependências

### Logger (logger.c)

**Responsabilidade**: Registrar execução de steps de forma estruturada.

**Formato de log:**
```
[YYYY-MM-DD HH:MM:SS] step-name: OK
[YYYY-MM-DD HH:MM:SS] step-name: FAIL (exit N)
```

**Características:**
- Um arquivo por execução
- Timestamp em cada linha
- Status claro (OK/FAIL)
- Exit code quando falha

**Localização:**
- `.clurg/ci/logs/ci_YYYYMMDD_HHMMSS.log`

**Decisões:**
- Formato legível por humanos
- Fácil de parsear
- Um arquivo por execução (simples)
- Timestamp no nome do arquivo

## Fluxo de Execução

### Execução Manual

```
clurg-ci run pipelines/default.ci
  └─> Parse pipeline
  └─> Criar workspace
  └─> Para cada step (sequencial):
       ├─> Executar step
       ├─> Log resultado
       └─> Se falhou: parar pipeline
  └─> Limpar workspace
  └─> Retornar status final
```

### Execução Automática (via commit)

```
clurg commit "mensagem"
  └─> clurg_commit()
       └─> system("clurg-ci run pipelines/default.ci")
            └─> (mesmo fluxo acima)
       └─> Commit continua mesmo se CI falhar
```

**Regra de ouro**: Falhou um step → pipeline falha (para no primeiro erro)

## Decisões de Design

### Por que sequencial?

- Simples de entender
- Fácil de depurar
- Ordem determinística
- Paralelização pode vir depois

### Por que workspace isolado?

- Não modifica repositório original
- Execução determinística
- Fácil de limpar
- Isolamento de efeitos colaterais

### Por que logs simples?

- Fácil de ler
- Fácil de parsear
- Sem dependências
- Suficiente para começar

### Por que DSL própria?

- Sem dependências (YAML, JSON parsers)
- Controle total sobre formato
- Fácil de parsear em C
- Pode evoluir conforme necessário

## Limitações e Trade-offs

### Limitações Atuais

1. **Sem paralelização**: Steps executam sequencialmente
2. **Sem cache**: Cada execução é do zero
3. **Parsing simples**: Não é muito robusto a erros
4. **Workspace custoso**: Cópia completa do repo

### Trade-offs Aceitos

- **Simplicidade > Performance**: Código fácil de entender
- **Local > Distribuído**: Funciona sem infraestrutura
- **Clareza > Funcionalidades**: Menos features, mais compreensível

## Extensibilidade

O design permite extensões futuras:

1. **Novos tipos de steps**: Pode adicionar condicionais, loops, etc
2. **Paralelização**: Steps podem ser marcados como paralelos
3. **Cache**: Workspace pode ser melhorado com snapshots
4. **Variáveis**: DSL pode suportar variáveis e templates
5. **Notificações**: Logger pode enviar notificações

Mas tudo mantendo a simplicidade e clareza como prioridades.

## Comparação com Outras Ferramentas

### GitHub Actions

**Semelhante:**
- Pipeline como código
- Steps sequenciais
- Workspace isolado (virtualmente)

**Diferente:**
- Local vs cloud
- DSL própria vs YAML
- Simples vs complexo

### Make

**Semelhante:**
- Execução de comandos
- Dependências entre targets

**Diferente:**
- Pipeline explícito vs regras implícitas
- Workspace isolado vs diretório atual
- Logs estruturados vs stdout

## Conclusão

O design do CI/CD do Clurg prioriza:
1. **Aprendizado**: Entender como CI/CD funciona por dentro
2. **Simplicidade**: Código que pode ser lido e entendido
3. **Controle**: Sem caixas pretas ou mágica

É um sistema educacional que demonstra os princípios fundamentais de automação e orquestração.

