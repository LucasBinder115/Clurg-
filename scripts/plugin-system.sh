# Clurg Plugin System
# Sistema de plugins para extensibilidade

PLUGINS_DIR=".clurg/plugins"
HOOKS_DIR=".clurg/hooks"

# Função para executar hooks de plugin
run_hook() {
    local hook_name=$1
    shift  # Remove hook_name dos argumentos
    
    local hook_script="$HOOKS_DIR/$hook_name"
    
    if [ -x "$hook_script" ]; then
        echo "🔌 Executando hook: $hook_name"
        "$hook_script" "$@"
    fi
    
    # Executar plugins que implementam este hook
    if [ -d "$PLUGINS_DIR" ]; then
        for plugin_dir in "$PLUGINS_DIR"/*/; do
            if [ -d "$plugin_dir" ]; then
                local plugin_hook="$plugin_dir/hooks/$hook_name"
                if [ -x "$plugin_hook" ]; then
                    echo "🔌 Executando plugin hook: $(basename "$plugin_dir")/$hook_name"
                    "$plugin_hook" "$@"
                fi
            fi
        done
    fi
}

# Função para instalar plugin
install_plugin() {
    local plugin_url=$1
    local plugin_name=$(basename "$plugin_url" .git)
    
    echo "📦 Instalando plugin: $plugin_name"
    
    mkdir -p "$PLUGINS_DIR"
    cd "$PLUGINS_DIR"
    
    if command -v git >/dev/null 2>&1; then
        git clone "$plugin_url" "$plugin_name"
        echo "✅ Plugin instalado: $plugin_name"
    else
        echo "❌ Git não encontrado. Instale git para instalar plugins."
        return 1
    fi
}

# Função para listar plugins instalados
list_plugins() {
    echo "🔌 Plugins instalados:"
    
    if [ ! -d "$PLUGINS_DIR" ]; then
        echo "  Nenhum plugin instalado"
        return
    fi
    
    local count=0
    for plugin_dir in "$PLUGINS_DIR"/*/; do
        if [ -d "$plugin_dir" ]; then
            local plugin_name=$(basename "$plugin_dir")
            local plugin_desc=""
            
            if [ -f "$plugin_dir/plugin.json" ]; then
                plugin_desc=$(grep '"description"' "$plugin_dir/plugin.json" | cut -d: -f2 | tr -d '",' || echo "")
            fi
            
            echo "  📦 $plugin_name - ${plugin_desc:-Sem descrição}"
            ((count++))
        fi
    done
    
    if [ $count -eq 0 ]; then
        echo "  Nenhum plugin instalado"
    fi
}

# Função para executar plugin diretamente
run_plugin() {
    local plugin_name=$1
    shift
    
    local plugin_script="$PLUGINS_DIR/$plugin_name/plugin.sh"
    
    if [ -x "$plugin_script" ]; then
        echo "🔌 Executando plugin: $plugin_name"
        "$plugin_script" "$@"
    else
        echo "❌ Plugin não encontrado ou não executável: $plugin_name"
        return 1
    fi
}