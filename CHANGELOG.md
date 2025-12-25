# Changelog

Registro de mudanças do projeto Clurg.

## [0.1.0] - 2025-12-21

### Adicionado

- **clurg-web**: Servidor web para visualização de logs de CI
  - Servidor HTTP implementado do zero em C puro usando sockets
  - Interface web minimalista para visualizar logs de execução de pipelines
  - Listagem de todos os logs disponíveis em `.clurg/ci/logs/`
  - Visualização detalhada de logs individuais
  - HTML/CSS inline simples, sem dependências externas

- **Sistema de CI/CD completo (clurg-ci)**
  - Parser de arquivos `.ci` com DSL própria
  - Executor de steps usando fork/execvp
  - Sistema de workspace isolado (cópia de repositório)
  - Logger estruturado com timestamps
  - Suporte a wildcards em comandos (expansão via glob)

- **Integração CI com commit**
  - Comando `clurg commit` executa pipeline CI automaticamente
  - Logs gravados em `.clurg/ci/logs/`
  - Commit continua mesmo se CI falhar

- **Comando principal (clurg)**
  - Sistema básico de versionamento
  - Comando `commit` com integração CI

- **Pipeline padrão**
  - Arquivo `pipelines/default.ci` com steps: build, test, lint

- **Documentação**
  - README.md com visão geral do projeto
  - CONTEXT.md com filosofia e contexto
  - ESTRUTURA.md explicando organização do repositório
  - docs/architecture.md com arquitetura técnica detalhada
  - docs/ci-design.md com design do sistema CI/CD

- **Organização do repositório**
  - Estrutura de diretórios organizada
  - .gitignore configurado
  - Makefile completo para compilação

### Detalhes Técnicos

- Servidor HTTP: implementação própria usando socket(), bind(), listen(), accept()
- Execução de comandos: fork() + execvp() para isolamento
- Workspace: cópia recursiva de diretórios com mkdtemp()
- Logs: formato simples `[YYYY-MM-DD HH:MM:SS] step-name: OK/FAIL`
- Parsing: parser manual de DSL `.ci` sem dependências externas

### Notas

- Projeto educacional: foco em aprendizado de fundamentos
- Implementação do zero: sem frameworks ou bibliotecas complexas
- Local apenas: tudo funciona na máquina local, sem serviços externos

