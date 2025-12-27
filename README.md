# Clurg

Sistema educacional de versionamento e automação inspirado no Git, feito do zero em C.

---

## O que é o Clurg

Clurg é um **sistema local de controle de versão** com automação integrada, criado como
projeto educacional para compreender, na prática, os fundamentos de:

- Linguagem C
- Processos Unix (fork, exec, wait, signals)
- Filesystem como fonte de verdade
- Ferramentas CLI
- Automação de tarefas e deploy
- Build systems (Makefile)
- Shell scripting

O foco do Clurg **não é competir com Git**, mas entender como sistemas desse tipo
funcionam por dentro.

---

## O que o Clurg NÃO é

- ❌ Não é um clone completo do Git
- ❌ Não é um produto pronto para produção
- ❌ Não depende de servidores externos
- ❌ Não usa frameworks ou abstrações mágicas


Tudo funciona **localmente**, de forma explícita e previsível.

---

## Filosofia do Projeto

- **From scratch** — sem bibliotecas complexas
- **Filesystem-first** — arquivos são a verdade
- **CLI acima de tudo**
- **Código simples, legível e direto**
- **Aprendizado acima de features**

Ferramentas mudam. Fundamentos permanecem.

---

## Arquitetura Geral

clurg (CLI)
│
├── commits imutáveis (snapshots .tar.gz)
├── metadados simples (.meta)
├── automação via shell
└── integração com pipelines locais


Cada comando faz **uma coisa bem feita**, seguindo a tradição Unix.

---

## Estrutura do Repositório

.clurg/
├── commits/
│ ├── HEAD
│ ├── <id>.tar.gz # snapshot imutável
│ └── <id>.meta # mensagem, timestamp
├── logs/
├── plugins/
└── deploy/


---

## Comandos Principais

### Inicializar projeto

```bash
clurg init

Cria a estrutura .clurg/.
Status

clurg status

Mostra estado do repositório local.
Commit

clurg commit "mensagem"

    Cria snapshot completo do projeto

    Executa hooks configurados

    Garante imutabilidade do histórico

Sistema de Deploy (educacional)

O Clurg possui um sistema simples de deploy baseado em snapshots e symlinks.
Ajuda

clurg deploy --help

Status de ambientes

clurg deploy status

Lista ambientes configurados e estado atual.
Deploy completo

clurg deploy run staging

Executa:

    Backup

    Deploy

    Healthcheck

    Switch de symlink

Deploy reprodutível

clurg deploy run staging abc123

Executa deploy exatamente do commit informado.
Rollback

clurg deploy rollback production

Ou rollback específico:

clurg deploy rollback production deploy_20251222_215900

Não depende de banco, CI ou rede.
Lock de ambiente

clurg deploy lock production
clurg deploy unlock production

Usado para incidentes e controle operacional.
Sistema de Plugins

Clurg suporta plugins simples baseados em shell scripts.

.clurg/plugins/meu-plugin/
├── plugin.json
├── plugin.sh
└── hooks/
    └── post-commit

Plugins permitem extensão sem tocar no core.
Compilação
Requisitos

    GCC

    Make

    Sistema Unix-like

Build

make all

Binários gerados:

    bin/clurg

    bin/clurg-ci (opcional / educacional)

### Uso

O binário é gerado em `bin/clurg`. Para usar:

```bash
./bin/clurg <comando>
```

Ou adicione ao seu PATH:

```bash
export PATH=$PATH:$(pwd)/bin
```


Projeto educacional.
Use, estude, modifique e aprenda.

Não recomendado para produção.

    Clurg existe para ensinar.
    O código é simples porque o entendimento precisa ser profundo.



git tag v1.0-educational