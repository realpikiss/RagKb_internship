#!/usr/bin/env bash

# ===========================
# Batch CPG Sanity Check Script (jq-based)
# ===========================
# Fast batch sanity check for all GraphSON CPG files in a directory
# Generates a summary report of CPG quality across the dataset
# Usage: ./batch_cpg_sanity_check.sh <cpg_directory> [output_report.json]
# ===========================

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <cpg_directory> [output_report.json]"
    echo "  cpg_directory: Directory containing GraphSON CPG files"
    echo "  output_report.json: Optional output file for detailed report"
    exit 1
fi

CPG_DIR="$1"
OUTPUT_REPORT="$2"

# Check if directory exists
if [[ ! -d "$CPG_DIR" ]]; then
    echo "Error: Directory '$CPG_DIR' not found"
    exit 1
fi

# Check if jq is available
if ! command -v jq &> /dev/null; then
    echo "Error: jq is required but not installed"
    exit 1
fi

echo "=== Batch CPG Sanity Check ==="
echo "Directory: $CPG_DIR"
echo "Started: $(date)"
echo ""

# Initialize counters
total_files=0
good_cpgs=0
suspicious_cpgs=0
poor_cpgs=0
flat_cpgs=0
invalid_files=0

# Initialize detailed results array for JSON output
declare -a detailed_results=()

# Find all JSON/GraphSON files
while IFS= read -r -d '' cpg_file; do
    ((total_files++))
    filename=$(basename "$cpg_file")
    
    # Quick progress indicator
    if (( total_files % 50 == 0 )); then
        echo "Processed $total_files files..."
    fi
    
    # Extract basic metrics using jq
    vertex_count=$(jq '.["@value"].vertices | length' "$cpg_file" 2>/dev/null)
    edge_count=$(jq '.["@value"].edges | length' "$cpg_file" 2>/dev/null)
    
    if [[ -z "$vertex_count" || "$vertex_count" == "null" ]]; then
        ((invalid_files++))
        quality="INVALID"
        is_flat="N/A"
    else
        # Count semantic nodes
        unknown_count=$(jq -r '.["@value"].vertices[].label' "$cpg_file" 2>/dev/null | grep -c '^UNKNOWN$' || echo "0")
        call_count=$(jq -r '.["@value"].vertices[].label' "$cpg_file" 2>/dev/null | grep -c '^CALL$' || echo "0")
        identifier_count=$(jq -r '.["@value"].vertices[].label' "$cpg_file" 2>/dev/null | grep -c '^IDENTIFIER$' || echo "0")
        method_count=$(jq -r '.["@value"].vertices[].label' "$cpg_file" 2>/dev/null | grep -c '^METHOD$' || echo "0")
        control_count=$(jq -r '.["@value"].vertices[].label' "$cpg_file" 2>/dev/null | grep -c '^CONTROL_STRUCTURE$' || echo "0")
        
        semantic_nodes=$((call_count + identifier_count + method_count + control_count))
        
        # Calculate unknown ratio
        if [[ $vertex_count -gt 0 ]]; then
            unknown_ratio=$(echo "scale=3; $unknown_count / $vertex_count" | bc 2>/dev/null || echo "0")
        else
            unknown_ratio="0"
        fi
        
        # Determine quality
        is_flat="NO"
        if [[ $vertex_count -lt 10 ]]; then
            quality="POOR"
        elif [[ $(echo "$unknown_ratio > 0.8" | bc -l 2>/dev/null || echo "0") -eq 1 ]]; then
            quality="FLAT"
            is_flat="YES"
        elif [[ $semantic_nodes -eq 0 ]]; then
            quality="FLAT"
            is_flat="YES"
        elif [[ $call_count -eq 0 && $identifier_count -eq 0 ]]; then
            quality="SUSPICIOUS"
        elif [[ $(echo "$unknown_ratio > 0.6" | bc -l 2>/dev/null || echo "0") -eq 1 ]]; then
            quality="POOR"
        elif [[ $edge_count -eq 0 ]]; then
            quality="FLAT"
            is_flat="YES"
        else
            quality="GOOD"
        fi
        
        # Update counters
        case "$quality" in
            "GOOD") ((good_cpgs++)) ;;
            "SUSPICIOUS") ((suspicious_cpgs++)) ;;
            "POOR") ((poor_cpgs++)) ;;
            "FLAT") ((flat_cpgs++)) ;;
        esac
    fi
    
    # Store detailed result if output report requested
    if [[ -n "$OUTPUT_REPORT" ]]; then
        detailed_results+=("{
            \"file\": \"$filename\",
            \"vertices\": $vertex_count,
            \"edges\": $edge_count,
            \"unknown_count\": ${unknown_count:-0},
            \"unknown_ratio\": ${unknown_ratio:-0},
            \"semantic_nodes\": ${semantic_nodes:-0},
            \"call_count\": ${call_count:-0},
            \"identifier_count\": ${identifier_count:-0},
            \"method_count\": ${method_count:-0},
            \"control_count\": ${control_count:-0},
            \"quality\": \"$quality\",
            \"is_flat\": \"$is_flat\"
        }")
    fi
    
done < <(find "$CPG_DIR" -name "*.json" -o -name "*.graphson" -print0)

echo ""
echo "=== Summary Report ==="
echo "📊 Files Processed: $total_files"
echo ""
echo "🎯 Quality Distribution:"
echo "   ✅ Good CPGs:        $good_cpgs ($(( total_files > 0 ? good_cpgs * 100 / total_files : 0 ))%)"
echo "   ⚠️  Suspicious CPGs:  $suspicious_cpgs ($(( total_files > 0 ? suspicious_cpgs * 100 / total_files : 0 ))%)"
echo "   📉 Poor CPGs:        $poor_cpgs ($(( total_files > 0 ? poor_cpgs * 100 / total_files : 0 ))%)"
echo "   ❌ Flat CPGs:        $flat_cpgs ($(( total_files > 0 ? flat_cpgs * 100 / total_files : 0 ))%)"
echo "   💥 Invalid files:    $invalid_files ($(( total_files > 0 ? invalid_files * 100 / total_files : 0 ))%)"
echo ""

usable_cpgs=$((good_cpgs + suspicious_cpgs))
echo "📈 KB2 Usability:"
echo "   Usable for KB2: $usable_cpgs / $total_files ($(( total_files > 0 ? usable_cpgs * 100 / total_files : 0 ))%)"
echo "   Need fallback:  $((flat_cpgs + poor_cpgs + invalid_files)) / $total_files ($(( total_files > 0 ? (flat_cpgs + poor_cpgs + invalid_files) * 100 / total_files : 0 ))%)"

echo ""
echo "Completed: $(date)"

# Generate detailed JSON report if requested
if [[ -n "$OUTPUT_REPORT" ]]; then
    echo ""
    echo "📝 Generating detailed report: $OUTPUT_REPORT"
    
    # Join detailed results array
    IFS=','
    detailed_json="[${detailed_results[*]}]"
    unset IFS
    
    # Create complete report
    cat > "$OUTPUT_REPORT" << EOF
{
    "timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
    "directory": "$CPG_DIR",
    "summary": {
        "total_files": $total_files,
        "good_cpgs": $good_cpgs,
        "suspicious_cpgs": $suspicious_cpgs,
        "poor_cpgs": $poor_cpgs,
        "flat_cpgs": $flat_cpgs,
        "invalid_files": $invalid_files,
        "usable_cpgs": $usable_cpgs,
        "usable_percentage": $(( total_files > 0 ? usable_cpgs * 100 / total_files : 0 )),
        "flat_percentage": $(( total_files > 0 ? flat_cpgs * 100 / total_files : 0 ))
    },
    "details": $detailed_json
}
EOF
    
    echo "   Report saved to: $OUTPUT_REPORT"
fi

# Exit code based on overall quality
if [[ $flat_cpgs -gt $((total_files / 2)) ]]; then
    echo ""
    echo "⚠️  WARNING: More than 50% of CPGs are flat!"
    exit 2
elif [[ $usable_cpgs -lt $((total_files / 2)) ]]; then
    echo ""
    echo "⚠️  WARNING: Less than 50% of CPGs are usable for KB2!"
    exit 1
else
    exit 0
fi
