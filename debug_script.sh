#!/usr/bin/env bash
set -euo pipefail

ROOT="$HOME/Documents/RessourcesStages/Projets/VulRAG-Hybrid-System/data/tmp"
INSTANCES_DIR="$ROOT/instances"
CPG_JSON_DIR="$ROOT/cpg_json"

echo "Creating output directory..."
mkdir -p "$CPG_JSON_DIR"

echo "Counting instances..."
total=$(find "$INSTANCES_DIR" -mindepth 1 -maxdepth 1 -type d | wc -l)
echo "Found $total instances"

echo "Testing first instance..."
first_inst=$(find "$INSTANCES_DIR" -mindepth 1 -maxdepth 1 -type d | head -n 1)
echo "First instance: $first_inst"

# Test just the first one
if [[ -d "$first_inst" ]]; then
    echo "Testing directory check: OK"
    echo "Contents of first instance:"
    ls -la "$first_inst"
else
    echo "ERROR: First instance is not a directory"
fi

echo "Debug completed successfully"
