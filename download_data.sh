#!/bin/bash
# download_data.sh - Download VulRAG Knowledge Base for VulRAG-Hybrid-System

set -e  # Exit on any error

echo " Downloading VulRAG Knowledge Base..."
echo "Source: https://github.com/KnowledgeRAG4LLMVulD/KnowledgeRAG4LLMVulD"

# Base URL for raw GitHub files
BASE_URL="https://raw.githubusercontent.com/KnowledgeRAG4LLMVulD/KnowledgeRAG4LLMVulD/main"

# Create data directories
echo " Creating data directories..."
mkdir -p data/raw/vulrag_kb

# Download VulRAG knowledge base files
echo "Downloading VulRAG knowledge base (10 CWE categories)..."
CWE_LIST=(119 125 20 200 264 362 401 416 476 787)

for cwe in "${CWE_LIST[@]}"; do
    filename="gpt-4o-mini_CWE-${cwe}_316.json"
    echo "   Downloading CWE-${cwe}..."
    
    if wget -q --show-progress -O "data/raw/vulrag_kb/${filename}" \
        "$BASE_URL/vulnerability%20knowledge/${filename}"; then
        echo "   CWE-${cwe} downloaded successfully"
    else
        echo "   Failed to download CWE-${cwe}"
        exit 1
    fi
done

# Verify downloads
echo " Verifying downloads..."
kb_count=$(find data/raw/vulrag_kb/ -name "*.json" | wc -l)

if [[ $kb_count -eq 10 ]]; then
    echo "VulRAG knowledge base complete: ${kb_count}/10 CWE files"
    
    # Show file sizes
    echo " Downloaded files:"
    for file in data/raw/vulrag_kb/*.json; do
        size=$(ls -lh "$file" | awk '{print $5}')
        basename_file=$(basename "$file")
        echo "   ${basename_file}: ${size}"
    done
    
else
    echo " VulRAG knowledge base incomplete: ${kb_count}/10 files"
    exit 1
fi

echo ""
echo "VulRAG Knowledge Base downloaded successfully!"
echo " Location: data/raw/vulrag_kb/"
echo ""
echo "Next step: jupyter notebook notebooks/01_data_exploration.ipynb"