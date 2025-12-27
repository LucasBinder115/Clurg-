#!/bin/bash
# Clurg Commit Script
set -e

MESSAGE="${1:-no message}"
AUTHOR="${USER:-unknown}"
TIMESTAMP=$(date +%Y-%m-%d\ %H:%M:%S)
ID=$(date +%Y%m%d%H%M%S)

# Structure check
if [ ! -d ".clurg" ]; then
  echo "Erro: Repositório não inicializado."
  exit 1
fi

mkdir -p .clurg/commits

echo "📦 Criando snapshot $ID..."

# Create tarball
TAR_FILE=".clurg/commits/$ID.tar.gz"
tar -czf "$TAR_FILE" --exclude .clurg .

# Calculate checksum
CHECKSUM=$(sha256sum "$TAR_FILE" | cut -d' ' -f1)
SIZE=$(stat -c%s "$TAR_FILE")

# Create Meta file (Key: Value format compatible with clone.c)
cat > ".clurg/commits/$ID.meta" <<EOF
id: $ID
timestamp: $TIMESTAMP
author: $AUTHOR
message: $MESSAGE
size_bytes: $SIZE
checksum: $CHECKSUM
EOF

# Update HEAD
echo "$ID" > .clurg/HEAD

echo "✅ Commit $ID realizado com sucesso!"
echo "   Mensagem: $MESSAGE"
echo "   Checksum: $CHECKSUM"
