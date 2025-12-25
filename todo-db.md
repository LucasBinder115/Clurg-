# Clurg — Database PostgreSQL (todo-db.md)

> *Persistência forte, dados vivos e base para crescer sem medo.*
> PostgreSQL como cérebro, filesystem como memória eterna.

---

## Visão Filosófica (importante antes de codar)

No Clurg:

* **Filesystem é a verdade** (commits, snapshots, logs)
* **PostgreSQL é o índice inteligente**
* O banco **nunca substitui arquivos**
* O banco **acelera leitura, consulta e visão**

Essa separação é o que evita arrependimento no futuro.

---

## Por que PostgreSQL (decisão consciente)

Escolha deliberada, não modinha:

* Escala real (quando quiser)
* ACID de verdade
* Excelente suporte em C (`libpq`)
* Forte para métricas, histórico e API
* Aprendizado transferível para mercado

SQLite seria suficiente — mas **Postgres te ensina mais**.

---

## Papel do Banco no Clurg

O banco vai armazenar:

* Projetos
* Commits
* Status de CI
* Deploys
* Métricas básicas
* Usuários (futuro)

O banco **não guarda código**.

---

## Estrutura Lógica de Dados

### Tabela: projects

```sql
CREATE TABLE projects (
    id SERIAL PRIMARY KEY,
    name TEXT UNIQUE NOT NULL,
    path TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT now()
);
```

---

### Tabela: commits

```sql
CREATE TABLE commits (
    id SERIAL PRIMARY KEY,
    project_id INTEGER REFERENCES projects(id),
    commit_hash TEXT NOT NULL,
    message TEXT,
    created_at TIMESTAMP DEFAULT now(),
    size_bytes BIGINT,
    checksum TEXT
);
```

---

### Tabela: ci_runs

```sql
CREATE TABLE ci_runs (
    id SERIAL PRIMARY KEY,
    commit_id INTEGER REFERENCES commits(id),
    status TEXT CHECK (status IN ('OK','FAIL')),
    log_file TEXT,
    executed_at TIMESTAMP DEFAULT now(),
    duration_ms INTEGER
);
```

---

### Tabela: deploys

```sql
CREATE TABLE deploys (
    id SERIAL PRIMARY KEY,
    commit_id INTEGER REFERENCES commits(id),
    environment TEXT,
    status TEXT CHECK (status IN ('OK','FAIL')),
    started_at TIMESTAMP,
    finished_at TIMESTAMP,
    log_file TEXT
);
```

---

## Fluxo de Escrita (quando gravar no banco)

### 1️⃣ Ao criar projeto

* Inserir em `projects`

### 2️⃣ Ao commit

* Inserir em `commits`
* Calcular tamanho
* Calcular checksum

### 3️⃣ Ao rodar CI

* Inserir em `ci_runs`

### 4️⃣ Ao deploy

* Inserir em `deploys`

Filesystem continua intacto.

---

## Integração com C (libpq)

### Dependências

```bash
sudo apt install libpq-dev
```

### Conexão básica

```c
PGconn *conn = PQconnectdb("host=localhost dbname=clurg user=clurg password=clurg");

if (PQstatus(conn) != CONNECTION_OK) {
    fprintf(stderr, "%s", PQerrorMessage(conn));
}
```

---

## Camada de Acesso (boa prática)

Criar arquivos dedicados:

```
db/
├── db.c
├── db.h
├── projects.c
├── commits.c
├── ci.c
└── deploys.c
```

Nada de SQL espalhado pelo código.

---

## Queries Essenciais (exemplos)

### Últimos commits

```sql
SELECT c.commit_hash, c.message, c.created_at, r.status
FROM commits c
LEFT JOIN ci_runs r ON r.commit_id = c.id
WHERE c.project_id = $1
ORDER BY c.created_at DESC
LIMIT 20;
```

---

## UI Benefícios Imediatos

Com DB:

* Dashboard rápido
* Filtros por status
* Histórico consistente
* Métricas visuais

---

## Migração Progressiva

Plano seguro:

1. Introduzir Postgres
2. Gravar dados novos
3. Backfill opcional
4. Nunca apagar FS

---

## Segurança Inicial

* Usuário PostgreSQL dedicado
* Senha fora do código (env)
* Conexão local inicialmente

---

## O que NÃO colocar no banco

* Código fonte
* Snapshots
* Logs completos
* Artefatos binários

Banco não é lixeira.

---

## Fase Seguinte (depois do DB)

* API REST
* Auth
* Multi-tenancy
* Integrações externas

---

## FASE 12 — IMPLEMENTAÇÃO DO DATABASE (PostgreSQL)

> *Da teoria à persistência real, sem quebrar nada.*

### Objetivo da Fase 12

Transformar o PostgreSQL no **índice oficial do Clurg**, mantendo o filesystem como fonte de verdade.

Ao final desta fase:

* O Clurg continuará funcionando **sem banco** (fallback)
* Com banco ativo, tudo será **mais rápido, consultável e histórico**

---

## Etapas da FASE 12 (ordem obrigatória)

### 12.1 — Preparação do Ambiente

* Criar usuário PostgreSQL dedicado (`clurg`)
* Criar banco `clurg`
* Definir variáveis de ambiente:

```bash
export CLURG_DB_HOST=localhost
export CLURG_DB_NAME=clurg
export CLURG_DB_USER=clurg
export CLURG_DB_PASS=******
```

Nada de senha hardcoded.

---

### 12.2 — Criação do Schema Inicial

* Criar as tabelas:

  * `projects`
  * `commits`
  * `ci_runs`
  * `deploys`

* Executar via script `db/schema.sql`

* Commitar o schema como **contrato de dados**

---

### 12.3 — Camada DB em C (libpq)

Criar módulo isolado:

```
db/
├── db.c        // conexão, helpers
├── db.h
├── projects.c // CRUD projetos
├── commits.c  // inserts commits
├── ci.c       // registros de CI
├── deploys.c
```

Regras:

* Nenhum `PQexec` fora dessa pasta
* Código de negócio **não conhece SQL**

---

### 12.4 — Integração Progressiva (sem quebrar nada)

Ordem segura:

1. Projetos → DB
2. Commits → DB
3. CI runs → DB
4. Deploys → DB

Filesystem continua sempre ativo.

---

### 12.5 — Fallback e Resiliência

* Se DB cair:

  * Clurg continua funcionando
  * Apenas perde indexação

Banco **acelera**, não impede.

---

### 12.6 — Testes da Camada DB

Criar testes específicos:

* Conexão válida
* Insert simples
* Query básica
* Falha controlada

Nada de mock mágico. DB real.

---

### 12.7 — Uso do DB na UI

Primeiros ganhos:

* Lista de projetos
* Últimos commits
* Status de CI

Sem mudar HTML ainda.

---

## Critérios de Conclusão da FASE 12

✔ Clurg funciona com ou sem DB
✔ Nenhuma feature antiga quebra
✔ Queries rápidas
✔ Código organizado
✔ Testes passando

---

## Encerramento

O PostgreSQL não torna o Clurg complexo.
Ele torna o Clurg **legível no tempo**.

> *Arquivos guardam a verdade.*
> *O banco conta a história.*

---

Próximo documento natural:

* `todo-api.md`
* `todo-auth.md`
* `todo-metrics.md`

## PostgreSQL — Estado Atual e Contrato de Conexão (Fase 12)

### Database

- PostgreSQL rodando localmente
- Database utilizado pela aplicação: `clurg`
- Todas as estruturas SQL estão criadas **no database `clurg`**
- Schema explícito utilizado: `clurg`

> Observação: a aplicação **não** deve usar o database `postgres`.

---

### Contrato de Conexão (Aplicação)

A aplicação Clurg conecta ao PostgreSQL como serviço, não como arquivo.

Parâmetros esperados (ambiente local):

- Host: `localhost` ou `127.0.0.1`
- Port: `5432`
- Database: `clurg`
- User: definido no ambiente local
- Password: definido em `.clurg/database.conf`

A aplicação **não depende de `search_path`**.  
Todo SQL deve usar nomes totalmente qualificados (`clurg.tabela`).

---

### Estrutura SQL Existente

#### Schema `clurg`

##### Tabela `clurg.projects`
- Identificação e organização de projetos
- Sem foreign keys obrigatórias neste momento

##### Tabela `clurg.commits`
Tabela usada como **índice de commits persistidos no filesystem**.

Campos:
- `id`
- `project`
- `message`
- `author`
- `timestamp`
- `checksum`
- `size_bytes`
- `ci_status`
- `created_at`

O banco **não é a fonte da verdade**.  
O filesystem é a fonte da verdade.

---

### Integração com Aplicação — Estado Atual

- Código em C
- Uso direto de `libpq`
- Conexão já implementada e testada (`SELECT 1`)
- Nenhuma leitura real de dados ainda

---

### Próximo Passo Planejado

Implementar **leitura simples** da tabela `clurg.commits`:

- Apenas `SELECT`
- Nenhuma alteração de schema
- Nenhuma escrita no banco
- SQL explícito usando `clurg.commits`
- Exibição do resultado no CLI

---

## Progresso da Implementação

### ✅ FASE 12.1 - Infraestrutura Básica (COMPLETA)

**Data:** 2025-01-21  
**Status:** ✅ Concluída

#### O que foi implementado:

1. **Configuração PostgreSQL**
   - Usuário `clurg` criado
   - Banco `clurg` criado
   - Permissões concedidas

2. **Schema SQL**
   - Arquivo `sql/schema.sql` criado
   - Schema `clurg` definido
   - Tabelas `projects` e `commits` criadas
   - Índices de performance adicionados
   - DDL idempotente (usa `IF NOT EXISTS`)

3. **Conexão C/libpq**
   - Função `db_init()` implementada
   - Carregamento de config de `.clurg/database.conf`
   - Conexão testada com `SELECT 1`
   - Comando CLI `db-test` funcionando

4. **Primeira Leitura Real**
   - Função `db_read_first_commit()` implementada
   - Query `SELECT * FROM clurg.commits ORDER BY created_at ASC LIMIT 1`
   - Comando CLI `db-first-commit` criado
   - Tratamento de resultado vazio (sem commits)

#### Arquivos modificados/criados:
- `sql/schema.sql` (novo)
- `core/db.h` (declaração da nova função)
- `core/db.c` (implementação da leitura)
- `core/main.c` (novo comando CLI)
- `.clurg/database.conf` (senha corrigida)

#### Testes realizados:
- ✅ Conexão estabelecida
- ✅ Schema aplicado sem erros
- ✅ Tabelas criadas corretamente
- ✅ Comando `db-test` funciona
- ✅ Comando `db-first-commit` funciona (retorna vazio, como esperado)

#### Contrato mantido:
- ✅ Filesystem permanece como source of truth
- ✅ Banco usado apenas como índice
- ✅ Conexão sem caminhos físicos
- ✅ SQL totalmente qualificado (`clurg.commits`)

---

### Próximos Passos (FASE 12.2)

1. **Inserir primeiro commit real**
   - Modificar `clurg commit` para escrever no banco
   - Inserir dados na tabela `clurg.commits`
   - Manter atomicidade (filesystem primeiro, banco depois)

2. **Expandir leituras**
   - Listar todos commits de um projeto
   - Buscar commit por hash
   - Filtros por data/autor

3. **Integração com API**
   - Endpoints REST para consultar commits via banco
   - Cache inteligente (banco como índice rápido)

4. **Métricas e monitoramento**
   - Contadores de commits por projeto
   - Estatísticas de uso
   - Performance de queries

