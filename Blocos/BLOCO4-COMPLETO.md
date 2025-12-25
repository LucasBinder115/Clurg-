# BLOCO 4 — SEGURANÇA E ROBUSTEZ ✅

## Melhorias Implementadas

### ✅ 1. Path Traversal - Revisado e Melhorado

**Proteção implementada:**
- Rejeita `..` (path traversal clássico)
- Rejeita `/` (navegação de diretório)
- Rejeita `\` (backslash, Windows-style)
- Rejeita caracteres de controle (bytes 0x00-0x0F)

**Validações adicionais:**
- Verifica comprimento do nome do arquivo (0 < len <= 255)
- Verifica se arquivo existe antes de ler
- Verifica se é arquivo regular (não diretório, symlink, etc)

**Código:**
```c
if (strstr(log_name, "..") != NULL || 
    strchr(log_name, '/') != NULL ||
    strchr(log_name, '\\') != NULL ||
    strpbrk(log_name, "\x00...") != NULL) {
    // Rejeita requisição
}
```

### ✅ 2. Limitar Tamanho de Arquivos de Log

**Implementação:**
- Limite máximo: 1MB (1.048.576 bytes)
- Verifica tamanho do arquivo antes de ler usando `stat()`
- Retorna erro 413 "Payload Too Large" se exceder
- Trunca leitura se necessário durante processamento

**Benefícios:**
- Previne consumo excessivo de memória
- Previne DoS via arquivos muito grandes
- Resposta rápida mesmo com arquivos grandes

**Código:**
```c
if (st.st_size > 1024 * 1024) {
    send_response(client_fd, "413 Payload Too Large", ...);
    return;
}
```

### ✅ 3. Tratar Erro 404 Corretamente

**Melhorias:**
- Resposta HTTP 404 com HTML apropriado (não apenas texto)
- Mensagem clara para o usuário
- Link para voltar à página inicial
- Headers HTTP corretos (Content-Type)

**Implementações:**
- Rota não encontrada: 404 com HTML
- Arquivo de log não encontrado: 404 com HTML
- Mensagens consistentes e úteis

**Código:**
```c
send_response(client_fd, "404 Not Found", "text/html; charset=utf-8",
    "<html><body><h1>404 Not Found</h1><p>...</p><a href='/'>← Voltar</a></body></html>");
```

### ✅ 4. Validar Porta (Range Válido)

**Validação robusta:**
- Usa `strtol()` ao invés de `atoi()` (mais seguro)
- Valida que toda string foi convertida (sem caracteres extras)
- Range válido: 1-65535 (portas TCP válidas)
- Aviso para portas privilegiadas (< 1024)

**Implementação:**
```c
long port_long = strtol(argv[1], &endptr, 10);
if (*endptr != '\0' || port_long <= 0 || port_long > 65535) {
    // Erro: porta inválida
}
if (port < 1024) {
    // Aviso: requer root
}
```

**Testes:**
- ✅ Porta 8080: válida
- ✅ Porta 3000: válida  
- ❌ Porta 70000: rejeitada (fora do range)
- ❌ Porta "abc": rejeitada (não numérica)
- ❌ Porta "8080x": rejeitada (caracteres extras)

## Resumo das Melhorias

| Item | Status | Implementação |
|------|--------|---------------|
| Path Traversal | ✅ Melhorado | Múltiplas validações, caracteres de controle |
| Limite de tamanho | ✅ Implementado | 1MB máximo, verificação prévia |
| Erro 404 | ✅ Melhorado | HTML apropriado, mensagens claras |
| Validação de porta | ✅ Melhorado | strtol, range completo, avisos |

## Impacto

Essas melhorias aumentam significativamente a robustez e segurança do servidor web:

1. **Segurança**: Proteção contra ataques comuns (path traversal, DoS)
2. **Robustez**: Validações adequadas previnem crashes
3. **Usabilidade**: Mensagens de erro claras e úteis
4. **Confiabilidade**: Validações de entrada previnem comportamentos inesperados

## Notas

- As proteções são adequadas para uso educacional/local
- Para produção, revisão adicional de segurança seria necessária
- Todas as validações seguem boas práticas de segurança web

