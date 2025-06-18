#!/usr/bin/env bash
set -euo pipefail

ROOT="$HOME/Documents/RessourcesStages/Projets/VulRAG-Hybrid-System/data/tmp"
INSTANCES_DIR="$ROOT/instances"
CPG_JSON_DIR="$ROOT/cpg_json"
mkdir -p "$CPG_JSON_DIR"

export JAVA_OPTIONS="-Xms1g -Xmx2g"
LOG="$CPG_JSON_DIR/test.log"

# Test function on ONE instance
process_one () {
    local inst="$1"
    local id=$(basename "$inst")
    local out="$CPG_JSON_DIR/$id"

    echo "Processing $id..."
    mkdir -p "$out"
    echo "Created output dir: $out"

    for src in vuln patch; do
        local f="$inst/${src}.c"
        echo "Checking file: $f"
        
        if [[ -f "$f" ]]; then
            echo "  Found $src.c"
        else
            echo "  $src.c not found, skipping"
            continue
        fi

        local json_file="$out/${src}_cpg.json"
        if [[ -f "$json_file" ]]; then
            echo "  $src.c already exists, skipping"
            continue
        fi

        echo "  Processing $src.c..."
        # Just test the joern commands without full processing
        echo "  Would process: $f -> $json_file"
    done
    
    echo "Completed $id"
}

# Test on first instance only
first_inst=$(find "$INSTANCES_DIR" -mindepth 1 -maxdepth 1 -type d | head -n 1)
echo "Testing process_one on: $first_inst"
process_one "$first_inst"
