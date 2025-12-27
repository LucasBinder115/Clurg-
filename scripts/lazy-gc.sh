#!/bin/bash

# lazy-gc.sh - Garbage Collector Automático (Lazy)
# Ativado quando espaço em disco fica baixo
# FASE 13 - Otimização: GC Automático

set -e  # Exit on any error

# Configurações
CLURG_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BACKUP_DIR="$CLURG_ROOT/.clurg/backups"
LOG_FILE="$CLURG_ROOT/.clurg/gc.log"
CONFIG_FILE="$CLURG_ROOT/.clurg/gc.conf"

# Valores padrão (podem ser sobrescritos pelo config)
DISK_THRESHOLD_PERCENT=85  # Ativar GC quando disco > 85% usado
MIN_FREE_SPACE_GB=5        # Manter pelo menos 5GB livres
KEEP_BACKUPS=3             # Manter 3 backups mais recentes
LOG_RETENTION_DAYS=7       # Manter logs por 7 dias
MAX_GC_RUNS_PER_DAY=3      # Máximo 3 execuções por dia

# Carregar configuração se existir
if [ -f "$CONFIG_FILE" ]; then
    source "$CONFIG_FILE"
fi

# Logging
log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') [LAZY-GC] $*" >> "$LOG_FILE"
    echo "$*"
}

# Verificar se já rodou hoje muitas vezes
check_daily_limit() {
    local today=$(date +%Y%m%d)
    local runs_today=$(grep "$today" "$LOG_FILE" 2>/dev/null | grep "Starting lazy GC" | wc -l)

    if [ "$runs_today" -ge "$MAX_GC_RUNS_PER_DAY" ]; then
        log "Daily limit reached ($runs_today/$MAX_GC_RUNS_PER_DAY), skipping"
        exit 0
    fi
}

# Verificar espaço em disco
check_disk_space() {
    # Usar df para verificar espaço do filesystem onde está o .clurg
    local disk_usage_percent=$(df "$CLURG_ROOT" | tail -1 | awk '{print $5}' | sed 's/%//')
    local free_space_gb=$(df -BG "$CLURG_ROOT" | tail -1 | awk '{print $4}' | sed 's/G//')

    log "Disk usage: ${disk_usage_percent}%, Free space: ${free_space_gb}GB"

    # Verificar thresholds
    if [ "$disk_usage_percent" -lt "$DISK_THRESHOLD_PERCENT" ] && [ "$free_space_gb" -gt "$MIN_FREE_SPACE_GB" ]; then
        log "Disk space OK, no cleanup needed"
        return 1  # Não precisa limpar
    fi

    log "Disk space low, starting cleanup"
    return 0  # Precisa limpar
}

# Limpar backups antigos
cleanup_backups() {
    if [ ! -d "$BACKUP_DIR" ]; then
        log "Backup directory not found: $BACKUP_DIR"
        return
    fi

    local backup_count=$(ls -1 "$BACKUP_DIR"/backup-*.tar.gz 2>/dev/null | wc -l)
    log "Found $backup_count backups"

    if [ "$backup_count" -le "$KEEP_BACKUPS" ]; then
        log "Only $backup_count backups, keeping all"
        return
    fi

    local to_remove=$((backup_count - KEEP_BACKUPS))
    local removed=0
    local space_saved=0

    log "Removing $to_remove oldest backups"

    # Remover backups mais antigos (ordenados por data)
    ls -t "$BACKUP_DIR"/backup-*.tar.gz | tail -n "$to_remove" | while read -r backup; do
        if [ -f "$backup" ]; then
            local size_mb=$(du -m "$backup" | cut -f1)
            log "Removing backup: $(basename "$backup") (${size_mb}MB)"
            rm -f "$backup"
            removed=$((removed + 1))
            space_saved=$((space_saved + size_mb))
        fi
    done

    log "Backup cleanup: removed $removed files, saved ${space_saved}MB"
}

# Limpar logs antigos
cleanup_logs() {
    local log_dirs=("$CLURG_ROOT/.clurg/ci" "$CLURG_ROOT/.clurg/hooks")
    local total_cleaned=0

    for log_dir in "${log_dirs[@]}"; do
        if [ -d "$log_dir" ]; then
            # Encontrar logs mais antigos que LOG_RETENTION_DAYS
            local old_logs=$(find "$log_dir" -name "*.log" -mtime +"$LOG_RETENTION_DAYS" 2>/dev/null)
            local log_count=$(echo "$old_logs" | grep -v '^$' | wc -l)

            if [ "$log_count" -gt 0 ]; then
                log "Found $log_count old log files in $(basename "$log_dir")"
                echo "$old_logs" | while read -r logfile; do
                    if [ -f "$logfile" ]; then
                        local size_kb=$(du -k "$logfile" | cut -f1)
                        log "Removing old log: $(basename "$logfile") (${size_kb}KB)"
                        rm -f "$logfile"
                        total_cleaned=$((total_cleaned + size_kb))
                    fi
                done
            fi
        fi
    done

    if [ "$total_cleaned" -gt 0 ]; then
        log "Log cleanup: saved ${total_cleaned}KB"
    fi
}

# Limpar arquivos temporários
cleanup_temp() {
    local temp_dir="$CLURG_ROOT/.clurg/temp"
    local cleaned=0

    if [ -d "$temp_dir" ]; then
        # Remover arquivos mais antigos que 1 dia
        find "$temp_dir" -type f -mtime +1 -exec rm -f {} \; -exec log "Removed temp file: {}" \; 2>/dev/null || true
        cleaned=$(find "$temp_dir" -type f -mtime +1 2>/dev/null | wc -l)
    fi

    if [ "$cleaned" -gt 0 ]; then
        log "Temp cleanup: removed $cleaned old temp files"
    fi
}

# Verificar se espaço foi liberado
verify_cleanup() {
    local after_usage=$(df "$CLURG_ROOT" | tail -1 | awk '{print $5}' | sed 's/%//')
    local after_free=$(df -BG "$CLURG_ROOT" | tail -1 | awk '{print $4}' | sed 's/G//')

    log "After cleanup: ${after_usage}%, ${after_free}GB free"

    if [ "$after_free" -gt "$MIN_FREE_SPACE_GB" ]; then
        log "✅ Cleanup successful - sufficient space available"
    else
        log "⚠️  Cleanup completed but space still low (${after_free}GB < ${MIN_FREE_SPACE_GB}GB)"
    fi
}

# Função principal
main() {
    log "Starting lazy GC check"

    # Verificar limite diário
    check_daily_limit

    # Verificar espaço em disco
    if ! check_disk_space; then
        log "No cleanup needed"
        exit 0
    fi

    log "=== Starting Lazy Garbage Collection ==="

    # Executar limpeza em ordem de prioridade
    cleanup_backups
    cleanup_logs
    cleanup_temp

    # Verificar resultado
    verify_cleanup

    log "Lazy GC completed"
}

# Executar apenas se chamado diretamente
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi