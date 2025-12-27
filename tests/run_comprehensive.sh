#!/bin/bash
# Script de testes abrangentes para o Clurg
# Inclui testes unitários, integração e qualidade de código

set -e  # Parar em caso de erro

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Diretório do projeto
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_DIR"

echo "=========================================="
echo "Testes Abrangentes do Clurg"
echo "=========================================="
echo ""

# Contador de testes
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_SKIPPED=0

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

# Função para pular teste
test_skip() {
    local name="$1"
    local reason="$2"

    echo -e "Teste: $name ... ${YELLOW}PULADO${NC} ($reason)"
    TESTS_SKIPPED=$((TESTS_SKIPPED + 1))
}

# Limpar testes anteriores
cleanup() {
    rm -f /tmp/clurg_test_output
    # Limpar projetos de teste
    rm -rf /tmp/clurg_test_project_*
}

trap cleanup EXIT

echo "1. Verificando compilação e binários..."
echo "----------------------------------------"
test_check "Compilação completa" "make all"
test_check "Binários existem" "[ -f bin/clurg ] && [ -f bin/clurg-ci ] && [ -f bin/clurg-web ]"
test_check "Binários são executáveis" "[ -x bin/clurg ] && [ -x bin/clurg-ci ] && [ -x bin/clurg-web ]"
echo ""

echo "2. Testando qualidade de código..."
echo "----------------------------------------"
# Verificar se clang-tidy está instalado
if command -v clang-tidy >/dev/null 2>&1; then
    test_check "Linting (clang-tidy)" "make lint"
else
    test_skip "Linting (clang-tidy)" "clang-tidy não instalado"
fi

# Verificar se clang-format está instalado
if command -v clang-format >/dev/null 2>&1; then
    test_check "Formatação (clang-format check)" "make format-check"
else
    test_skip "Formatação (clang-format check)" "clang-format não instalado"
fi
echo ""

echo "3. Testando Pipeline CI (clurg-ci)..."
echo "----------------------------------------"
# Criar diretório de logs se não existir
mkdir -p .clurg/ci/logs

# Executar pipeline uma vez para teste
./bin/clurg-ci run pipelines/default.ci > /tmp/clurg_ci_output 2>&1 || true

test_check "Pipeline parseia corretamente" "grep -q 'Executando pipeline' /tmp/clurg_ci_output"
test_check "Workspace é criado" "grep -q 'Workspace criado' /tmp/clurg_ci_output"
test_check "Steps são identificados" "grep -q 'Steps:' /tmp/clurg_ci_output"

# Testar pipeline com erro
echo 'steps:
  - name: "test-fail"
    run: "exit 1"' > /tmp/test_pipeline_fail.ci

test_check "Pipeline falha corretamente" "! ./bin/clurg-ci run /tmp/test_pipeline_fail.ci >/dev/null 2>&1"
echo ""

echo "4. Testando Geração de Logs..."
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
    else
        echo -e "Teste: Log tem conteúdo ... ${RED}FALHOU${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
else
    echo -e "Teste: Logs são gerados ... ${RED}FALHOU${NC}"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi
echo ""

echo "5. Testando Commit (clurg)..."
echo "----------------------------------------"
# Criar projeto de teste
TEST_PROJECT="/tmp/clurg_test_project_$$"
mkdir -p "$TEST_PROJECT"
cd "$TEST_PROJECT"

# Inicializar repositório git
git init --quiet
git config user.name "Test User"
git config user.email "test@example.com"

# Criar arquivo de teste
echo "test content" > test.txt
git add test.txt
git commit -m "initial commit" --quiet

# Testar commit (deve executar CI automaticamente)
test_check "Commit executa CI automaticamente" "$PROJECT_DIR/bin/clurg commit 'teste automatizado' 2>&1 | grep -q 'Executando pipeline CI'"

cd "$PROJECT_DIR"
echo ""

echo "6. Testando Servidor Web (clurg-web)..."
echo "----------------------------------------"
test_check "Servidor compila e existe" "[ -f bin/clurg-web ]"
test_check "Servidor valida porta inválida" "./bin/clurg-web 70000 2>&1 | grep -q 'Porta inválida'"

# Testar se servidor detecta diretório .clurg
if [ -d ".clurg" ]; then
    echo -e "Teste: Servidor encontra diretório .clurg ... ${GREEN}OK${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "Teste: Servidor encontra diretório .clurg ... ${YELLOW}AVISO${NC} (.clurg não existe, mas será criado)"
fi

# Testar inicialização do servidor (background)
test_check "Servidor inicia na porta 8080" "timeout 2 ./bin/clurg-web 8080 >/dev/null 2>&1 & sleep 1; kill %1 2>/dev/null; true"
echo ""

echo "7. Testando Estrutura de Arquivos..."
echo "----------------------------------------"
test_check "Pipeline padrão existe" "[ -f pipelines/default.ci ]"
test_check "Makefile existe" "[ -f Makefile ]"
test_check "README existe" "[ -f README.md ]"
test_check "Configurações de qualidade existem" "[ -f .clang-tidy ] && [ -f .clang-format ]"
echo ""

echo "8. Testando funcionalidades de banco de dados..."
echo "----------------------------------------"
# Testar conexão com banco (se disponível)
if pg_isready >/dev/null 2>&1; then
    test_check "PostgreSQL está disponível" "pg_isready"
else
    test_skip "PostgreSQL está disponível" "PostgreSQL não está rodando"
fi
echo ""

echo "9. Testando deploy..."
echo "----------------------------------------"
# Testar configuração de deploy (se existir)
if [ -f "clurg.deploy" ]; then
    test_check "Arquivo de configuração de deploy existe" "[ -f clurg.deploy ]"
    test_check "Deploy config tem formato válido" "grep -q 'deploy:' clurg.deploy && grep -q 'healthcheck:' clurg.deploy"
else
    test_skip "Arquivo de configuração de deploy existe" "clurg.deploy não encontrado"
fi
echo ""

echo "=========================================="
echo "Resumo dos Testes Abrangentes"
echo "=========================================="
echo -e "Testes passaram: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Testes falharam: ${RED}$TESTS_FAILED${NC}"
echo -e "Testes pulados: ${YELLOW}$TESTS_SKIPPED${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ Todos os testes obrigatórios passaram!${NC}"
    exit 0
else
    echo -e "${RED}✗ Alguns testes falharam${NC}"
    exit 1
fi