TODO.md — FASE 15
Finalização do Core do Clurg (CLI + Versionamento + Deploy)

Objetivo:
Remover completamente a camada web e consolidar o Clurg como um sistema educacional de versionamento e deploy via CLI, inspirado no Git.

🧹 1. REMOÇÃO TOTAL DA CAMADA WEB (DESACOPLAMENTO)
Deletar código e artefatos web

Remover diretório web/

Remover binário clurg-web do Makefile

Remover targets relacionados a web (make clurg-web)

Remover documentação relacionada a web:

Referências no README

Referências em architecture.md (se existir)

Remover scripts auxiliares que só fazem sentido para web

Validar build sem qualquer dependência web

✅ Resultado esperado:

O projeto não compila, não referencia e não depende de servidor HTTP.

🧠 2. CONSOLIDAÇÃO DO CORE DE VERSIONAMENTO (CLI)
Estrutura base

Garantir .clurg/ como única fonte de controle

Garantir commits como snapshots imutáveis (.tar.gz)

HEAD como ponteiro simples

Comandos obrigatórios (estilo Git)
clurg init

Criar estrutura .clurg/

Criar diretórios:

commits/

logs/

deploy/

Criar arquivo HEAD

Proteger permissões básicas

clurg status

Verificar se repositório está inicializado

Mostrar commit atual (HEAD)

Indicar estado limpo ou modificado (simplificado)

Mensagem clara para iniciantes

clurg add .

Registrar arquivos para o próximo commit

Implementação simples (snapshot-based)

Sem staging complexo (educacional)

Preparar lista de arquivos para commit

clurg commit -m "mensagem"

Criar snapshot completo do projeto

Gerar ID único do commit

Salvar:

<id>.tar.gz

<id>.meta (mensagem, timestamp)

Atualizar HEAD

Executar hooks (se existirem)

clurg log

Listar commits em ordem cronológica

Mostrar:

ID

Data

Mensagem

Saída simples e legível

clurg show <commit>

Mostrar metadados do commit

Exibir:

Mensagem

Timestamp

Lista de arquivos (opcional)

Não extrair arquivos

clurg checkout <commit>

Restaurar snapshot do commit informado

Atualizar working directory

Atualizar HEAD

Aviso claro de overwrite de arquivos

🚀 3. DEPLOY — SOMENTE O NECESSÁRIO (SEM EXCESSO)
Manter apenas comandos essenciais
clurg deploy --help

Ajuda clara e didática

Exemplos de uso

Explicação educacional

clurg deploy status

Listar ambientes configurados

Mostrar commit ativo por ambiente

Indicar lock ativo ou não

clurg deploy run <env>

Executar fluxo completo:

Backup

Deploy

Healthcheck

Switch de symlink

Registrar log do deploy

clurg deploy run <env> <commit>

Deploy reprodutível

Ignorar commits mais novos

Garantir integridade do snapshot

clurg deploy rollback <env>

Restaurar último snapshot válido

Atualizar symlink current

Registrar rollback em log

clurg deploy rollback <env> <snapshot>

Rollback exato para snapshot informado

Não depender de banco, CI ou rede

clurg deploy lock <env>

Impedir novos deploys

Criar lockfile explícito

clurg deploy unlock <env>

Remover lock manualmente

🧩 4. LIMPEZA FINAL DO PROJETO

Remover código morto

Remover scripts não utilizados

Revisar Makefile

Garantir make all limpo

Atualizar README final

Atualizar CHANGELOG (encerramento)

🏁 5. ENCERRAMENTO OFICIAL

Commit final: Finalize Clurg v1.0 (educational)

Tag:

git tag v1.0-educational


Projeto congelado (somente manutenção)

🧠 Nota Final

Clurg não é sobre features.
É sobre entender como as coisas realmente funcionam.

Fase 15 é o ponto final.
Depois disso, o projeto cumpriu seu papel. e finalizamos o projeto, só vamos ficar para polir o projeto. 