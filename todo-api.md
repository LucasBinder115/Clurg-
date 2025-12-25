# Clurg — FASE 11: API REST COMPLETA (todo-api.md)

> *API poderosa, segura e integrada.*
> RESTful API para automação e integração.

---

## Visão da API REST

O Clurg deve oferecer uma **API RESTful completa** que permita:

* **Integração total** com ferramentas externas
* **Automatização avançada** de workflows
* **Monitoramento em tempo real** do sistema
* **Controle programático** de todas as operações
* **Webhooks para eventos** importantes

---

## Princípios da API

1. **RESTful design** - Recursos, verbos HTTP apropriados
2. **JSON everywhere** - Request/response em JSON
3. **Versionamento** - `/api/v1/` prefixo
4. **Autenticação obrigatória** - Token-based security
5. **Rate limiting** - Proteção contra abuso

---

## Endpoints Principais

### 📁 Projetos (`/api/v1/projects`)

**GET /api/v1/projects**
- Lista todos os projetos
- Query params: `limit`, `offset`, `search`
- Response: Array de projetos com metadados

**POST /api/v1/projects**
- Cria novo projeto
- Body: `{"name": "meu-projeto", "description": "..." }`
- Response: Projeto criado

**GET /api/v1/projects/{name}**
- Detalhes do projeto
- Response: Metadados completos + status

**DELETE /api/v1/projects/{name}**
- Remove projeto (com confirmação)

### 📦 Commits (`/api/v1/projects/{name}/commits`)

**GET /api/v1/projects/{name}/commits**
- Lista commits do projeto
- Query params: `limit`, `offset`, `branch`
- Response: Array de commits com metadados

**POST /api/v1/projects/{name}/commits**
- Faz commit no projeto
- Body: `{"message": "commit message", "files": [...]}`

**GET /api/v1/projects/{name}/commits/{id}**
- Detalhes do commit
- Response: Metadados + arquivos modificados

**GET /api/v1/projects/{name}/commits/{id}/download**
- Download do snapshot do commit

### 🚀 Deploy (`/api/v1/projects/{name}/deploy`)

**POST /api/v1/projects/{name}/deploy**
- Gatilho de deploy
- Body: `{"environment": "staging", "commit_id": "abc123"}`
- Response: Status do deploy iniciado

**GET /api/v1/projects/{name}/deploy/status**
- Status atual dos ambientes
- Response: Status de staging/production

**GET /api/v1/projects/{name}/deploy/history**
- Histórico de deploys
- Query params: `environment`, `limit`

### 🔧 CI/CD (`/api/v1/projects/{name}/ci`)

**POST /api/v1/projects/{name}/ci/run**
- Executa pipeline CI manualmente
- Response: ID do job iniciado

**GET /api/v1/projects/{name}/ci/status**
- Status do último CI
- Response: Status + logs resumidos

**GET /api/v1/projects/{name}/ci/jobs**
- Lista jobs de CI
- Response: Array de jobs com status

**GET /api/v1/projects/{name}/ci/jobs/{id}/logs**
- Logs completos do job

### 📊 Sistema (`/api/v1/system`)

**GET /api/v1/system/status**
- Status geral do sistema
- Response: Health check + métricas básicas

**GET /api/v1/system/metrics**
- Métricas detalhadas
- Response: JSON com todas as métricas

**GET /api/v1/system/logs**
- Logs do sistema
- Query params: `level`, `since`, `limit`

---

## Autenticação e Segurança

### Token-Based Authentication

**Header obrigatório:**
```
Authorization: Bearer <token>
```

**Tokens armazenados em:**
```
.clurg/security/auth-tokens.json
```

**Formato:**
```json
{
  "tokens": [
    {
      "token": "abc123...",
      "user": "ci-user",
      "permissions": ["read", "write", "deploy"],
      "projects": ["project1", "project2"],
      "expires": "2025-12-31T23:59:59Z",
      "rate_limit": 100
    }
  ]
}
```

### Rate Limiting

- **Por token**: 100 requests/minuto (configurável)
- **Por IP**: 1000 requests/minuto
- **Headers de resposta**:
  ```
  X-RateLimit-Limit: 100
  X-RateLimit-Remaining: 95
  X-RateLimit-Reset: 1640995200
  ```

### CORS Support

- **Allowed origins**: Configurável
- **Allowed methods**: GET, POST, PUT, DELETE
- **Allowed headers**: Authorization, Content-Type

---

## Webhooks

### Configuração

**Arquivo:** `clurg-webhooks.json`
```json
{
  "webhooks": [
    {
      "id": "deploy-success",
      "url": "https://slack.com/webhook/...",
      "events": ["deploy.success"],
      "secret": "webhook-secret",
      "active": true
    }
  ]
}
```

### Eventos Disponíveis

- `commit.created` - Novo commit feito
- `ci.started` - CI iniciado
- `ci.success` - CI passou
- `ci.failed` - CI falhou
- `deploy.started` - Deploy iniciado
- `deploy.success` - Deploy concluído
- `deploy.failed` - Deploy falhou
- `project.created` - Projeto criado
- `project.deleted` - Projeto removido

### Payload do Webhook

```json
{
  "event": "deploy.success",
  "timestamp": "2025-12-22T21:00:00Z",
  "project": "meu-projeto",
  "data": {
    "environment": "staging",
    "commit_id": "abc123",
    "deploy_id": "deploy_20251222_210000",
    "duration": 45
  },
  "signature": "sha256=..."
}
```

---

## Formatos de Resposta

### Sucesso (2xx)

```json
{
  "success": true,
  "data": { ... },
  "meta": {
    "timestamp": "2025-12-22T21:00:00Z",
    "request_id": "req_123456"
  }
}
```

### Erro (4xx/5xx)

```json
{
  "success": false,
  "error": {
    "code": "VALIDATION_ERROR",
    "message": "Commit ID is required",
    "details": { ... }
  },
  "meta": {
    "timestamp": "2025-12-22T21:00:00Z",
    "request_id": "req_123456"
  }
}
```

### Paginação

```json
{
  "success": true,
  "data": [ ... ],
  "meta": {
    "pagination": {
      "page": 1,
      "limit": 20,
      "total": 150,
      "total_pages": 8
    }
  }
}
```

---

## Documentação OpenAPI

### Geração Automática

- **Endpoint:** `GET /api/v1/docs`
- **Formato:** OpenAPI 3.0 JSON
- **UI:** Swagger UI integrada em `/api/docs`

### Exemplo de Documentação

```yaml
openapi: 3.0.0
info:
  title: Clurg API
  version: v1
  description: REST API for Clurg VCS

paths:
  /api/v1/projects:
    get:
      summary: List projects
      security:
        - bearerAuth: []
      responses:
        '200':
          description: Success
```

---

## Implementação Técnica

### Estrutura de Código

```
web/
├── server.c          # Servidor HTTP principal
├── api.c            # Handlers da API REST
├── auth.c           # Autenticação e autorização
├── webhooks.c       # Sistema de webhooks
├── rate_limit.c     # Rate limiting
└── docs.c           # Geração de documentação
```

### Middleware Pipeline

```
Request → CORS → Rate Limit → Auth → Routing → Handler → Response
```

### Content Negotiation

- **Accept:** `application/json` (obrigatório)
- **Content-Type:** `application/json` (para POST/PUT)

---

## Roadmap de Implementação

### Semana 1: Foundation
- [ ] Estrutura base da API (`/api/v1/`)
- [ ] Sistema de autenticação por token
- [ ] Rate limiting básico
- [ ] Endpoint `/api/v1/system/status`

### Semana 2: Projetos e Commits
- [ ] CRUD completo de projetos
- [ ] API de commits (listar, detalhes, download)
- [ ] Paginação e filtros
- [ ] Validação de entrada

### Semana 3: CI/CD e Deploy
- [ ] API para executar CI
- [ ] API para deploy
- [ ] Status e histórico
- [ ] Integração com comandos existentes

### Semana 4: Advanced Features
- [ ] Sistema de webhooks
- [ ] Documentação OpenAPI
- [ ] CORS e segurança avançada
- [ ] Testes e otimização

---

## Critérios de Sucesso

* **100% RESTful** - API segue princípios REST
* **Documentação completa** - OpenAPI + exemplos
* **Autenticação robusta** - Token-based com permissões
* **Rate limiting** - Proteção contra abuso
* **Webhooks funcionais** - Integração com ferramentas externas
* **Testes automatizados** - Cobertura completa

---

## Próximas Fases Possíveis

Após API REST completa:

* **FASE 12**: Multi-tenancy
* **FASE 13**: Integrações externas avançadas
* **FASE 14**: Interface web moderna

---

> *API que conecta.*
> *Integração que automatiza.*
> *Desenvolvedor que sorri.*