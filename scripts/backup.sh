#!/bin/bash
# Clurg Automated Backup Script
# Cria backup completo do repositório periodicamente

set -e

echo "💾 Iniciando backup automatizado do Clurg..."

# Diretório base do repositório
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_DIR"

# Verificar se estamos em um repositório Clurg
if [ ! -d ".clurg" ]; then
    echo "❌ Erro: Não é um repositório Clurg"
    exit 1
fi

# Criar diretório de backups se não existir
BACKUP_DIR=".clurg/backups"
mkdir -p "$BACKUP_DIR"

# Nome do arquivo de backup
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_FILE="$BACKUP_DIR/weekly_backup_$TIMESTAMP.tar.gz"

echo "📦 Criando backup: $(basename "$BACKUP_FILE")"

# Criar backup completo (excluindo arquivos temporários)
tar -czf "$BACKUP_FILE" \
    --exclude='.clurg/backups/*' \
    --exclude='.clurg/logs/*' \
    --exclude='*.tmp' \
    --exclude='*.log' \
    .clurg/ \
    2>/dev/null

# Verificar se o backup foi criado com sucesso
if [ -f "$BACKUP_FILE" ]; then
    BACKUP_SIZE=$(du -h "$BACKUP_FILE" | cut -f1)
    echo "✅ Backup criado com sucesso: $BACKUP_SIZE"

    # Calcular hash do backup para verificação
    BACKUP_HASH=$(sha256sum "$BACKUP_FILE" | cut -d' ' -f1)
    echo "$BACKUP_HASH  $(basename "$BACKUP_FILE")" > "$BACKUP_FILE.sha256"

    # Limpar backups antigos (manter apenas últimos 4 backups semanais)
    echo "🧹 Limpando backups antigos..."
    ls -t "$BACKUP_DIR"/weekly_backup_*.tar.gz | tail -n +5 | while read -r old_backup; do
        echo "🗑️ Removendo backup antigo: $(basename "$old_backup")"
        rm -f "$old_backup" "${old_backup}.sha256"
    done

    # Atualizar estatísticas
    STATS_FILE=".clurg/stats.json"
    if [ -f "$STATS_FILE" ]; then
        jq --arg date "$(date +%Y-%m-%d)" \
           '.last_backup = $date | .backups_created += 1' "$STATS_FILE" > "${STATS_FILE}.tmp" && \
        mv "${STATS_FILE}.tmp" "$STATS_FILE"
    fi

else
    echo "❌ Erro: Falha ao criar backup"
    exit 1
fi

echo "✅ Backup concluído"