# VulRAG-Hybrid: Knowledge-Enhanced Vulnerability Detection & Repair

🚧 **Work in Progress** - Research project in development

## Overview

A novel RAG system combining textual and graph-based knowledge representations for automated vulnerability detection and repair in C/C++ code.

 **Innovation** : Extending VulRAG's text-based knowledge with graph-based code representations (CPG) for hybrid retrieval.

### Key Contributions

* **Dual Knowledge Base** : VulRAG text knowledge (KB1) + Graph-based code representations (KB2)
* **Hybrid Retrieval** : Semantic similarity (text) + Structural similarity (graphs)
* **Neo4j Integration** : Unified storage for text and graph embeddings
* **End-to-End Pipeline** : Detection → Repair → Validation
* **Function-Level Granularity** : Optimized for practical development workflows

### System Architecture

```
Input C/C++ Function → KB1 (VulRAG Text) + KB2 (CPG Graphs) → Hybrid Retrieval → Qwen 2.5 Coder → Patch + Validation
```

## Quick Start

### Environment Setup

```bash
# Create virtual environment
python -m venv vulrag_env
source vulrag_env/bin/activate  # On Windows: vulrag_env\Scripts\activate

# Install dependencies
pip install -r requirements.txt
```

### Prerequisites

* **Python 3.9+**
* **Neo4j** (install via brew): `brew install neo4j`
* **Ollama + Qwen 7B** : `ollama pull qwen2.5-coder:7b`
* **Joern** (for CPG extraction): `brew install joern`
* tmux
* parallells

## Data Setup

### Required Dataset

This project uses the **VulRAG Knowledge Base** - pre-extracted vulnerability knowledge across top-10 CWEs.

 **Source** : [VulRAG Repository](https://github.com/KnowledgeRAG4LLMVulD/KnowledgeRAG4LLMVulD) - `vulnerability knowledge/` directory

### Automated Data Download

```bash
# Download VulRAG knowledge base
chmod +x download_data.sh
./download_data.sh
```

### Manual Download (Alternative)

```bash
# Clone repository temporarily
git clone https://github.com/KnowledgeRAG4LLMVulD/KnowledgeRAG4LLMVulD temp_vulrag

# Copy VulRAG knowledge base
cp -r "temp_vulrag/vulnerability knowledge/" data/raw/vulrag_kb/

# Clean up
rm -rf temp_vulrag
```

### Data Verification

```bash
# Check knowledge base files (should be 10 CWE files)
ls -la data/raw/vulrag_kb/
# Expected output:
# gpt-4o-mini_CWE-119_316.json
# gpt-4o-mini_CWE-125_316.json
# gpt-4o-mini_CWE-20_316.json
# gpt-4o-mini_CWE-200_316.json
# gpt-4o-mini_CWE-264_316.json
# gpt-4o-mini_CWE-362_316.json
# gpt-4o-mini_CWE-401_316.json
# gpt-4o-mini_CWE-416_316.json
# gpt-4o-mini_CWE-476_316.json
# gpt-4o-mini_CWE-787_316.json

# Quick data check
python -c "
import os
kb_files = [f for f in os.listdir('data/raw/vulrag_kb/') if f.endswith('.json')]
print(f'✅ VulRAG knowledge base: {len(kb_files)}/10 CWE files loaded')
"
```

### Knowledge Base Information

* **VulRAG KB** : Pre-extracted knowledge from top-10 CWEs covering:
* **Functional semantics** : Purpose and behavior description
* **Vulnerability behavior** : Preconditions, triggers, specific code patterns
* **Solutions** : Fix descriptions and code changes
* **Coverage** : 10 CWE categories, 1000+ unique CVEs
* **Quality** : GPT-4 extracted knowledge with human verification

## Project Structure

```
VulRAG-Hybrid-System/
├── README.md                       # This file
├── requirements.txt                # Python dependencies
├── download_data.sh                # Data download script
├── setup.sh                        # Environment setup script
│
├── data/
│   ├── raw/
│   │   └── vulrag_kb/              # VulRAG knowledge base (text)
│   ├── processed/                  # CPG extractions + embeddings
│   └── neo4j/                      # Neo4j exports
│
├── notebooks/
│   ├── 01_data_exploration.ipynb        # VulRAG KB analysis
│   ├── 02_cpg_extraction.ipynb          # Build KB2 (graph representations)
│   ├── 03_neo4j_setup.ipynb             # Hybrid storage design
│   ├── 04_rag_system.ipynb              # Hybrid retrieval system
│   └── 05_evaluation.ipynb              # Benchmark evaluation
│
├── src/
│   ├── data/                       # Data loading utilities
│   ├── knowledge_base/             # KB1 (text) + KB2 (graph) management
│   ├── database/                   # Neo4j integration
│   └── rag_system/                 # Hybrid RAG implementation
│
├── configs/                        # Configuration files
├── results/                        # Experimental results
├── tests/                          # Unit tests
└── docs/                           # Additional documentation
```

## Research Approach

### Phase 1: Innovation (Current Focus)

 **Objective** : Enhance VulRAG with graph-based knowledge representations

1. **KB1 (Text)** : Use VulRAG knowledge base directly
2. **KB2 (Graphs)** : Extract CPG from vulnerable/patched functions using Joern
3. **Hybrid Storage** : Neo4j for unified text + graph embeddings
4. **Hybrid Retrieval** : Combine semantic and structural similarity
5. **Evaluation** : PrimeVul + VulnLLMEval benchmarks

### Key Research Questions

* **RQ1** : How do graph-based representations improve vulnerability detection accuracy?
* **RQ2** : What is the optimal fusion strategy for text + graph knowledge?
* **RQ3** : How does hybrid retrieval perform vs. text-only approaches?

## Usage

### 1. Data Exploration

```bash
jupyter notebook notebooks/01_data_exploration.ipynb
```

### 2. Start Neo4j

```bash
brew services start neo4j
# Access: http://localhost:7474 (default: neo4j/neo4j)
```

### 3. Build Knowledge Bases

```bash
# KB2 construction (CPG extraction)
jupyter notebook notebooks/02_cpg_extraction.ipynb

# Neo4j setup
jupyter notebook notebooks/03_neo4j_setup.ipynb
```

### 4. Run RAG System

```bash
# Hybrid RAG system
jupyter notebook notebooks/04_rag_system.ipynb

# Evaluation
jupyter notebook notebooks/05_evaluation.ipynb
```

## Development Status

### Phase 1: Core Innovation

* [X] Project structure
* [X] Data download automation
* [X] VulRAG KB exploration
* [ ] CPG extraction pipeline (KB2)
* [ ] Neo4j hybrid storage
* [ ] Hybrid retrieval system
* [ ] Qwen 2.5 Coder integration
* [ ] Patch generation & validation
* [ ] Benchmark evaluation (PrimeVul, VulnLLMEval)

### Future Work

* [ ] VulRAG benchmark comparison
* [ ] Extended CWE coverage
* [ ] Performance optimization

## Evaluation Benchmarks

* **PrimeVul** : Realistic vulnerability detection (7k vulnerable + 229k benign functions)
* **VulnLLMEval** : LLM vulnerability evaluation framework (307 real-world vulnerabilities)
* **Comparison baseline** : Original VulRAG system

## Data Sources & Attribution

* **VulRAG research** : [Vul-RAG: Enhancing LLM-based Vulnerability Detection via Knowledge-level RAG](https://arxiv.org/abs/2406.11147)
* **Knowledge base** : [KnowledgeRAG4LLMVulD Repository](https://github.com/KnowledgeRAG4LLMVulD/KnowledgeRAG4LLMVulD)
* **PrimeVul benchmark** : [Vulnerability Detection with Code Language Models](https://github.com/DLVulDet/PrimeVul)
* **VulnLLMEval benchmark** : [VulnLLMEval Framework](https://arxiv.org/abs/2409.10756)

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](https://claude.ai/chat/LICENSE) file for details.

## Citation

If you use this work, please cite:

```bibtex
@misc{vulrag-hybrid-2025,
  title={VulRAG-Hybrid: Knowledge-Enhanced Vulnerability Detection \& Repair},
  author={[Your Name]},
  year={2025},
  url={https://github.com/[your-username]/VulRAG-Hybrid-System}
}
```

---

**🚀 Ready to start? Run `./download_data.sh` then open `notebooks/01_data_exploration.ipynb`**
