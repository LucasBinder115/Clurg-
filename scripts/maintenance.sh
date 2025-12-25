#!/bin/bash
# Clurg Maintenance Script
# Executado periodicamente via cron para manutenção automatizada

set -e

echo "🔧 Iniciando manutenção automática do Clurg..."

# Diretório base do repositório
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_DIR"

# 1. Verificar se estamos em um repositório Clurg
if [ ! -d ".clurg" ]; then
    echo "❌ Erro: Não é um repositório Clurg"
    exit 1
fi

# 2. Limpeza profunda de commits muito antigos (90 dias)
echo "🧹 Executando limpeza profunda..."
COMMITS_DIR=".clurg/commits"
DEEP_CLEAN_DAYS=90

if [ -d "$COMMITS_DIR" ]; then
    find "$COMMITS_DIR" -name "*.tar.gz" -mtime +$DEEP_CLEAN_DAYS -print | while read -r old_commit; do
        base_name=$(basename "$old_commit" .tar.gz)
        echo "🗑️ Removendo commit muito antigo: $base_name"
        rm -f "$COMMITS_DIR/$base_name.tar.gz" "$COMMITS_DIR/$base_name.meta" "$COMMITS_DIR/$base_name.metadata.json"
    done
fi

# 3. Limpeza de backups antigos (manter apenas 30 dias)
echo "💾 Limpando backups antigos..."
BACKUP_DIR=".clurg/backups"
if [ -d "$BACKUP_DIR" ]; then
    find "$BACKUP_DIR" -name "backup-*.tar.gz" -mtime +30 -exec rm -f {} \; 2>/dev/null || true
fi

# 4. Verificação de integridade geral
echo "🔍 Verificando integridade geral..."
INTEGRITY_FILE=".clurg/integrity.txt"
if [ -f "$INTEGRITY_FILE" ]; then
    if ! sha256sum -c "$INTEGRITY_FILE" --quiet 2>/dev/null; then
        echo "⚠️ ALERTA: Integridade comprometida! Arquivos modificados fora do VCS"
        # Em produção, poderia enviar notificação
    else
        echo "✅ Integridade OK"
    fi
fi

# 5. Otimização de logs
echo "📝 Otimizando logs..."
HOOK_LOG=".clurg/hooks/post-commit.log"
if [ -f "$HOOK_LOG" ]; then
    # Manter apenas últimas 500 linhas
    tail -n 500 "$HOOK_LOG" > "${HOOK_LOG}.tmp" && mv "${HOOK_LOG}.tmp" "$HOOK_LOG"
fi

# 6. Atualizar estatísticas de manutenção
echo "📊 Atualizando estatísticas de manutenção..."
STATS_FILE=".clurg/stats.json"
if [ -f "$STATS_FILE" ]; then
    jq --arg date "$(date +%Y-%m-%d)" \
       '.last_maintenance = $date | .maintenance_runs += 1' "$STATS_FILE" > "${STATS_FILE}.tmp" && \
    mv "${STATS_FILE}.tmp" "$STATS_FILE"
fi

# 7. Verificar espaço em disco
echo "💽 Verificando uso de espaço..."
if command -v df >/dev/null 2>&1; then
    CLURG_SIZE=$(du -sh .clurg 2>/dev/null | cut -f1)
    echo "📏 Tamanho do .clurg: $CLURG_SIZE"
fi

echo "✅ Manutenção concluída com sucesso"