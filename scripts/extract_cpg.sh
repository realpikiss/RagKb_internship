#!/usr/bin/env bash

# ===========================
# VulnRAG CPG Extraction Script
# ===========================
# This script extracts Code Property Graphs (CPGs) for all vulnerable/patched code pairs
# in a directory structure organized by CWE. It uses joern-parse and joern-export.
# Output is stored in a mirrored directory structure under the specified output directory.
# ===========================

# --- Argument Checking ---
# Ensure exactly two arguments are provided: input and output directories.
if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <CODE_FILES_DIR> <CPG_OUTPUT_DIR>"
    echo "  CODE_FILES_DIR: Directory containing CWE-* subdirectories with source files"
    echo "  CPG_OUTPUT_DIR: Directory where CPG extraction results will be stored"
    exit 1
fi

CODE_FILES_DIR="$1"
CPG_OUTPUT_DIR="$2"

# --- Input Directory Validation ---
# Check that the input directory exists and is a directory.
if [[ ! -d "$CODE_FILES_DIR" ]]; then
    echo "Error: CODE_FILES_DIR '$CODE_FILES_DIR' does not exist or is not a directory"
    exit 1
fi

# Create output directory if it doesn't exist.
mkdir -p "$CPG_OUTPUT_DIR"

# --- Environment Setup ---
# Set Java memory options for Joern and define log file location.
export JAVA_OPTIONS="-Xms1g -Xmx2g"
LOG="$CPG_OUTPUT_DIR/extract.log"

# --- Extraction Start Banner ---
echo "=== FULL CPG EXTRACTION ==="
echo "Input: $CODE_FILES_DIR"
echo "Output: $CPG_OUTPUT_DIR"
echo "Started: $(date)"

# --- CPG Extraction Function ---
# extract_cpg <source_file> <json_output> <log_prefix>
# Runs joern-parse and joern-export on a source file, moves the resulting JSON to output,
# and logs errors if any step fails.
extract_cpg() {
    local src_file="$1"
    local json_output="$2"
    local log_prefix="$3"
    
    # Create a temporary directory for intermediate files.
    local tmpdir=$(mktemp -d)
    local cpg_file="$tmpdir/code.cpg.bin"
    local export_dir="$tmpdir/export"
    
    # Step 1: Parse source file to CPG binary.
    if joern-parse --output "$cpg_file" "$src_file" 2>/dev/null; then
        # Step 2: Export CPG to GraphSON (JSON) format.
        if joern-export "$cpg_file" --repr=all --format=graphson --out="$export_dir" 2>/dev/null; then
            # Step 3: Find the exported JSON/GraphSON file.
            exported_file=$(find "$export_dir" -name "*.json" -o -name "*.graphson" 2>/dev/null | head -n 1)
            if [[ -n "$exported_file" && -f "$exported_file" ]]; then
                mv "$exported_file" "$json_output"
                echo "$log_prefix OK"
            else
                # No export file found after successful export step.
                echo "$log_prefix NO_EXPORT_FILE" >> "$LOG"
            fi
        else
            # Export step failed.
            echo "$log_prefix EXPORT_FAIL" >> "$LOG"
        fi
    else
        # Parsing step failed.
        echo "$log_prefix PARSE_FAIL" >> "$LOG"
    fi
    
    # Clean up temporary directory.
    rm -rf "$tmpdir" 2>/dev/null || true
}

# --- Count Total Vulnerable/Patch Pairs ---
# This loop counts the number of (vuln, patch) pairs to process for progress reporting.
total=0
for cwe_dir in "$CODE_FILES_DIR"/CWE-*; do
    if [[ -d "$cwe_dir" ]]; then
        for vuln_file in "$cwe_dir"/*_vuln.c; do
            if [[ -f "$vuln_file" ]]; then
                instance_id=$(basename "$vuln_file" _vuln.c)
                patch_file="$cwe_dir/${instance_id}_patch.c"
                # Only count if both vuln and patch files exist.
                [[ -f "$patch_file" ]] && ((total++))
            fi
        done
    fi
done

echo "Found $total pairs to process"
echo ""

# --- Main Extraction Loop ---
# For each CWE directory and each vuln/patch pair, extract CPGs if not already present.
count=0
start_time=$(date +%s)

for cwe_dir in "$CODE_FILES_DIR"/CWE-*; do
    if [[ -d "$cwe_dir" ]]; then
        cwe_name=$(basename "$cwe_dir")
        echo "Processing $cwe_name..."
        
        for vuln_file in "$cwe_dir"/*_vuln.c; do
            if [[ -f "$vuln_file" ]]; then
                instance_id=$(basename "$vuln_file" _vuln.c)
                patch_file="$cwe_dir/${instance_id}_patch.c"
                
                if [[ -f "$patch_file" ]]; then
                    ((count++))
                    
                    # Show progress every 50 pairs processed.
                    if (( count % 50 == 0 )); then
                        current_time=$(date +%s)
                        elapsed=$((current_time - start_time))
                        # Calculate processing rate (pairs per minute).
                        rate=$(echo "scale=2; $count / ($elapsed / 60)" | bc -l 2>/dev/null || echo "N/A")
                        echo "  [$count/$total] Progress: ${rate} pairs/min"
                    fi
                    
                    # Create output directory for this instance.
                    out_dir="$CPG_OUTPUT_DIR/$cwe_name/$instance_id"
                    mkdir -p "$out_dir"
                    
                    # Define output file paths for vuln and patch CPGs.
                    vuln_json="$out_dir/vuln_cpg.json"
                    patch_json="$out_dir/patch_cpg.json"
                    
                    # Only extract if output file does not already exist.
                    [[ ! -f "$vuln_json" ]] && extract_cpg "$vuln_file" "$vuln_json" "$instance_id/vuln"
                    [[ ! -f "$patch_json" ]] && extract_cpg "$patch_file" "$patch_json" "$instance_id/patch"
                fi
            fi
        done
    fi
done

# --- Final Results and Summary ---
end_time=$(date +%s)
total_time=$((end_time - start_time))
# Count all JSON files created (should be twice the number of pairs if all succeeded).
json_count=$(find "$CPG_OUTPUT_DIR" -name "*.json" 2>/dev/null | wc -l)

echo ""
echo "=== FINAL RESULTS ==="
echo "Finished: $(date)"
echo "Total time: ${total_time}s ($((total_time / 60))m)"
echo "Pairs processed: $count"
echo "CPG files created: $json_count"
echo "Expected files: $((count * 2))"
echo "Success rate: $(echo "scale=1; $json_count * 100 / ($count * 2)" | bc -l)%"

# --- Error Reporting ---
# If the log file exists and is not empty, print error statistics.
if [[ -f "$LOG" && -s "$LOG" ]]; then
    error_count=$(wc -l < "$LOG")
    echo "Errors logged: $error_count"
    echo ""
    echo "Error breakdown:"
    echo "  Parse failures: $(grep -c PARSE_FAIL "$LOG" 2>/dev/null || echo 0)"
    echo "  Export failures: $(grep -c EXPORT_FAIL "$LOG" 2>/dev/null || echo 0)"
    echo "  No export file: $(grep -c NO_EXPORT_FILE "$LOG" 2>/dev/null || echo 0)"
fi

echo ""
echo "Output directory: $CPG_OUTPUT_DIR"
echo "Log file: $LOG"