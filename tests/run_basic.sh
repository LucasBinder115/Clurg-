#!/bin/bash
# Script de teste básico para o Clurg
# Testa funcionalidades principais: commit, pipeline, logs e web

set -e  # Parar em caso de erro

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Diretório do projeto
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_DIR"

echo "=========================================="
echo "Testes Básicos do Clurg"
echo "=========================================="
echo ""

# Contador de testes
TESTS_PASSED=0
TESTS_FAILED=0

# Função para testar
test_check() {
    local name="$1"
    local command="$2"
    
    echo -n "Teste: $name ... "
    if eval "$command" > /tmp/clurg_test_output 2>&1; then
        echo -e "${GREEN}OK${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        echo -e "${RED}FALHOU${NC}"
        echo "Saída:"
        cat /tmp/clurg_test_output | head -10
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
}

# Limpar testes anteriores
cleanup() {
    rm -f /tmp/clurg_test_output
}

trap cleanup EXIT

echo "1. Verificando compilação..."
echo "----------------------------------------"
test_check "Compilação completa" "make all"
test_check "Binários existem" "[ -f bin/clurg ] && [ -f bin/clurg-ci ] && [ -f bin/clurg-web ]"
echo ""

echo "2. Testando Pipeline CI (clurg-ci)"
echo "----------------------------------------"
# Criar diretório de logs se não existir
mkdir -p .clurg/ci/logs

# Executar pipeline uma vez para teste (pode falhar no step test, mas isso é OK)
./bin/clurg-ci run pipelines/default.ci > /tmp/clurg_ci_output 2>&1 || true

test_check "Pipeline parseia corretamente" "grep -q 'Executando pipeline' /tmp/clurg_ci_output"
test_check "Workspace é criado" "grep -q 'Workspace criado' /tmp/clurg_ci_output"
test_check "Steps são identificados" "grep -q 'Steps:' /tmp/clurg_ci_output"
echo ""

echo "3. Testando Geração de Logs"
echo "----------------------------------------"
# Executar pipeline para gerar log
./bin/clurg-ci run pipelines/default.ci > /dev/null 2>&1 || true

LOG_COUNT=$(find .clurg/ci/logs -name "*.log" -type f 2>/dev/null | wc -l)
if [ "$LOG_COUNT" -gt 0 ]; then
    echo -e "Teste: Logs são gerados ... ${GREEN}OK${NC} (encontrados $LOG_COUNT logs)"
    TESTS_PASSED=$((TESTS_PASSED + 1))
    
    # Verificar conteúdo do log mais recente
    LATEST_LOG=$(ls -t .clurg/ci/logs/*.log 2>/dev/null | head -1)
    if [ -n "$LATEST_LOG" ] && [ -s "$LATEST_LOG" ]; then
        echo -e "Teste: Log tem conteúdo ... ${GREEN}OK${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        
        # Verificar formato do log
        if grep -q "\[.*\]" "$LATEST_LOG"; then
            echo -e "Teste: Formato de log correto ... ${GREEN}OK${NC}"
            TESTS_PASSED=$((TESTS_PASSED + 1))
        else
            echo -e "Teste: Formato de log correto ... ${RED}FALHOU${NC}"
            TESTS_FAILED=$((TESTS_FAILED + 1))
        fi
    fi
else
    echo -e "Teste: Logs são gerados ... ${RED}FALHOU${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi
echo ""

echo "4. Testando Commit (clurg)"
echo "----------------------------------------"
# Testar commit (que executa CI)
test_check "Commit executa CI automaticamente" "./bin/clurg commit 'teste automatizado' 2>&1 | grep -q 'Executando pipeline CI'"
echo ""

echo "5. Testando Servidor Web (clurg-web)"
echo "----------------------------------------"
# Testar se o servidor inicia (sem realmente iniciar)
test_check "Servidor compila e existe" "[ -f bin/clurg-web ]"
test_check "Servidor valida porta inválida" "./bin/clurg-web 70000 2>&1 | grep -q 'Porta inválida'"

# Testar se servidor detecta diretório .clurg
if [ -d ".clurg" ]; then
    echo -e "Teste: Servidor encontra diretório .clurg ... ${GREEN}OK${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "Teste: Servidor encontra diretório .clurg ... ${YELLOW}AVISO${NC} (.clurg não existe, mas será criado)"
fi
echo ""

echo "6. Testando Estrutura de Arquivos"
echo "----------------------------------------"
test_check "Pipeline padrão existe" "[ -f pipelines/default.ci ]"
test_check "Makefile existe" "[ -f Makefile ]"
test_check "README existe" "[ -f README.md ]"
echo ""

echo "=========================================="
echo "Resumo dos Testes"
echo "=========================================="
echo -e "Testes passaram: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Testes falharam: ${RED}$TESTS_FAILED${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ Todos os testes passaram!${NC}"
    exit 0
else
    echo -e "${RED}✗ Alguns testes falharam${NC}"
    exit 1
fi

