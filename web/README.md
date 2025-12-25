# Clurg Web Server

Servidor web simples para visualizar projetos, commits e logs de CI do Clurg.

## Como usar

```bash
# Compilar
make clurg-web

# Executar (porta padrão: 8080)
./bin/clurg-web

# Ou especificar uma porta diferente
./bin/clurg-web 3000
```

## Acessar

Abra no navegador: `http://localhost:8080`

## Funcionalidades

- **Página inicial**: Lista todos os logs de CI disponíveis
- **Visualização de logs**: Clique em qualquer log para ver o conteúdo completo
- **Interface simples**: HTML/CSS básico, sem dependências externas

## Rotas

- `GET /` - Página inicial com lista de logs
- `GET /logs/<nome-do-arquivo>` - Visualizar log específico

## Notas

O servidor web foi implementado do zero em C puro, seguindo a filosofia do projeto:
- Sem dependências externas
- Servidor HTTP simples usando sockets
- Interface web básica mas funcional

