#!/bin/bash

# cleanup-backups.sh - Limpeza segura de backups antigos do Clurg
# FASE 13 - Otimização: Redução de footprint em disco

set -e  # Exit on any error

BACKUP_DIR=".clurg/backups"
KEEP_COUNT=3  # Manter apenas os 3 backups mais recentes

echo "🧹 Clurg Backup Cleanup Script"
echo "=============================="
echo "Diretório: $BACKUP_DIR"
echo "Manter: $KEEP_COUNT backups mais recentes"
echo ""

# Verificar se o diretório existe
if [ ! -d "$BACKUP_DIR" ]; then
    echo "❌ Diretório de backups não encontrado: $BACKUP_DIR"
    exit 1
fi

# Contar backups atuais
BACKUP_COUNT=$(ls -1 "$BACKUP_DIR"/backup-*.tar.gz 2>/dev/null | wc -l)
echo "📊 Backups atuais: $BACKUP_COUNT"

if [ "$BACKUP_COUNT" -le "$KEEP_COUNT" ]; then
    echo "✅ Nenhum backup para remover (menos de $KEEP_COUNT backups)"
    exit 0
fi

echo ""
echo "🔍 Backups que serão REMOVIDOS:"

# Listar backups a remover (ordenados por data, mais antigos primeiro)
BACKUPS_TO_REMOVE=$(ls -t "$BACKUP_DIR"/backup-*.tar.gz | tail -n +$((KEEP_COUNT + 1)))

if [ -z "$BACKUPS_TO_REMOVE" ]; then
    echo "Nenhum backup para remover"
    exit 0
fi

TOTAL_SIZE=0
while IFS= read -r backup; do
    if [ -f "$backup" ]; then
        SIZE=$(du -m "$backup" | cut -f1)
        TOTAL_SIZE=$((TOTAL_SIZE + SIZE))
        echo "  $(basename "$backup") - ${SIZE}MB"
    fi
done <<< "$BACKUPS_TO_REMOVE"

echo ""
echo "💾 Espaço a ser liberado: ${TOTAL_SIZE}MB"
echo ""

# Confirmação interativa
read -p "⚠️  Continuar com a remoção? (y/N): " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "❌ Operação cancelada pelo usuário"
    exit 0
fi

echo "🗑️  Removendo backups antigos..."

# Remover backups
REMOVED_COUNT=0
while IFS= read -r backup; do
    if [ -f "$backup" ]; then
        echo "  Removendo: $(basename "$backup")"
        rm -f "$backup"
        REMOVED_COUNT=$((REMOVED_COUNT + 1))
    fi
done <<< "$BACKUPS_TO_REMOVE"

echo ""
echo "✅ Limpeza concluída!"
echo "📊 Backups removidos: $REMOVED_COUNT"
echo "💾 Espaço liberado: ${TOTAL_SIZE}MB"
echo "📁 Backups restantes: $(ls -1 "$BACKUP_DIR"/backup-*.tar.gz 2>/dev/null | wc -l)"

# Verificar espaço em disco após limpeza
echo ""
echo "📈 Espaço em disco após limpeza:"
df -h "$PWD" | tail -1