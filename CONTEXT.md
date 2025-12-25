🧠 VISÃO — O QUE VOCÊ VAI CONSTRUIR

Você não vai substituir o GitHub Actions.
Você vai criar um motor mínimo de automação, acoplável ao Clurg.

Vamos chamar isso (nome provisório):

Clurg Runner — o braço executor do Clurg

Ele será:

Local

Determinístico

Scriptável

Observável

Simples o suficiente para entender

Forte o suficiente para escalar mentalmente

🧱 ARQUITETURA CONCEITUAL (SEM MÁGICA)
┌────────────┐
│  clurg     │   (commit)
└─────┬──────┘
      │
      ▼
┌──────────────┐
│ clurg-ci     │   (orquestrador)
├──────────────┤
│ parser       │
│ executor     │
│ logger       │
│ workspace    │
└─────┬────────┘
      │
      ▼
┌──────────────┐
│ steps        │   (scripts)
│ build/test   │
└─────┬────────┘
      │
      ▼
┌──────────────┐
│ clurg-web    │   (visualização)
├──────────────┤
│ servidor HTTP│   (sockets)
│ visualização │   (logs de CI)
└──────────────┘


Nada distribuído por enquanto.
Primeiro, verdade funcional.

📁 NOVA ESTRUTURA DE DIRETÓRIOS

Estrutura final do projeto:

clurg/
├── core/
├── ci/
│   ├── clurg-ci.c        # runner principal
│   ├── config.c          # parsing do pipeline
│   ├── executor.c        # execução de etapas
│   ├── workspace.c       # diretórios temporários
│   ├── logger.c          # logs estruturados
│   └── ci.h
├── web/
│   └── server.c          # servidor HTTP em C puro
├── pipelines/
│   └── default.ci
├── bin/
│   ├── clurg
│   ├── clurg-ci
│   └── clurg-web

📜 PIPELINE COMO CONTRATO (SEM YAML)

Nada de YAML mágico.
Vamos usar formato próprio, legível, antigo e robusto.

Exemplo: pipelines/default.ci

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


Por quê?

Fácil de parsear em C

Sem dependência

Controlado por você

⚙️ COMPORTAMENTO DO clurg-ci
Execução manual
clurg-ci run

Execução automática (gancho de commit)

Quando:

clurg commit


Então:

clurg-ci run default.ci


Regra de ouro:

Falhou um step → pipeline falha

Status final gravado em .clurg/ci/logs/

🧠 COMPONENTES INTERNOS (TODO LIST PARA VOCÊ / AGENTES)
1️⃣ Parser de pipeline (config.c)

Ler arquivo .ci

Identificar:

nome do pipeline

steps

comandos

Saída interna:

typedef struct {
  char name[64];
  char command[256];
} ci_step_t;

2️⃣ Executor (executor.c)

Criar processo filho (fork)

Executar comando (execvp)

Capturar exit code

Redirecionar stdout/stderr

Sem shell mágico.
Execução explícita.

3️⃣ Workspace (workspace.c)

Criar diretório temporário

Copiar estado do repo

Rodar pipeline isolado

Aprendizado real:

mkdir

chdir

realpath

4️⃣ Logger (logger.c)

Log por step

Timestamp

Status (OK / FAIL)

Formato simples:

[2025-01-12 20:14:03] build: OK
[2025-01-12 20:14:05] test: FAIL (exit 1)

5️⃣ Servidor Web (web/server.c)

Servidor HTTP simples em C puro

Sockets TCP/IP

Sem dependências externas

Visualização de logs de CI

Interface HTML minimalista

🔗 INTEGRAÇÃO COM O CLURG

No commit.c:

int clurg_commit(...) {
  // commit normal
  system("clurg-ci run default.ci");
}


Mais tarde você refina isso (sem system()).

🧪 TESTE DE SUCESSO (MVP CI)

Clurg está com CI próprio quando:

clurg commit roda pipeline

Build falha → commit continua, mas CI registra erro

Logs ficam acessíveis

Nada depende de GitHub, Docker ou YAML

🧠 ISSO TE ENSINA O QUÊ?

Processos Unix

Orquestração

Parsing de DSL

Design de ferramentas

Como GitHub Actions funciona por dentro

Você não está usando a ferramenta.
Você está aprendendo a forjá-la.
Ferramentas mudam. Fundamentos permanecem." — filosofia do projeto Clurg

Atualizado. Context.md agora reflete 100% a realidade que você escolheu:
tudo from scratch, sem muleta, sem SaaS, sem “actions prontas”.

Você fez algo importante aqui — não é só técnico, é filosófico.

🧠 O QUE MUDOU (EM TERMOS DE ARQUITETURA)

Antes:

CI/CD como integração externa

Dependência implícita de GitHub Actions

Agora:

CI é parte do sistema

clurg-ci vira um binário de primeira classe

Pipeline é DSL própria

Execução local, previsível, auditável

Logs como artefato histórico (igual commits)

Interface web própria (clurg-web) para visualização

Servidor HTTP implementado do zero em C

Isso é como os sistemas eram feitos antes da nuvem — e por isso mesmo, você aprende o que importa.

📜 CONTEXT.md AGORA SERVE PARA:

🧭 Orientar qualquer agente de IA

🧱 Impedir decisões fora da arquitetura

🔒 Garantir que ninguém “resolva fácil demais”

📚 Registrar a visão original do projeto

Se alguém (humano ou IA) violar isso, está errado.
Documento manda mais que executor.

📋 COMO VOCÊ VAI TRABALHAR DAQUI PRA FRENTE

Fluxo recomendado (prático, testado):

Escolhe um bloco do CONTEXT.md

Ex: 6.4 PIPELINE (DSL PRÓPRIA)

Chama um agente e diz:

“Implemente APENAS isso, seguindo o CONTEXT.md”

Você:

Compila

Testa

Entende

Atualiza:

CHANGELOG.md

Isso cria cadência, evita caos e te mantém no controle.