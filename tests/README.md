# Testes do Clurg

Scripts de teste para validar funcionalidades do Clurg.

## Scripts de Teste

### Testes Básicos (`run_basic.sh`)

O script `run_basic.sh` executa uma suíte de testes básicos para verificar se as funcionalidades principais estão funcionando.

### Testes Abrangentes (`run_comprehensive.sh`)

O script `run_comprehensive.sh` executa testes mais completos incluindo:
- Testes unitários e de integração
- Validação de qualidade de código (linting e formatação)
- Testes de funcionalidades avançadas
- Verificação de estrutura de arquivos

## Como Executar

```bash
cd /home/Lucas/projetos/clurg

# Testes básicos
./tests/run_basic.sh

# Testes abrangentes
./tests/run_comprehensive.sh

# Via Makefile
make test-basic    # Testes básicos
make test          # Testes abrangentes
make quality       # Linting + formatação + testes
```

## O que é Testado

### Testes Básicos

1. **Compilação**
   - Verifica se `make all` compila sem erros
   - Verifica se todos os binários são criados

2. **Pipeline CI (clurg-ci)**
   - Execução do pipeline padrão
   - Parsing correto do arquivo `.ci`
   - Criação de workspace isolado

3. **Geração de Logs**
   - Verifica se logs são criados em `.clurg/ci/logs/`
   - Verifica se logs têm conteúdo
   - Verifica formato do log (timestamp, status)

4. **Commit (clurg)**
   - Verifica se commit executa CI automaticamente
   - Integração entre clurg e clurg-ci

5. **Servidor Web (clurg-web)**
   - Verifica se servidor compila
   - Validação de porta inválida
   - Detecção de diretório `.clurg`

6. **Estrutura de Arquivos**
   - Verifica existência de arquivos essenciais
   - Pipeline padrão, Makefile, README

### Testes Abrangentes (Adicionais)

7. **Qualidade de Código**
   - Linting com clang-tidy
   - Verificação de formatação com clang-format

8. **Funcionalidades Avançadas**
   - Testes de pipeline com falha
   - Inicialização do servidor web
   - Conectividade com PostgreSQL
   - Validação de configuração de deploy

## Saída

Os scripts mostram:
- ✓ Testes que passaram (verde)
- ✗ Testes que falharam (vermelho)
- ⚠ Testes pulados (amarelo, quando dependências não estão disponíveis)
- Resumo final com contagem de testes

### Notas

- Os testes são **manuais** e **não automatizados** no sentido de CI
- Focam em verificar que funcionalidades básicas funcionam
- Não são testes unitários completos, mas validações de integração

### Requisitos

- Bash
- Binários compilados (execute `make all` primeiro)
- Diretório `.clurg/ci/logs/` será criado automaticamente se necessário

### Exemplo de Execução

```
==========================================
Testes Básicos do Clurg
==========================================

1. Verificando compilação...
----------------------------------------
Teste: Compilação completa ... OK
Teste: Binários existem ... OK

2. Testando Pipeline CI (clurg-ci)
----------------------------------------
Teste: Pipeline padrão executa ... OK
...

==========================================
Resumo dos Testes
==========================================
Testes passaram: 15
Testes falharam: 0

✓ Todos os testes passaram!
```

