#!/bin/bash

# gc-monitor.sh - Monitor de Espaço em Disco para GC Lazy
# Hook que pode ser chamado periodicamente ou sob demanda
# FASE 13 - Otimização: GC Automático

set -e

CLURG_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GC_SCRIPT="$CLURG_ROOT/scripts/lazy-gc.sh"
LOG_FILE="$CLURG_ROOT/.clurg/gc-monitor.log"

# Logging
log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') [GC-MONITOR] $*" >> "$LOG_FILE"
}

# Verificar se GC script existe
if [ ! -x "$GC_SCRIPT" ]; then
    log "ERROR: GC script not found or not executable: $GC_SCRIPT"
    exit 1
fi

log "Starting disk space monitor"

# Executar GC lazy
if "$GC_SCRIPT" >> "$LOG_FILE" 2>&1; then
    log "GC monitor completed successfully"
else
    log "ERROR: GC monitor failed with exit code $?"
    exit 1
fi