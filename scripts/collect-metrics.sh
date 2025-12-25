#!/bin/bash
# Clurg Metrics Collector
# Coleta métricas de observabilidade do sistema

set -e

echo "📊 Clurg Metrics Collector"
echo "=========================="

# Verificar se estamos em um repositório Clurg
if [ ! -d ".clurg" ]; then
    echo "❌ Erro: Não é um repositório Clurg"
    exit 1
fi

# Arquivo de métricas
METRICS_FILE=".clurg/metrics.json"
TIMESTAMP=$(date +%s)
DATE=$(date '+%Y-%m-%d %H:%M:%S')

# Função para calcular tamanho do diretório
get_dir_size() {
    local dir=$1
    if [ -d "$dir" ]; then
        du -sb "$dir" 2>/dev/null | cut -f1 || echo "0"
    else
        echo "0"
    fi
}

# Função para contar arquivos
count_files() {
    local pattern=$1
    find . -name "$pattern" 2>/dev/null | wc -l || echo "0"
}

echo "📏 Coletando métricas..."

# Métricas básicas
TOTAL_COMMITS=$(count_files "*.tar.gz")
TOTAL_PROJECTS=$(find .clurg/projects -maxdepth 1 -type d 2>/dev/null | grep -v "^\.clurg/projects$" | wc -l || echo "0")
TOTAL_BACKUPS=$(count_files "weekly_backup_*.tar.gz")

# Tamanhos
CLURG_SIZE=$(get_dir_size ".clurg")
COMMITS_SIZE=$(get_dir_size ".clurg/commits")
PROJECTS_SIZE=$(get_dir_size ".clurg/projects")
BACKUPS_SIZE=$(get_dir_size ".clurg/backups")

# Estatísticas de atividade (últimos 7 dias)
RECENT_COMMITS=$(find .clurg/commits -name "*.tar.gz" -mtime -7 2>/dev/null | wc -l || echo "0")
RECENT_BACKUPS=$(find .clurg/backups -name "*.tar.gz" -mtime -7 2>/dev/null | wc -l || echo "0")

# Sistema
UPTIME=$(cut -d. -f1 /proc/uptime 2>/dev/null || echo "0")
LOAD_AVG=$(uptime | awk -F'load average:' '{ print $2 }' | tr -d ' ' 2>/dev/null || echo "unknown")

# Memória (se disponível)
MEM_TOTAL=$(grep MemTotal /proc/meminfo 2>/dev/null | awk '{print $2}' || echo "0")
MEM_AVAILABLE=$(grep MemAvailable /proc/meminfo 2>/dev/null | awk '{print $2}' || echo "0")

# Disco
DISK_USAGE=$(df . 2>/dev/null | tail -1 | awk '{print $5}' | tr -d '%' || echo "0")

# Criar JSON de métricas
cat > "$METRICS_FILE" << EOF
{
  "timestamp": $TIMESTAMP,
  "date": "$DATE",
  "clurg": {
    "version": "1.0",
    "total_commits": $TOTAL_COMMITS,
    "total_projects": $TOTAL_PROJECTS,
    "total_backups": $TOTAL_BACKUPS,
    "recent_commits_7d": $RECENT_COMMITS,
    "recent_backups_7d": $RECENT_BACKUPS
  },
  "storage": {
    "clurg_total_mb": $((CLURG_SIZE / 1024 / 1024)),
    "commits_mb": $((COMMITS_SIZE / 1024 / 1024)),
    "projects_mb": $((PROJECTS_SIZE / 1024 / 1024)),
    "backups_mb": $((BACKUPS_SIZE / 1024 / 1024)),
    "disk_usage_percent": $DISK_USAGE
  },
  "system": {
    "uptime_seconds": $UPTIME,
    "load_average": "$LOAD_AVG",
    "memory_total_kb": $MEM_TOTAL,
    "memory_available_kb": $MEM_AVAILABLE
  }
}
EOF

echo "✅ Métricas coletadas e salvas em $METRICS_FILE"

# Exibir resumo
echo ""
echo "📈 Resumo das Métricas:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📦 Commits: $TOTAL_COMMITS total, $RECENT_COMMITS nos últimos 7 dias"
echo "📁 Projetos: $TOTAL_PROJECTS"
echo "💾 Backups: $TOTAL_BACKUPS total, $RECENT_BACKUPS nos últimos 7 dias"
echo ""
echo "💽 Armazenamento:"
echo "  Clurg total: $((CLURG_SIZE / 1024 / 1024)) MB"
echo "  Commits: $((COMMITS_SIZE / 1024 / 1024)) MB"
echo "  Projetos: $((PROJECTS_SIZE / 1024 / 1024)) MB"
echo "  Backups: $((BACKUPS_SIZE / 1024 / 1024)) MB"
echo "  Uso do disco: $DISK_USAGE%"
echo ""
echo "🖥️  Sistema:"
echo "  Uptime: $((UPTIME / 3600))h $(((UPTIME % 3600) / 60))m"
echo "  Load average: $LOAD_AVG"
echo "  Memória: $((MEM_AVAILABLE / 1024))/$((MEM_TOTAL / 1024)) MB disponível"