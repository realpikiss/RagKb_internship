#!/usr/bin/env bash

# ===========================
# VulnRAG Vulnerable CPG Extraction Script (VULN ONLY)
# ===========================
# This script extracts Code Property Graphs (CPGs) ONLY for vulnerable code files
# (*_vuln.c pattern) in a directory structure organized by CWE.
# Uses joern-parse and joern-export with kernel stub preprocessing.
# ===========================

# --- Argument Checking ---
if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <CODE_FILES_DIR> <CPG_OUTPUT_DIR>"
    echo "  CODE_FILES_DIR: Directory containing CWE-* subdirectories with *_vuln.c files"
    echo "  CPG_OUTPUT_DIR: Directory where vulnerable CPG files will be stored"
    exit 1
fi

# Resolve project root
if root_dir="$(git rev-parse --show-toplevel 2>/dev/null)"; then
  project_dir="$root_dir"
else
  script_dir_init="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
  project_dir="$(cd "$script_dir_init/../.." && pwd -P)"
fi

CODE_FILES_DIR="$1"
CPG_OUTPUT_DIR="$2"
LOG_DIR="$project_dir/logs"

# --- Input validation ---
if [[ ! -d "$CODE_FILES_DIR" ]]; then
    echo "❌ Error: CODE_FILES_DIR '$CODE_FILES_DIR' does not exist"
    exit 1
fi

mkdir -p "$CPG_OUTPUT_DIR" "$LOG_DIR"

# --- Environment setup ---
export JAVA_OPTIONS="-Xms1g -Xmx2g"
LOG="$LOG_DIR/extract_vuln_cpg.log"

# Clear previous log
> "$LOG"

echo "======================================"
echo "🦠 VULNERABLE CPG EXTRACTION ONLY"
echo "======================================"
echo "Project root: $project_dir"
echo "Input: $CODE_FILES_DIR"
echo "Output: $CPG_OUTPUT_DIR"
echo "Logs: $LOG"
echo "Target: *_vuln.c files ONLY"
echo "Started: $(date)"
echo ""

# --- CPG Extraction Function for Vulnerable Files ---
extract_vuln_cpg() {
    local src_file="$1"
    local json_output="$2"
    local log_prefix="$3"
    
    # Create temporary directory
    local tmpdir=$(mktemp -d)
    local cpg_file="$tmpdir/code.cpg.bin"
    local export_dir="$tmpdir/export" 
    local stub_file="$tmpdir/kernel_stub.h"
    local wrapped_file="$tmpdir/wrapped.c"
    local preprocessed_file="$tmpdir/preprocessed.c"
    
    # Copy kernel stub
    local script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
    cp "$script_dir/../config/kernel_stub.h" "$stub_file"
    
    # Step 1: Wrap source with stub include
    {
        echo '#include "kernel_stub.h"'
        echo ""
        cat "$src_file"
    } > "$wrapped_file"
    
    # Step 2: Preprocess to flatten macros
    if gcc -E -P -I"$tmpdir" \
        -D__user= -D__kernel= -D__iomem= -D__init= -D__exit= \
        -D__initdata= -D__devinit= -D__devexit= -D__packed= \
        -D__aligned\(x\)= -D__maybe_unused= -D__always_inline= \
        -D__force= -D__bitwise= -D__rcu= -D__percpu= \
        -D__read_mostly= -D__cold= -D__hot= -D__weak= \
        -D__section\(x\)= -D__visible= -D__printf\(a,b\)= \
        -D__scanf\(a,b\)= -D__noreturn= -D__pure= -D__const= \
        -D__must_check= -D__deprecated= -D__used= -D__unused= \
        -D__noinline= -D__cacheline_aligned= \
        -Dlikely\(x\)=\(x\) -Dunlikely\(x\)=\(x\) \
        -Dbarrier\(\)= -Dsmp_mb\(\)= -Dsmp_rmb\(\)= -Dsmp_wmb\(\)= \
        "$wrapped_file" > "$preprocessed_file" 2>/dev/null; then
        
        # Step 3: Parse to CPG
        if joern-parse --output "$cpg_file" "$preprocessed_file" 2>/dev/null; then
            # Step 4: Export CPG to JSON
            if joern-export "$cpg_file" --repr=all --format=graphson --out="$export_dir" 2>/dev/null; then
                # Step 5: Move exported file
                exported_file=$(find "$export_dir" -name "*.json" -o -name "*.graphson" 2>/dev/null | head -n 1)
                if [[ -n "$exported_file" && -f "$exported_file" ]]; then
                    mv "$exported_file" "$json_output"
                    echo "✅ $log_prefix OK"
                    echo "$log_prefix SUCCESS" >> "$LOG"
                else
                    echo "❌ $log_prefix NO_EXPORT"
                    echo "$log_prefix NO_EXPORT_FILE" >> "$LOG"
                fi
            else
                echo "❌ $log_prefix EXPORT_FAIL"
                echo "$log_prefix EXPORT_FAIL" >> "$LOG"
            fi
        else
            echo "❌ $log_prefix PARSE_FAIL"
            echo "$log_prefix PARSE_FAIL_PREPROCESSED" >> "$LOG"
        fi
    else
        # Fallback: try original file without preprocessing
        if joern-parse --output "$cpg_file" "$src_file" 2>/dev/null; then
            if joern-export "$cpg_file" --repr=all --format=graphson --out="$export_dir" 2>/dev/null; then
                exported_file=$(find "$export_dir" -name "*.json" -o -name "*.graphson" 2>/dev/null | head -n 1)
                if [[ -n "$exported_file" && -f "$exported_file" ]]; then
                    mv "$exported_file" "$json_output"
                    echo "⚠️ $log_prefix OK_FALLBACK"
                    echo "$log_prefix SUCCESS_FALLBACK" >> "$LOG"
                else
                    echo "❌ $log_prefix NO_EXPORT_FALLBACK"
                    echo "$log_prefix NO_EXPORT_FILE_FALLBACK" >> "$LOG"
                fi
            else
                echo "❌ $log_prefix EXPORT_FAIL_FALLBACK"
                echo "$log_prefix EXPORT_FAIL_FALLBACK" >> "$LOG"
            fi
        else
            echo "❌ $log_prefix COMPLETE_FAIL"
            echo "$log_prefix PARSE_FAIL_BOTH" >> "$LOG"
        fi
    fi
    
    # Cleanup
    rm -rf "$tmpdir" 2>/dev/null || true
}

# --- Count Total Vulnerable Files ---
total=0
for cwe_dir in "$CODE_FILES_DIR"/CWE-*; do
    if [[ -d "$cwe_dir" ]]; then
        vuln_count=$(find "$cwe_dir" -name "*_vuln.c" -type f | wc -l)
        total=$((total + vuln_count))
    fi
done

echo "🔍 Found $total vulnerable files to process"
echo ""

# --- Main Extraction Loop (VULNERABLE FILES ONLY) ---
count=0
success=0
start_time=$(date +%s)

for cwe_dir in "$CODE_FILES_DIR"/CWE-*; do
    if [[ -d "$cwe_dir" ]]; then
        cwe_name=$(basename "$cwe_dir")
        vuln_files=$(find "$cwe_dir" -name "*_vuln.c" -type f)
        
        if [[ -n "$vuln_files" ]]; then
            echo "🦠 Processing $cwe_name..."
            
            while IFS= read -r vuln_file; do
                if [[ -f "$vuln_file" ]]; then
                    ((count++))
                    
                    # Progress reporting
                    if (( count % 20 == 0 )); then
                        current_time=$(date +%s)
                        elapsed=$((current_time - start_time))
                        if [[ $elapsed -gt 0 ]]; then
                            rate=$(echo "scale=1; $count * 60 / $elapsed" | bc -l 2>/dev/null || echo "N/A")
                            echo "   📊 [$count/$total] Rate: ${rate} files/min | Success: $success"
                        fi
                    fi
                    
                    # Extract instance ID from filename
                    instance_id=$(basename "$vuln_file" _vuln.c)
                    
                    # Define output path (flat structure in reparation directory)
                    vuln_json="$CPG_OUTPUT_DIR/${instance_id}_vuln.cpg.json"
                    
                    # Extract CPG if not already exists
                    if [[ ! -f "$vuln_json" ]]; then
                        extract_vuln_cpg "$vuln_file" "$vuln_json" "$instance_id"
                        # Check if extraction was successful
                        if [[ -f "$vuln_json" ]]; then
                            ((success++))
                        fi
                    else
                        echo "⏭️ $instance_id SKIP (exists)"
                        ((success++))
                    fi
                fi
            done <<< "$vuln_files"
        fi
    fi
done

# --- Final Results ---
end_time=$(date +%s)
total_time=$((end_time - start_time))
json_count=$(find "$CPG_OUTPUT_DIR" -name "*_vuln.cpg.json" 2>/dev/null | wc -l)

echo ""
echo "======================================"
echo "🎯 VULNERABLE CPG EXTRACTION RESULTS"
echo "======================================"
echo "Finished: $(date)"
echo "Total time: ${total_time}s ($((total_time / 60))m)"
echo "Vulnerable files processed: $count"
echo "CPG files created: $json_count"
echo "Success rate: $(echo "scale=1; $json_count * 100 / $count" | bc -l 2>/dev/null || echo "N/A")%"

# Error statistics
if [[ -f "$LOG" && -s "$LOG" ]]; then
    error_count=$(wc -l < "$LOG")
    success_count=$(grep -c SUCCESS "$LOG" 2>/dev/null || echo 0)
    
    echo ""
    echo "📊 Detailed Statistics:"
    echo "  Successful extractions: $success_count"
    echo "  Parse failures: $(grep -c PARSE_FAIL "$LOG" 2>/dev/null || echo 0)"
    echo "  Export failures: $(grep -c EXPORT_FAIL "$LOG" 2>/dev/null || echo 0)"
    echo "  No export file: $(grep -c NO_EXPORT_FILE "$LOG" 2>/dev/null || echo 0)"
    echo "  Fallback successes: $(grep -c FALLBACK "$LOG" 2>/dev/null || echo 0)"
fi

echo ""
echo "📁 Output: $CPG_OUTPUT_DIR"
echo "📋 Log: $LOG"
echo ""

exit 0
