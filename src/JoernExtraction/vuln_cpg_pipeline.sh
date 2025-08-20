#!/usr/bin/env bash

# ===========================
# CPG Extraction Pipeline Orchestrator
# ===========================
# Pipeline for CPG extraction and quality assessment:
# . Extract CPGs with kernel stub preprocessing
# ===========================

set -e  # Exit on any error

# Resolve project root: prefer git top-level, fallback to two levels up from this script
if root_dir="$(git rev-parse --show-toplevel 2>/dev/null)"; then
  PROJECT_ROOT="$root_dir"
else
  script_dir_init="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
  PROJECT_ROOT="$(cd "$script_dir_init/../.." && pwd -P)"
fi

# Configure logging: append to timestamped and latest log files
LOG_DIR="$PROJECT_ROOT/logs"
mkdir -p "$LOG_DIR"
TS="$(date +%Y%m%d_%H%M%S)"
LOG_FILE="$LOG_DIR/vuln_cpg_pipeline_${TS}.log"
LATEST_LOG="$LOG_DIR/vuln_cpg_pipeline_latest.log"

# Duplicate stdout/stderr to both console and logs, append mode to avoid truncation
exec > >(tee -a "$LOG_FILE" "$LATEST_LOG") 2>&1

# Default paths (relative to project root)
DEFAULT_CODE_DIR="$PROJECT_ROOT/data/tmp/temp_code_files"
DEFAULT_CPG_DIR="$PROJECT_ROOT/data/tmp/cpgs"
DEFAULT_REPORT_DIR="$PROJECT_ROOT/results/cpg_extraction"

# Parse arguments
CODE_DIR="${1:-$DEFAULT_CODE_DIR}"
CPG_DIR="${2:-$DEFAULT_CPG_DIR}"
REPORT_DIR="${3:-$DEFAULT_REPORT_DIR}"

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"

echo "======================================"
echo "🚀 CPG Extraction Pipeline - VULN ONLY"
echo "======================================"
echo "Project root: $PROJECT_ROOT"
echo "Code directory: $CODE_DIR"
echo "CPG directory: $CPG_DIR"
echo "Report directory: $REPORT_DIR"
echo "Target: VULNERABLE FILES ONLY (*_vuln.c)"
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

if [[ ! -f "$SCRIPT_DIR/../config/kernel_stub.h" ]]; then
    echo "❌ Error: kernel_stub.h not found in KB_building/config directory"
    exit 1
fi

echo "✅ Prerequisites OK"
echo ""

# Create output directories
mkdir -p "$CPG_DIR" "$REPORT_DIR"

# Step : Extract CPGs for VULNERABLE files only
echo "======================================"
echo "🦠 Vulnerable CPG Extraction"
echo "======================================"
echo "Extracting CPGs from VULNERABLE files (*_vuln.c) only..."
echo "Source: $CODE_DIR"
echo "Output: $CPG_DIR"
echo ""

if "$SCRIPT_DIR/extract_vuln_cpg.sh" "$CODE_DIR" "$CPG_DIR"; then
    echo "✅ Vulnerable CPG extraction completed successfully"
else
    echo "❌ Vulnerable CPG extraction failed"
    exit 1
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
        "vulnerable_cpg_extraction"
    ],
    "reports": {
        "pipeline_summary": "$FINAL_REPORT"
    },
    "next_steps": [
        "Review vulnerable CPG quality statistics",
        "Run KB construction with vulnerable code analysis",
        "Analyze vulnerable code patterns for feature extraction"
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
echo ""

exit 0