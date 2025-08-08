#!/usr/bin/env bash

# ===========================
# CPG Extraction Pipeline Orchestrator
# ===========================
# Pipeline for CPG extraction and quality assessment:
# 1. Extract CPGs with kernel stub preprocessing
# 2. Run sanity check and generate quality report
# 3. Generate summary statistics for KB2 preparation
# ===========================

set -e  # Exit on any error

# Default paths
DEFAULT_CODE_DIR="data/tmp/temp_code_files"
DEFAULT_CPG_DIR="data/processed/cpgs"
DEFAULT_REPORT_DIR="data/processed/reports"

# Parse arguments
CODE_DIR="${1:-$DEFAULT_CODE_DIR}"
CPG_DIR="${2:-$DEFAULT_CPG_DIR}"
REPORT_DIR="${3:-$DEFAULT_REPORT_DIR}"

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "======================================"
echo "🚀 CPG Extraction Pipeline"
echo "======================================"
echo "Code directory: $CODE_DIR"
echo "CPG directory: $CPG_DIR"
echo "Report directory: $REPORT_DIR"
echo "Started: $(date)"
echo ""

# Check prerequisites
echo "🔍 Checking prerequisites..."
if [[ ! -d "$CODE_DIR" ]]; then
    echo "❌ Error: Code directory '$CODE_DIR' not found"
    exit 1
fi

if ! command -v joern-parse &> /dev/null; then
    echo "❌ Error: joern-parse not found in PATH"
    exit 1
fi

if ! command -v jq &> /dev/null; then
    echo "❌ Error: jq not found in PATH"
    exit 1
fi

if [[ ! -f "$SCRIPT_DIR/kernel_stub.h" ]]; then
    echo "❌ Error: kernel_stub.h not found in scripts directory"
    exit 1
fi

echo "✅ Prerequisites OK"
echo ""

# Create output directories
mkdir -p "$CPG_DIR" "$REPORT_DIR"

# Step 1: Extract CPGs with preprocessing
echo "======================================"
echo "📊 Step 1: CPG Extraction with Preprocessing"
echo "======================================"
echo "Extracting CPGs from $CODE_DIR to $CPG_DIR..."
echo ""

if "$SCRIPT_DIR/extract_cpg.sh" "$CODE_DIR" "$CPG_DIR"; then
    echo "✅ CPG extraction completed successfully"
else
    echo "❌ CPG extraction failed"
    exit 1
fi
echo ""

# Step 2: CPG Quality Assessment
echo "======================================"
echo "🔍 Step 2: CPG Quality Assessment"
echo "======================================"
echo "Running batch sanity check on extracted CPGs..."
echo ""

SANITY_REPORT="$REPORT_DIR/cpg_sanity_report.json"
if "$SCRIPT_DIR/batch_cpg_sanity_check.sh" "$CPG_DIR" "$SANITY_REPORT"; then
    echo "✅ CPG sanity check completed"
    echo "📝 Report saved to: $SANITY_REPORT"
else
    echo "⚠️  CPG sanity check completed with warnings"
    echo "📝 Report saved to: $SANITY_REPORT"
fi
echo ""

# Show quick summary from sanity check
if [[ -f "$SANITY_REPORT" ]]; then
    echo "📊 Quick Quality Summary:"
    jq -r '
        "   Total CPGs: " + (.summary.total_files | tostring) +
        "\n   Good CPGs: " + (.summary.good_cpgs | tostring) + " (" + (.summary.usable_percentage | tostring) + "%)" +
        "\n   Flat CPGs: " + (.summary.flat_cpgs | tostring) + " (" + (.summary.flat_percentage | tostring) + "%)" +
        "\n   Usable for KB2: " + (.summary.usable_cpgs | tostring) + " files"
    ' "$SANITY_REPORT"
    echo ""
fi

# Step 3: CPG Statistics and Preparation for KB2
echo "======================================"
echo "📈 Step 3: CPG Statistics and KB2 Preparation"
echo "======================================"

echo "🎯 CPG Dataset Summary:"
if [[ -f "$SANITY_REPORT" ]]; then
    # Extract detailed statistics from sanity report
    TOTAL_CPGS=$(jq -r '.summary.total_files' "$SANITY_REPORT" 2>/dev/null || echo "N/A")
    GOOD_CPGS=$(jq -r '.summary.good_cpgs' "$SANITY_REPORT" 2>/dev/null || echo "N/A")
    USABLE_CPGS=$(jq -r '.summary.usable_cpgs' "$SANITY_REPORT" 2>/dev/null || echo "N/A")
    FLAT_CPGS=$(jq -r '.summary.flat_cpgs' "$SANITY_REPORT" 2>/dev/null || echo "N/A")
    USABLE_PCT=$(jq -r '.summary.usable_percentage' "$SANITY_REPORT" 2>/dev/null || echo "N/A")
    
    echo "   📊 Total CPG files: $TOTAL_CPGS"
    echo "   ✅ Good quality CPGs: $GOOD_CPGS"
    echo "   🎯 Usable for KB2: $USABLE_CPGS ($USABLE_PCT%)"
    echo "   ❌ Flat CPGs (need fallback): $FLAT_CPGS"
    echo ""
    
    # Recommendations based on quality
    if [[ "$USABLE_PCT" != "N/A" ]] && [[ $USABLE_PCT -ge 80 ]]; then
        echo "✅ Excellent CPG quality! Ready for KB2 construction."
    elif [[ "$USABLE_PCT" != "N/A" ]] && [[ $USABLE_PCT -ge 60 ]]; then
        echo "⚠️  Good CPG quality. KB2 will use fallback features for some entries."
    else
        echo "🚨 Low CPG quality. Consider reviewing preprocessing or kernel stub."
    fi
else
    echo "⚠️  Sanity report not available for detailed statistics"
fi
echo ""

# Generate final summary report
FINAL_REPORT="$REPORT_DIR/cpg_pipeline_summary.json"
echo "📋 Generating final pipeline summary..."

cat > "$FINAL_REPORT" << EOF
{
    "timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
    "pipeline_version": "1.0",
    "input_directory": "$CODE_DIR",
    "cpg_directory": "$CPG_DIR",
    "steps_completed": [
        "cpg_extraction",
        "quality_assessment",
        "statistics_generation"
    ],
    "reports": {
        "cpg_sanity": "$SANITY_REPORT",
        "pipeline_summary": "$FINAL_REPORT"
    },
    "next_steps": [
        "Review CPG quality statistics",
        "Run KB2 construction with notebooks/kb2.py",
        "Use CPG sanity report for feature extraction decisions"
    ]
}
EOF

echo "✅ Final summary saved to: $FINAL_REPORT"
echo ""

# Success banner
echo "======================================"
echo "🎉 CPG Extraction Pipeline Completed!"
echo "======================================"
echo "📊 Outputs:"
echo "   CPGs: $CPG_DIR"
echo "   Reports: $REPORT_DIR"
echo ""
echo "📋 Next Steps:"
echo "   1. Review CPG quality in: $SANITY_REPORT"
echo "   2. Run KB2 construction: notebooks/kb2.py"
echo "   3. Use sanity report for feature extraction decisions"
echo ""
echo "⏱️  Completed: $(date)"
echo "🚀 Your CPGs are ready for KB2 construction!"
echo ""

exit 0
