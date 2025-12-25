#!/bin/sh
# Script para verificar permissões do Clurg
# Aprendizado: permissões Unix, chmod, chown

echo "=== Verificação de Permissões Clurg ==="

# Verificar se scripts são executáveis
if [ ! -x "scripts/commit.sh" ]; then
    echo "❌ scripts/commit.sh não é executável"
    echo "   Execute: chmod +x scripts/commit.sh"
else
    echo "✅ scripts/commit.sh executável"
fi

# Verificar binários
for bin in bin/clurg bin/clurg-ci bin/clurg-web; do
    if [ -f "$bin" ]; then
        if [ ! -x "$bin" ]; then
            echo "❌ $bin não é executável"
        else
            echo "✅ $bin executável"
        fi
    else
        echo "⚠️  $bin não encontrado (compile com make)"
    fi
done

# Verificar .clurg
if [ -d ".clurg" ]; then
    echo "✅ .clurg existe"
    ls -ld .clurg
else
    echo "❌ .clurg não existe"
fi

echo ""
echo "=== Dicas de Permissões ==="
echo "- Scripts: chmod +x scripts/*"
echo "- Binários: make cria com permissões corretas"
echo "- .clurg: mkdir -p .clurg (755 por padrão)"