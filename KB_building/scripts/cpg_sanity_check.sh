#!/usr/bin/env bash

# ===========================
# CPG Sanity Check Script (jq-based)
# ===========================
# Fast sanity check for GraphSON CPG files using jq
# Detects flat CPGs and provides quality metrics
# Usage: ./cpg_sanity_check.sh <graphson_file> [--verbose]
# ===========================

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <graphson_file> [--verbose]"
    echo "  graphson_file: Path to GraphSON JSON file to analyze"
    echo "  --verbose: Show detailed breakdown"
    exit 1
fi

GRAPHSON_FILE="$1"
VERBOSE=false

if [[ "$2" == "--verbose" ]]; then
    VERBOSE=true
fi

# Check if file exists
if [[ ! -f "$GRAPHSON_FILE" ]]; then
    echo "Error: File '$GRAPHSON_FILE' not found"
    exit 1
fi

# Check if jq is available
if ! command -v jq &> /dev/null; then
    echo "Error: jq is required but not installed"
    exit 1
fi

# Extract basic metrics using jq
echo "=== CPG Sanity Check: $(basename "$GRAPHSON_FILE") ==="

# Count vertices and edges
VERTEX_COUNT=$(jq '.["@value"].vertices | length' "$GRAPHSON_FILE" 2>/dev/null)
EDGE_COUNT=$(jq '.["@value"].edges | length' "$GRAPHSON_FILE" 2>/dev/null)

if [[ -z "$VERTEX_COUNT" || "$VERTEX_COUNT" == "null" ]]; then
    echo "❌ INVALID: Cannot parse GraphSON structure"
    exit 1
fi

echo "📊 Basic Metrics:"
echo "   Vertices: $VERTEX_COUNT"
echo "   Edges: $EDGE_COUNT"

if [[ $VERTEX_COUNT -gt 0 ]]; then
    EDGE_VERTEX_RATIO=$(echo "scale=2; $EDGE_COUNT / $VERTEX_COUNT" | bc 2>/dev/null || echo "N/A")
    echo "   Edge/Vertex Ratio: $EDGE_VERTEX_RATIO"
fi

# Count vertex labels
echo ""
echo "🏷️  Vertex Labels (Top 10):"
jq -r '.["@value"].vertices[].label' "$GRAPHSON_FILE" 2>/dev/null | \
    sort | uniq -c | sort -nr | head -10 | \
    while read count label; do
        printf "   %-20s: %d\n" "$label" "$count"
    done

# Count edge labels  
echo ""
echo "🔗 Edge Labels (Top 10):"
jq -r '.["@value"].edges[].label' "$GRAPHSON_FILE" 2>/dev/null | \
    sort | uniq -c | sort -nr | head -10 | \
    while read count label; do
        printf "   %-20s: %d\n" "$label" "$count"
    done

# Calculate flatness metrics
UNKNOWN_COUNT=$(jq -r '.["@value"].vertices[].label' "$GRAPHSON_FILE" 2>/dev/null | grep -c '^UNKNOWN$' || echo "0")
UNKNOWN_RATIO=$(echo "scale=3; $UNKNOWN_COUNT / $VERTEX_COUNT" | bc 2>/dev/null || echo "0")

# Check for semantic nodes
SEMANTIC_NODES=$(jq -r '.["@value"].vertices[].label' "$GRAPHSON_FILE" 2>/dev/null | \
    grep -E '^(CALL|IDENTIFIER|METHOD|CONTROL_STRUCTURE)$' | wc -l || echo "0")

CALL_COUNT=$(jq -r '.["@value"].vertices[].label' "$GRAPHSON_FILE" 2>/dev/null | grep -c '^CALL$' || echo "0")
IDENTIFIER_COUNT=$(jq -r '.["@value"].vertices[].label' "$GRAPHSON_FILE" 2>/dev/null | grep -c '^IDENTIFIER$' || echo "0")
METHOD_COUNT=$(jq -r '.["@value"].vertices[].label' "$GRAPHSON_FILE" 2>/dev/null | grep -c '^METHOD$' || echo "0")
CONTROL_COUNT=$(jq -r '.["@value"].vertices[].label' "$GRAPHSON_FILE" 2>/dev/null | grep -c '^CONTROL_STRUCTURE$' || echo "0")

echo ""
echo "🔍 Flatness Analysis:"
echo "   UNKNOWN nodes: $UNKNOWN_COUNT"
echo "   UNKNOWN ratio: $UNKNOWN_RATIO"
echo "   Semantic nodes: $SEMANTIC_NODES"
echo "     - CALL: $CALL_COUNT"
echo "     - IDENTIFIER: $IDENTIFIER_COUNT" 
echo "     - METHOD: $METHOD_COUNT"
echo "     - CONTROL_STRUCTURE: $CONTROL_COUNT"

# Determine CPG quality
echo ""
echo "🎯 Quality Assessment:"

# Flat CPG detection logic
IS_FLAT=false
QUALITY="GOOD"

if [[ $VERTEX_COUNT -lt 10 ]]; then
    echo "   ⚠️  Very small CPG (< 10 vertices)"
    QUALITY="POOR"
elif [[ $(echo "$UNKNOWN_RATIO > 0.8" | bc -l 2>/dev/null || echo "0") -eq 1 ]]; then
    echo "   ❌ FLAT CPG detected (UNKNOWN ratio > 80%)"
    IS_FLAT=true
    QUALITY="FLAT"
elif [[ $SEMANTIC_NODES -eq 0 ]]; then
    echo "   ❌ FLAT CPG detected (no semantic nodes)"
    IS_FLAT=true
    QUALITY="FLAT"
elif [[ $CALL_COUNT -eq 0 && $IDENTIFIER_COUNT -eq 0 ]]; then
    echo "   ⚠️  Suspicious CPG (no CALL or IDENTIFIER nodes)"
    QUALITY="SUSPICIOUS"
elif [[ $(echo "$UNKNOWN_RATIO > 0.6" | bc -l 2>/dev/null || echo "0") -eq 1 ]]; then
    echo "   ⚠️  High UNKNOWN ratio (> 60%)"
    QUALITY="POOR"
else
    echo "   ✅ Good CPG quality"
    QUALITY="GOOD"
fi

# Edge quality check
if [[ $EDGE_COUNT -eq 0 ]]; then
    echo "   ❌ No edges found"
    QUALITY="FLAT"
elif [[ $(echo "$EDGE_VERTEX_RATIO < 1.0" | bc -l 2>/dev/null || echo "0") -eq 1 ]]; then
    echo "   ⚠️  Low edge density (< 1.0 edges per vertex)"
    if [[ "$QUALITY" == "GOOD" ]]; then
        QUALITY="POOR"
    fi
fi

# Summary
echo ""
echo "📋 Summary:"
echo "   File: $(basename "$GRAPHSON_FILE")"
echo "   Quality: $QUALITY"
echo "   Flat CPG: $(if $IS_FLAT; then echo "YES"; else echo "NO"; fi)"
echo "   Usable for KB2: $(if [[ "$QUALITY" == "GOOD" || "$QUALITY" == "SUSPICIOUS" ]]; then echo "YES"; else echo "NO"; fi)"

# Verbose output
if $VERBOSE; then
    echo ""
    echo "🔬 Detailed Analysis:"
    
    # Show some sample UNKNOWN nodes if present
    if [[ $UNKNOWN_COUNT -gt 0 ]]; then
        echo ""
        echo "   Sample UNKNOWN nodes (first 3):"
        jq -r '.["@value"].vertices[] | select(.label == "UNKNOWN") | .properties.CODE[0]["@value"] // "N/A"' "$GRAPHSON_FILE" 2>/dev/null | \
            head -3 | sed 's/^/     /'
    fi
    
    # Show all vertex label counts
    echo ""
    echo "   All vertex labels:"
    jq -r '.["@value"].vertices[].label' "$GRAPHSON_FILE" 2>/dev/null | \
        sort | uniq -c | sort -nr | \
        while read count label; do
            printf "     %-25s: %d\n" "$label" "$count"
        done
        
    # Show all edge label counts
    echo ""
    echo "   All edge labels:"
    jq -r '.["@value"].edges[].label' "$GRAPHSON_FILE" 2>/dev/null | \
        sort | uniq -c | sort -nr | \
        while read count label; do
            printf "     %-25s: %d\n" "$label" "$count"
        done
fi

# Exit code based on quality
case "$QUALITY" in
    "GOOD")
        exit 0
        ;;
    "SUSPICIOUS")
        exit 1
        ;;
    "POOR")
        exit 2
        ;;
    "FLAT")
        exit 3
        ;;
    *)
        exit 4
        ;;
esac
