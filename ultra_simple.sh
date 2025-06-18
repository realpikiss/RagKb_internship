#!/usr/bin/env bash
# Sans set -euo pipefail pour voir l'erreur

ROOT="$HOME/Documents/RessourcesStages/Projets/VulRAG-Hybrid-System/data/tmp"
INSTANCES_DIR="$ROOT/instances"

echo "INSTANCES_DIR: $INSTANCES_DIR"
echo "Testing find command..."

# Test étape par étape
echo "Step 1: find command"
find "$INSTANCES_DIR" -mindepth 1 -maxdepth 1 -type d | head -n 3

echo "Step 2: head command"
result=$(find "$INSTANCES_DIR" -mindepth 1 -maxdepth 1 -type d | head -n 1)
echo "First instance: '$result'"

echo "Step 3: test if directory"
if [[ -d "$result" ]]; then
    echo "Is directory: YES"
    ls -la "$result"
else
    echo "Is directory: NO"
fi
