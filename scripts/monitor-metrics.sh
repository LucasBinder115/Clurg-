#!/bin/bash
# Clurg Metrics Monitor
# Monitora métricas ao longo do tempo e gera gráficos

set -e

echo "📈 Clurg Metrics Monitor"
echo "========================"

# Verificar se estamos em um repositório Clurg
if [ ! -d ".clurg" ]; then
    echo "❌ Erro: Não é um repositório Clurg"
    exit 1
fi

# Arquivos de histórico
METRICS_HISTORY=".clurg/metrics_history.json"
METRICS_DIR=".clurg/metrics"

# Criar diretório se não existir
mkdir -p "$METRICS_DIR"

# Função para coletar métricas atuais
collect_current() {
    echo "📊 Coletando métricas atuais..."
    ./scripts/collect-metrics.sh > /dev/null
}

# Função para salvar snapshot histórico
save_snapshot() {
    local timestamp=$(date +%s)
    local date=$(date '+%Y-%m-%d %H:%M:%S')
    
    if [ -f ".clurg/metrics.json" ]; then
        cp ".clurg/metrics.json" "$METRICS_DIR/snapshot_$timestamp.json"
        echo "💾 Snapshot salvo: $timestamp"
    fi
}

# Função para mostrar tendências
show_trends() {
    echo "📈 Analisando tendências..."
    
    # Contar snapshots disponíveis
    local snapshot_count=$(ls -1 "$METRICS_DIR"/snapshot_*.json 2>/dev/null | wc -l)
    
    if [ "$snapshot_count" -eq 0 ]; then
        echo "ℹ️ Nenhum histórico disponível. Execute '$0 collect' primeiro."
        return
    fi
    
    echo "📊 Histórico encontrado: $snapshot_count snapshots"
    
    # Analisar crescimento de commits
    echo ""
    echo "📦 Crescimento de Commits:"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    # Pegar os últimos 5 snapshots
    local recent_snapshots=$(ls -t "$METRICS_DIR"/snapshot_*.json | head -5)
    
    local prev_commits=0
    local first=true
    
    for snapshot in $recent_snapshots; do
        local commits=$(grep '"total_commits"' "$snapshot" | grep -o '[0-9]*' | head -1)
        local timestamp=$(basename "$snapshot" | sed 's/snapshot_//' | sed 's/\.json//')
        local date=$(date -d "@$timestamp" '+%m/%d %H:%M' 2>/dev/null || echo "unknown")
        
        if [ "$first" = true ]; then
            echo "  $date: $commits commits (inicial)"
            first=false
        else
            local growth=$((commits - prev_commits))
            if [ "$growth" -gt 0 ]; then
                echo "  $date: $commits commits (+$growth)"
            elif [ "$growth" -lt 0 ]; then
                echo "  $date: $commits commits ($growth)"
            else
                echo "  $date: $commits commits (sem mudança)"
            fi
        fi
        
        prev_commits=$commits
    done
    
    # Analisar uso de storage
    echo ""
    echo "💽 Crescimento de Armazenamento:"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    prev_mb=0
    first=true
    
    for snapshot in $recent_snapshots; do
        local mb=$(grep '"clurg_total_mb"' "$snapshot" | grep -o '[0-9]*' | head -1)
        local timestamp=$(basename "$snapshot" | sed 's/snapshot_//' | sed 's/\.json//')
        local date=$(date -d "@$timestamp" '+%m/%d %H:%M' 2>/dev/null || echo "unknown")
        
        if [ "$first" = true ]; then
            echo "  $date: ${mb} MB (inicial)"
            first=false
        else
            local growth=$((mb - prev_mb))
            if [ "$growth" -gt 0 ]; then
                echo "  $date: ${mb} MB (+${growth} MB)"
            elif [ "$growth" -lt 0 ]; then
                echo "  $date: ${mb} MB (${growth} MB)"
            else
                echo "  $date: ${mb} MB (sem mudança)"
            fi
        fi
        
        prev_mb=$mb
    done
}

# Função para gerar relatório semanal
generate_report() {
    echo "📋 Gerando relatório semanal..."
    
    local week_start=$(date -d 'last monday' +%Y-%m-%d 2>/dev/null || date +%Y-%m-%d)
    local report_file=".clurg/reports/weekly_$(date +%Y%m%d).md"
    
    mkdir -p ".clurg/reports"
    
    # Coletar métricas atuais
    collect_current
    
    # Gerar relatório
    cat > "$report_file" << EOF
# Relatório Semanal Clurg - $(date +%Y-%m-%d)

## 📊 Métricas Atuais

\`\`\`json
$(cat .clurg/metrics.json)
\`\`\`

## 📈 Tendências da Semana

### Commits
- Total: $(grep '"total_commits"' .clurg/metrics.json | grep -o '[0-9]*' | head -1)
- Nos últimos 7 dias: $(grep '"recent_commits_7d"' .clurg/metrics.json | grep -o '[0-9]*' | head -1)

### Armazenamento
- Total: $(grep '"clurg_total_mb"' .clurg/metrics.json | grep -o '[0-9]*' | head -1) MB
- Commits: $(grep '"commits_mb"' .clurg/metrics.json | grep -o '[0-9]*' | head -1) MB
- Projetos: $(grep '"projects_mb"' .clurg/metrics.json | grep -o '[0-9]*' | head -1) MB
- Backups: $(grep '"backups_mb"' .clurg/metrics.json | grep -o '[0-9]*' | head -1) MB

### Sistema
- Uptime: $(grep '"uptime_seconds"' .clurg/metrics.json | grep -o '[0-9]*' | head -1 | xargs -I {} echo "scale=1; {}/3600" | bc 2>/dev/null || echo "unknown") horas
- Load Average: $(grep '"load_average"' .clurg/metrics.json | grep -o '"[^"]*"' | tr -d '"')
- Memória disponível: $(($(grep '"memory_available_kb"' .clurg/metrics.json | grep -o '[0-9]*' | head -1) / 1024)) MB

## 🔍 Observações

- Relatório gerado automaticamente em $(date)
- Dados coletados do sistema Clurg
- Para visualizar dashboard: http://localhost:8080/metrics

---
*Relatório gerado por Clurg Metrics Monitor*
EOF

    echo "✅ Relatório salvo em: $report_file"
}

# Menu principal
case "${1:-help}" in
    "collect")
        collect_current
        save_snapshot
        ;;
    "trends")
        show_trends
        ;;
    "report")
        generate_report
        ;;
    "dashboard")
        echo "🌐 Dashboard disponível em: http://localhost:8080/metrics"
        echo "   (Certifique-se de que o servidor web está rodando)"
        ;;
    "help"|"-h"|"--help")
        echo "Uso: $0 [comando]"
        echo ""
        echo "Comandos:"
        echo "  collect  - Coleta métricas atuais e salva snapshot histórico"
        echo "  trends   - Mostra tendências baseadas no histórico"
        echo "  report   - Gera relatório semanal detalhado"
        echo "  dashboard- Mostra URL do dashboard web"
        echo "  help     - Mostra esta ajuda"
        echo ""
        echo "Exemplos:"
        echo "  $0 collect    # Coleta métricas"
        echo "  $0 trends     # Mostra crescimento"
        echo "  $0 report     # Gera relatório semanal"
        ;;
    *)
        echo "❌ Comando desconhecido: $1"
        echo "Use '$0 help' para ver os comandos disponíveis"
        exit 1
        ;;
esac