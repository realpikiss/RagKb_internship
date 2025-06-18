#!/usr/bin/env bash
set -euo pipefail

# Same configuration
ROOT="$HOME/Documents/RessourcesStages/Projets/VulRAG-Hybrid-System/data/tmp"
INSTANCES_DIR="$ROOT/instances"
CPG_JSON_DIR="$ROOT/cpg_json"
mkdir -p "$CPG_JSON_DIR"

export JAVA_OPTIONS="-Xms1g -Xmx2g"
LOG="$CPG_JSON_DIR/extract.log"

# EXACT same function from successful test
process_one () {
    local inst="$1"
    local id=$(basename "$inst")
    local out="$CPG_JSON_DIR/$id"

    echo "Processing $id..."
    mkdir -p "$out"

    for src in vuln patch; do
        local f="$inst/${src}.c"
        [[ -f "$f" ]] || continue

        local json_file="$out/${src}_cpg.json"
        if [[ -f "$json_file" ]]; then
            echo "  $src.c EXISTS"
            continue
        fi

        local tmpdir=$(mktemp -d)
        local cpg_file="$tmpdir/${src}.cpg.bin"
        local export_dir="$tmpdir/export"

        if joern-parse --output "$cpg_file" "$f" 2>/dev/null; then
            if joern-export "$cpg_file" --repr=cpg --format=graphson --out="$export_dir" 2>/dev/null; then
                exported_file=$(find "$export_dir" -name "*.json" -o -name "*.graphson" | head -n 1)
                if [[ -n "$exported_file" && -f "$exported_file" ]]; then
                    mv "$exported_file" "$json_file"
                    echo "  $src.c OK"
                else
                    echo "  $src.c NO_EXPORT_FILE" >> "$LOG"
                fi
            else
                echo "  $src.c EXPORT_FAIL" >> "$LOG"
            fi
        else
            echo "  $src.c PARSE_FAIL" >> "$LOG"
        fi

        rm -rf "$tmpdir" 2>/dev/null || true
    done
}

echo "Sequential processing (no parallel)..."
echo "Input: $INSTANCES_DIR"
echo "Output: $CPG_JSON_DIR"

# SEQUENTIAL LOOP instead of parallel
count=0
total=$(find "$INSTANCES_DIR" -mindepth 1 -maxdepth 1 -type d | wc -l)

for inst in "$INSTANCES_DIR"/*; do
    if [[ -d "$inst" ]]; then
        ((count++))
        echo "[$count/$total] Processing..."
        process_one "$inst"
    fi
done

echo "Sequential processing completed!"