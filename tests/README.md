# Testes do Clurg

Scripts de teste para validar funcionalidades do Clurg.

## Script de Teste Básico

O script `run_basic.sh` executa uma suíte de testes básicos para verificar se as funcionalidades principais estão funcionando.

### Como Executar

```bash
cd /home/Lucas/projetos/clurg
./tests/run_basic.sh
```

### O que é Testado

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

### Saída

O script mostra:
- ✓ Testes que passaram (verde)
- ✗ Testes que falharam (vermelho)
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

