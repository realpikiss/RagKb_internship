#!/usr/bin/env bash


ROOT="$HOME/Documents/RessourcesStages/Projets/VulRAG-Hybrid-System/data/tmp"
INSTANCES_DIR="$ROOT/instances"
CPG_JSON_DIR="$ROOT/cpg_json"
mkdir -p "$CPG_JSON_DIR"

export JAVA_OPTIONS="-Xms1g -Xmx2g"
LOG="$CPG_JSON_DIR/extract.log"

echo "Sequential CPG extraction starting..."
echo "Input: $INSTANCES_DIR"
echo "Output: $CPG_JSON_DIR"
echo "Log: $LOG"
echo ""

process_one () {
    local inst="$1"
    local id=$(basename "$inst")
    local out="$CPG_JSON_DIR/$id"

    mkdir -p "$out"

    for src in vuln patch; do
        local f="$inst/${src}.c"
        if [[ ! -f "$f" ]]; then
            continue
        fi

        local json_file="$out/${src}_cpg.json"
        if [[ -f "$json_file" ]]; then
            echo "$id/$src EXISTS"
            continue
        fi

        local tmpdir=$(mktemp -d)
        local cpg_file="$tmpdir/${src}.cpg.bin"
        local export_dir="$tmpdir/export"

        echo "Processing $id/$src..."
        
        if joern-parse --output "$cpg_file" "$f" 2>/dev/null; then
            if joern-export "$cpg_file" --repr=cpg --format=graphson --out="$export_dir" 2>/dev/null; then
                exported_file=$(find "$export_dir" -name "*.json" -o -name "*.graphson" 2>/dev/null | head -n 1)
                if [[ -n "$exported_file" && -f "$exported_file" ]]; then
                    mv "$exported_file" "$json_file"
                    echo "$id/$src OK"
                else
                    echo "$id/$src NO_EXPORT_FILE" >> "$LOG"
                fi
            else
                echo "$id/$src EXPORT_FAIL" >> "$LOG"
            fi
        else
            echo "$id/$src PARSE_FAIL" >> "$LOG"
        fi

        rm -rf "$tmpdir" 2>/dev/null || true
    done
}

# Count total instances
total=$(find "$INSTANCES_DIR" -mindepth 1 -maxdepth 1 -type d 2>/dev/null | wc -l)
echo "Found $total instances to process"
echo ""

# Process all instances sequentially
count=0
for inst in "$INSTANCES_DIR"/*; do
    if [[ -d "$inst" ]]; then
        ((count++))
        echo "[$count/$total] Processing $(basename "$inst")..."
        process_one "$inst"
        
        # Show progress every 100 instances
        if (( count % 100 == 0 )); then
            echo ""
            echo "Progress: $count/$total completed"
            echo "Files created so far: $(find "$CPG_JSON_DIR" -name "*.json" 2>/dev/null | wc -l)"
            echo ""
        fi
    fi
done

echo ""
echo "=== FINAL RESULTS ==="
json_count=$(find "$CPG_JSON_DIR" -name "*.json" -o -name "*.graphson" 2>/dev/null | wc -l)
dir_count=$(find "$CPG_JSON_DIR" -mindepth 1 -type d 2>/dev/null | wc -l)

echo "Directories created: $dir_count"
echo "JSON files generated: $json_count"

if [[ -f "$LOG" && -s "$LOG" ]]; then
    error_count=$(wc -l < "$LOG")
    echo "Errors: $error_count (see $LOG)"
    echo "First few errors:"
    head -n 5 "$LOG"
else
    echo "No errors logged"
fi

echo ""
echo "Sample files created:"
find "$CPG_JSON_DIR" -name "*.json" 2>/dev/null | head -n 5
