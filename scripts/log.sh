#!/bin/bash
# Clurg Log Script

if [ ! -d ".clurg/commits" ]; then
    echo "❌ Sem commits encontrados."
    exit 0
fi

echo "📜 Histórico de Commits:"
echo "========================"

# Listar .meta files, ordenados reverso (mais novo primeiro)
ls -1r .clurg/commits/*.meta 2>/dev/null | while read metafile; do
    id=$(grep "^id: " "$metafile" | cut -d: -f2- | xargs)
    date=$(grep "^timestamp: " "$metafile" | cut -d: -f2- | xargs)
    author=$(grep "^author: " "$metafile" | cut -d: -f2- | xargs)
    msg=$(grep "^message: " "$metafile" | cut -d: -f2- | xargs)
    
    echo "Commit: $id"
    echo "Data:   $date"
    echo "Autor:  $author"
    echo "    $msg"
    echo ""
done
