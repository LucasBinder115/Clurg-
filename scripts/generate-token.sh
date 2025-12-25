#!/bin/bash
# Clurg Token Generator
# Gera tokens seguros para autenticação

set -e

echo "🔐 Clurg Token Generator"
echo "========================"

# Verificar se estamos em um repositório Clurg
if [ ! -d ".clurg" ]; then
    echo "❌ Erro: Não é um repositório Clurg"
    exit 1
fi

# Função para gerar token seguro
generate_token() {
    if command -v openssl >/dev/null 2>&1; then
        # Usar openssl para gerar token seguro
        openssl rand -hex 32
    else
        # Fallback: usar /dev/urandom
        head -c 32 /dev/urandom | xxd -p -c 32 | tr -d '\n'
    fi
}

# Solicitar permissões
echo "Permissões disponíveis:"
echo "  r = read-only (GET requests)"
echo "  w = write (POST/PUT/DELETE requests)"
echo "  a = admin (todas as permissões)"
echo ""
read -p "Digite as permissões (ex: rw, r, a): " permissions

# Validar permissões
if [[ ! "$permissions" =~ ^[rwa]+$ ]]; then
    echo "❌ Erro: Permissões inválidas. Use apenas r, w, a"
    exit 1
fi

# Solicitar projetos (opcional)
read -p "Projetos permitidos (vazio = todos): " projects

# Gerar token
token=$(generate_token)
echo ""
echo "✅ Token gerado com sucesso!"
echo ""
echo "Token: $token"
echo "Permissões: $permissions"
echo "Projetos: ${projects:-todos}"
echo ""

# Arquivo de configuração
config_file=".clurg/security.conf"

# Verificar se arquivo existe
if [ ! -f "$config_file" ]; then
    echo "❌ Arquivo de configuração não encontrado: $config_file"
    exit 1
fi

# Adicionar token à configuração
if [ -n "$projects" ]; then
    token_entry="$token:$permissions:$projects"
else
    token_entry="$token:$permissions:*"
fi

# Backup do arquivo original
cp "$config_file" "${config_file}.backup"

# Adicionar token (remover aspas se existir)
sed -i "s/^TOKENS=.*/TOKENS=\"$token_entry\"/" "$config_file"

echo "📝 Token adicionado à configuração"
echo ""
echo "Para usar o token, inclua no header das requisições:"
echo "Authorization: Bearer $token"
echo ""
echo "Exemplo curl:"
echo "curl -H \"Authorization: Bearer $token\" http://localhost:8080/projects"
echo ""
echo "⚠️  IMPORTANTE: Guarde este token em local seguro!"
echo "   Ele concede acesso aos seus projetos Clurg."