# Clurg

Sistema educacional de versionamento e CI/CD feito do zero em C.

## O que é o Clurg

Clurg é um sistema minimalista de controle de versão com CI/CD integrado, implementado completamente do zero em C puro. Foi criado como projeto educacional para entender os fundamentos de sistemas de versionamento, automação de builds e orquestração de processos.

O projeto demonstra:

- **Processos Unix**: fork, exec, wait, signals
- **Orquestração**: pipelines, execução sequencial de tarefas
- **Parsing de DSL**: interpretação de configuração própria
- **Design de ferramentas**: como construir ferramentas CLI úteis
- **Servidores web simples**: HTTP do zero usando sockets

## O que o Clurg NÃO é

- ❌ **Clone do Git** - Não implementa todas as funcionalidades do Git
- ❌ **Substituto do GitHub Actions** - É um sistema educacional, não um produto
- ❌ **Ferramenta de produção** - Foi feito para aprendizado, não para uso em produção
- ❌ **Sistema distribuído** - Tudo funciona localmente

## Arquitetura em Alto Nível

```
┌────────────┐
│  clurg     │   (sistema de versionamento)
│  commit    │
└─────┬──────┘
      │
      ▼
┌──────────────┐
│ clurg-ci     │   (executor de pipelines)
│              │
│ - parser     │   (lê arquivos .ci)
│ - executor   │   (executa steps)
│ - logger     │   (registra resultados)
│ - workspace  │   (isolamento)
└─────┬────────┘
      │
      ▼
┌──────────────┐
│ clurg-web    │   (interface web)
│              │
│ - servidor   │   (HTTP simples)
│ - visualização│  (logs de CI)
└──────────────┘
```

### Componentes Principais

1. **clurg** - Comando principal
   - Gerencia commits
   - Integra com CI automaticamente

2. **clurg-ci** - Executor de pipelines
   - Lê arquivos `.ci` (DSL própria)
   - Executa steps sequencialmente
   - Cria workspaces isolados
   - Gera logs estruturados

3. **clurg-web** - Servidor web
   - HTTP simples em C puro
   - Visualiza logs de CI
   - Interface minimalista

## Compilação

### Pré-requisitos

- GCC (compilador C)
- Make
- Sistema Unix-like (Linux, macOS, etc)

### Compilar Tudo

```bash
make all
```

Isso compila três binários:
- `bin/clurg` - Comando principal
- `bin/clurg-ci` - Executor de CI
- `bin/clurg-web` - Servidor web

### Compilar Individualmente

```bash
make clurg      # Apenas o comando principal
make clurg-ci   # Apenas o executor de CI
make clurg-web  # Apenas o servidor web
```

### Limpar

```bash
make clean
```

## Como Rodar

### clurg

Comando principal do sistema de versionamento:

```bash
# Fazer commit (executa CI automaticamente antes)
./bin/clurg commit "mensagem do commit"

# Enviar commit para repositório remoto
./bin/clurg push http://servidor-remoto:8080/api/push

# Gerenciar plugins
./bin/clurg plugin list                    # Listar plugins instalados
./bin/clurg plugin install <url>           # Instalar plugin do Git
./bin/clurg plugin run <plugin> [args...]  # Executar plugin específico
```

O comando `commit` automaticamente executa o pipeline CI antes de completar o commit. O `push` envia o último commit para um servidor remoto via HTTP. O sistema de plugins permite extensibilidade sem modificar o core.

### clurg-ci

Executor de pipelines CI/CD:

```bash
# Executar pipeline padrão
./bin/clurg-ci run

# Executar pipeline específico
./bin/clurg-ci run pipelines/default.ci
```

O pipeline é definido em arquivos `.ci` com uma DSL simples:

```
pipeline "nome-do-pipeline"

step "build" {
  run: "make"
}

step "test" {
  run: "make test"
}
```

### clurg-web

Servidor web para visualização:

```bash
# Iniciar servidor na porta padrão (8080)
./bin/clurg-web

# Especificar porta diferente
./bin/clurg-web 3000
```

Depois acesse no navegador: `http://localhost:8080`

O servidor mostra:
- Lista de logs de CI
- Conteúdo completo de cada log
- Status de execução dos pipelines

## Garantias do Sistema

### Imutabilidade de Commits

Todos os commits são armazenados como arquivos `.tar.gz` compactados, que são **imutáveis por natureza**. Uma vez criado, um commit não pode ser alterado. Isso garante:

- Integridade histórica
- Confiança nos backups
- Simplicidade de implementação

### Estrutura .clurg/

```
.clurg/
├── commits/        # Snapshots imutáveis (.tar.gz + .meta)
│   ├── HEAD        # Ponteiro para último commit
│   ├── <id>.tar.gz # Snapshot completo
│   └── <id>.meta   # Metadados (mensagem, timestamp, id)
├── ci/
│   └── logs/       # Logs de execução CI
└── remote/         # Commits recebidos via push (opcional)
```

### Permissões Recomendadas

Para segurança básica:

```bash
# Tornar scripts executáveis
chmod +x scripts/commit.sh

# Proteger diretório de controle
chmod 755 .clurg/
chmod 644 .clurg/commits/*.tar.gz
chmod 644 .clurg/commits/*.meta
```

O sistema segue o princípio Unix de "small sharp tools": cada componente faz uma coisa bem e se integra com outros.

## Filosofia

Este projeto segue uma filosofia específica:

- **From scratch**: Tudo feito do zero, sem dependências externas complexas
- **Educacional**: Foco em aprendizado, não em funcionalidades completas
- **Simples mas correto**: Código legível e bem estruturado
- **Sem mágica**: Nada de frameworks ou bibliotecas que escondem complexidade

Ferramentas mudam. Fundamentos permanecem.

## Sistema de Plugins

Clurg suporta extensibilidade através de um sistema de plugins modular. Plugins podem:

- Executar hooks em diferentes pontos do ciclo de vida (post-commit, etc.)
- Adicionar novos comandos
- Estender funcionalidades sem modificar o core

### Estrutura de Plugin

```
.clurg/plugins/meu-plugin/
├── plugin.json      # Metadados do plugin
├── plugin.sh        # Script principal (opcional)
└── hooks/           # Hooks executados automaticamente
    └── post-commit  # Executado após cada commit
```

### Plugin de Exemplo: Auto-Backup

O plugin `auto-backup` demonstra o sistema criando backups automáticos após cada commit:

```bash
# Instalar plugin
./bin/clurg plugin install https://github.com/exemplo/auto-backup

# Executar comandos do plugin
./bin/clurg plugin run auto-backup list    # Listar backups
./bin/clurg plugin run auto-backup backup  # Backup manual
```

## Documentação Adicional

- `CONTEXT.md` - Contexto e filosofia do projeto
- `ESTRUTURA.md` - Estrutura detalhada do repositório
- `TODO.md` - Lista de tarefas e roadmap
- `docs/` - Documentação técnica detalhada

## Licença

Este é um projeto educacional. Use como quiser para aprendizado.

## Contribuições

Este projeto foi criado para fins educacionais. Sinta-se livre para estudar, modificar e aprender com o código.

---

**Nota**: Clurg é um projeto educacional. Não use em produção sem revisar cuidadosamente questões de segurança e robustez.

# Teste hook expandido
# FASE 5 concluída - automação funcionando
# Correção do hook
