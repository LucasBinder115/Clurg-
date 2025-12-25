# Clurg — CI/CD Nativo (todo-cicd.md)

> *Pipeline simples, previsível e sob teu controle.*
> Nada de YAML mágico, nada de cloud. Só Linux, C e bom senso.

---

## Visão do CI/CD do Clurg

No Clurg:

* **Commit gera snapshot**
* **Snapshot dispara pipeline**
* **Pipeline gera logs**
* **Logs viram histórico**

Sem branches, sem matrix, sem segredo.

---

## FASE 9 — POLIMENTO DA INTERFACE ✅ COMPLETA

### ✅ Melhorias Implementadas

**Dark Mode Nativo:**
- CSS Variables para temas consistentes
- Detecção automática de preferência do sistema
- Transições suaves entre light/dark

**Layout Responsivo:**
- Mobile-first approach
- Breakpoints otimizados (768px, 480px)
- Componentes adaptáveis

**Performance:**
- CSS crítico inline
- Estrutura HTML semântica
- Carregamento otimizado

**Acessibilidade:**
- Contraste adequado (WCAG AA)
- Navegação por teclado
- Screen reader friendly

### 📱 Páginas Atualizadas
- ✅ Dashboard principal
- ✅ Página de métricas  
- ✅ Listagem de projetos
- ✅ Detalhes de commit
- ✅ Listagem de commits

### 🎨 Sistema de Design
- Variáveis CSS consistentes
- Paleta de cores profissional
- Tipografia otimizada
- Componentes reutilizáveis

---

## 🎯 PRÓXIMA FASE: FASE 10 — DEPLOY AUTOMÁTICO ✅ IMPLEMENTADA

### ✅ Funcionalidades Implementadas

**🚀 Comando Deploy:**
- Comando `clurg deploy <environment> <commit_id>` funcional
- Configuração via arquivo `clurg.deploy` simples
- Suporte a múltiplos ambientes (staging, production)

**📦 Processo de Deploy:**
- Backup automático antes de mudanças
- Extração de commits para diretórios de deploy
- Execução de comandos customizáveis
- Healthcheck para validação

**📊 Logs e Rastreamento:**
- Logs detalhados de cada deploy
- Status de sucesso/falha
- Histórico completo em arquivos

**🔧 Configuração Flexível:**
- Comandos de deploy customizáveis
- Healthchecks configuráveis
- Timeouts ajustáveis por ambiente

### 🎯 Resultados Alcançados

- ✅ **Deploy básico funcionando** - Comando executa e registra logs
- ✅ **Configuração por ambiente** - Staging e production suportados
- ✅ **Backup automático** - Estado anterior preservado
- ✅ **Logs detalhados** - Rastreamento completo de operações
- ✅ **Healthcheck integrado** - Validação pós-deploy

### 📋 Limitações Atuais

- Deploy funciona apenas com commits locais
- Não há integração automática com CI
- Interface web não mostra status de deploy
- Não há rollback automático em falha

---

## 🎯 PRÓXIMA FASE: FASE 11 — API REST COMPLETA

### Visão da API REST

O Clurg deve oferecer:

* **API RESTful completa** para todas as operações
* **Autenticação segura** com tokens
* **Documentação automática** (OpenAPI/Swagger)
* **Integração com ferramentas** externas
* **Webhooks para eventos** (commit, deploy, CI)

### Funcionalidades Planejadas

**Endpoints Principais:**
- `GET/POST /projects` - Gerenciar projetos
- `GET/POST /commits` - Operações com commits
- `POST /deploy` - Gatilho de deploy
- `GET /status` - Status do sistema

**Segurança:**
- Autenticação por token
- Controle de permissões
- Rate limiting
- Logs de auditoria

**Integração:**
- Webhooks para eventos
- API compatível com Git
- Suporte a CI/CD externo

---

## Princípios (não quebre isso)

1. Pipeline deve ser **determinístico**
2. Falhou → registra → segue a vida
3. Nada roda como root
4. Logs são imutáveis
5. Simples > completo

---

## Estrutura de Diretórios

```
.clurg/
├── projects/
│   └── meu-projeto/
│       ├── commits/
│       ├── ci/
│       │   ├── runs/
│       │   │   ├── ci_20251222_210012.log
│       │   └── last_status
│       ├── clurg.ci
│       └── metadata.json
```

---

## Arquivo de Pipeline (`clurg.ci`)

Formato propositalmente simples:

```
# cada linha é um comando
# falha se retornar != 0

make clean
make
./bin/test
```

Sem YAML. Sem parser complexo.

---

## Parte 1 — Disparo Automático

### Quando roda?

* Após `clurg commit`
* Após `clurg push` (opcional)

### Fluxo

1. Commit criado
2. Snapshot extraído em diretório temporário
3. Pipeline executado ali
4. Logs salvos
5. Status gravado

---

## Parte 2 — Execução do Pipeline

### Execução

* `fork()`
* `execvp()`
* `waitpid()`

Cada linha do `clurg.ci` vira um processo.

### Regras

* stdout + stderr → log
* Se um comando falhar:

  * marca FAIL
  * interrompe pipeline

---

## Parte 3 — Logs de CI

### Nome do log

```
ci_YYYYMMDD_HHMMSS.log
```

### Conteúdo

```
[START] 2025-12-22 21:00:12
[CMD] make clean
[OK]
[CMD] make
[OK]
[CMD] ./bin/test
[FAIL] code=1
[END] FAIL
```

---

## Parte 4 — Status por Commit

Arquivo simples:

```
.clurg/projects/meu-projeto/ci/last_status
```

Conteúdo:

```
OK
```

ou

```
FAIL
```

---

## Parte 5 — Integração com UI

### Página de projeto mostra:

* último status CI
* lista de logs
* data

Visual:

```
🟢 OK   ci_20251222_210012.log
🔴 FAIL ci_20251221_195932.log
```

---

## Parte 6 — Segurança Básica

* Executar como usuário dedicado `clurg`
* Diretório temporário com permissões restritas
* Timeout por comando (futuro)

---

## Parte 7 — CLI Auxiliar

### Ver status

```
clurg ci status
```

### Rodar manual

```
clurg ci run
```

---

## Parte 8 — Limpeza Automática

* Manter últimos N logs
* Apagar mais antigos
* Nunca apagar commits

---

## Fase Seguinte (depois do CI)

* Banco de dados para indexar resultados
* UI mais rica
* Deploy automático

---

## Encerramento

O CI do Clurg não é rápido.
Ele é **honesto**.

> *Se passou, passou de verdade.*
> *Se falhou, alguém vai saber.*

---

---

## 📋 Roadmap Atualizado

### ✅ FASES CONCLUÍDAS
- **FASE 0-7**: Core VCS (clone, commit, push, etc.)
- **FASE 8**: Sistema de plugins
- **FASE 9**: Polimento da interface
- **FASE 10**: Deploy automático

### 🔄 FASE ATUAL
- **FASE 11**: API REST completa (próxima)

### 📅 FASES FUTURAS
- **FASE 12**: Multi-tenancy
- **FASE 13**: Integrações externas

---

Próximo possível documento:

* `todo-db.md`
* `todo-ui-brand.md`
* `todo-deploy.md`
