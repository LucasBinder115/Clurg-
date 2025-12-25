#!/bin/bash
# Wrapper para comandos de plugin do Clurg

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLURG_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$CLURG_ROOT"

# Carregar sistema de plugins
source scripts/plugin-system.sh

case "$1" in
    "list")
        list_plugins
        ;;
    "install")
        if [ -z "$2" ]; then
            echo "❌ Erro: URL do plugin necessária"
            echo "Uso: $0 install <url>"
            exit 1
        fi
        install_plugin "$2"
        ;;
    "run")
        if [ -z "$2" ]; then
            echo "❌ Erro: Nome do plugin necessário"
            echo "Uso: $0 run <plugin> [args...]"
            exit 1
        fi
        plugin_name="$2"
        shift 2
        run_plugin "$plugin_name" "$@"
        ;;
    *)
        echo "🔌 Gerenciador de Plugins Clurg"
        echo ""
        echo "Comandos:"
        echo "  list                    - Listar plugins instalados"
        echo "  install <url>           - Instalar plugin do repositório git"
        echo "  run <plugin> [args...]  - Executar plugin específico"
        echo ""
        echo "Plugins são armazenados em .clurg/plugins/"
        ;;
esac