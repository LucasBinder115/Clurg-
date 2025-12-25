#!/bin/bash
# Clurg Project Permissions Manager
# Gerencia permissões de projetos (público/privado)

set -e

echo "🔐 Clurg Project Permissions Manager"
echo "===================================="

# Verificar se estamos em um repositório Clurg
if [ ! -d ".clurg" ]; then
    echo "❌ Erro: Não é um repositório Clurg"
    exit 1
fi

# Arquivo de configuração
CONFIG_FILE=".clurg/security.conf"

# Função para listar projetos
list_projects() {
    echo "📁 Projetos encontrados:"
    if [ -d ".clurg/projects" ]; then
        ls -1 ".clurg/projects" | while read project; do
            if [ -d ".clurg/projects/$project" ]; then
                # Verificar se é público ou privado
                if grep -q "^PUBLIC_PROJECTS=" "$CONFIG_FILE" 2>/dev/null; then
                    public_projects=$(grep "^PUBLIC_PROJECTS=" "$CONFIG_FILE" | cut -d'=' -f2 | tr -d '"')
                    if [[ "$public_projects" == *"$project"* ]]; then
                        echo "  🌐 $project (público)"
                    else
                        echo "  🔒 $project (privado)"
                    fi
                else
                    echo "  🔒 $project (privado - padrão)"
                fi
            fi
        done
    else
        echo "  Nenhum projeto encontrado"
    fi
    echo ""
}

# Função para tornar projeto público
make_public() {
    local project=$1
    
    if [ ! -d ".clurg/projects/$project" ]; then
        echo "❌ Erro: Projeto '$project' não encontrado"
        exit 1
    fi
    
    echo "🌐 Tornando projeto '$project' público..."
    
    # Ler projetos públicos atuais
    current_public=""
    if grep -q "^PUBLIC_PROJECTS=" "$CONFIG_FILE" 2>/dev/null; then
        current_public=$(grep "^PUBLIC_PROJECTS=" "$CONFIG_FILE" | cut -d'=' -f2 | tr -d '"')
    fi
    
    # Adicionar projeto se não estiver na lista
    if [[ "$current_public" != *"$project"* ]]; then
        if [ -n "$current_public" ]; then
            new_public="$current_public,$project"
        else
            new_public="$project"
        fi
        
        # Atualizar configuração
        sed -i "s/^PUBLIC_PROJECTS=.*/PUBLIC_PROJECTS=\"$new_public\"/" "$CONFIG_FILE"
        echo "✅ Projeto '$project' agora é público"
    else
        echo "ℹ️ Projeto '$project' já é público"
    fi
}

# Função para tornar projeto privado
make_private() {
    local project=$1
    
    if [ ! -d ".clurg/projects/$project" ]; then
        echo "❌ Erro: Projeto '$project' não encontrado"
        exit 1
    fi
    
    echo "🔒 Tornando projeto '$project' privado..."
    
    # Ler projetos públicos atuais
    current_public=""
    if grep -q "^PUBLIC_PROJECTS=" "$CONFIG_FILE" 2>/dev/null; then
        current_public=$(grep "^PUBLIC_PROJECTS=" "$CONFIG_FILE" | cut -d'=' -f2 | tr -d '"')
    fi
    
    # Remover projeto da lista pública
    if [[ "$current_public" == *"$project"* ]]; then
        # Remover da lista
        new_public=$(echo "$current_public" | sed "s/,*$project,*//g" | sed 's/^,*//;s/,*$//')
        
        # Atualizar configuração
        sed -i "s/^PUBLIC_PROJECTS=.*/PUBLIC_PROJECTS=\"$new_public\"/" "$CONFIG_FILE"
        echo "✅ Projeto '$project' agora é privado"
    else
        echo "ℹ️ Projeto '$project' já é privado"
    fi
}

# Menu principal
case "${1:-list}" in
    "list")
        list_projects
        ;;
    "public")
        if [ -z "$2" ]; then
            echo "❌ Uso: $0 public <nome-do-projeto>"
            exit 1
        fi
        make_public "$2"
        ;;
    "private")
        if [ -z "$2" ]; then
            echo "❌ Uso: $0 private <nome-do-projeto>"
            exit 1
        fi
        make_private "$2"
        ;;
    "help"|"-h"|"--help")
        echo "Uso: $0 [comando] [argumentos]"
        echo ""
        echo "Comandos:"
        echo "  list                    - Lista todos os projetos e suas permissões"
        echo "  public <projeto>        - Torna um projeto público (acesso sem token)"
        echo "  private <projeto>       - Torna um projeto privado (requer token)"
        echo "  help                    - Mostra esta ajuda"
        echo ""
        echo "Exemplos:"
        echo "  $0 list"
        echo "  $0 public meu-projeto"
        echo "  $0 private projeto-secreto"
        ;;
    *)
        echo "❌ Comando desconhecido: $1"
        echo "Use '$0 help' para ver os comandos disponíveis"
        exit 1
        ;;
esac