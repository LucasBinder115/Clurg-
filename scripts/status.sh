#!/bin/bash
# Clurg Status Script

if [ ! -d ".clurg" ]; then
    echo "❌ Repositório não inicializado."
    exit 1
fi

if [ -f ".clurg/HEAD" ]; then
    HEAD=$(cat .clurg/HEAD)
    if [ -z "$HEAD" ]; then
        echo "📂 Repositório inicializado (sem commits)."
    else
        echo "🔖 HEAD atual: $HEAD"
        # Optional: Check for modifications using tar diff or similar?
        # For simplicity/speed, we just show HEAD.
        # "Indicar estado limpo ou modificado (simplificado)" - TODO
        # Simpler approach: check if any file is newer than HEAD commit timestamp?
        # Maybe too complex for bash script right now.
        echo "📂 Working directory: $(pwd)"
    fi
else
    echo "⚠️  HEAD não encontrado (estado inconsistente)."
fi
