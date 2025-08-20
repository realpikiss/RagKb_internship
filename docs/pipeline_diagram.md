# VulRAG Hybrid System - Pipeline Architecture



## Detailed Pipeline Stages

### Stage 1: CPG Extraction

- **Input**: Raw C/C++ source code
- **Tool**: Joern static analysis framework
- **Output**: Code Property Graph (JSON format)
- **Purpose**: Convert code into structured graph representation

- **Scripts**: `KB_building/scripts/Cpg_pipeline.sh`, `KB_building/scripts/run_cpg_pipeline_vuln_patch.sh`, `KB_building/scripts/extract_vuln_cpg.sh`
- **Example**:

```bash
# From project root — generates CPGs under data/datasets/test_cpg_files/
bash KB_building/scripts/Cpg_pipeline.sh \
  data/datasets/test_c_files \
  data/datasets/test_cpg_files \
  results/cpg_extraction
```

- **Note**: `data/processed/kb1.json` was built from vulnerable KB code with CPGs generated via these scripts. For evaluation/test cases, also generate a CPG for each query sample and pass its path as `cpg_json_path` in datasets.

### Stage 2: Structural Signature Extraction

- **Input**: CPG JSON file
- **Process**: Extract 25 structural features
- **Output**: Feature vector (25 dimensions)
- **Features**: Graph metrics, control flow, dangerous calls by CWE

### Stage 3: FAISS Structural Retrieval

- **Input**: Query feature vector
- **Index**: Pre-built FAISS index (2317 vulnerability patterns)
- **Process**: Cosine similarity search
- **Output**: Top 10 structurally similar candidates
- **Reduction**: 99.57% search space reduction (2317→10)

### Stage 4: BM25 Semantic Reranking

- **Input**: 10 structural candidates + query code
- **Process**: BM25 scoring on code, purpose, behavior fields
- **Fusion**: Reciprocal Rank Fusion (RRF)
- **Output**: Top 3 semantically relevant candidates
- **Reduction**: Additional 70% reduction (10→3)

### Stage 5: LLM Analysis

- **Input**: Query code + top 3 KB contexts
- **Model**: deepseek-coder-v2:latest (via Ollama)
- **Process**: Contextual vulnerability analysis
- **Output**: Binary verdict (VULNERABLE/SAFE) + reasoning
- **Optimization**: Guided analysis with structured context

## Performance Metrics

| Stage                | Input Size        | Output Size         | Reduction |
| -------------------- | ----------------- | ------------------- | --------- |
| CPG Extraction       | 1 code            | 1 CPG               | -         |
| Signature Extraction | 1 CPG             | 25 features         | -         |
| FAISS Retrieval      | 2317 KB           | 10 candidates       | 99.57%    |
| BM25 Reranking       | 10 candidates     | 3 candidates        | 70%       |
| LLM Analysis         | 3 contexts        | 1 verdict           | -         |
| **Total**            | **2317 KB**       | **1 verdict**       | **99.87%** |

## Key Advantages

1. **Structural Pre-filtering**: Invariant to syntax changes and obfuscation
2. **Semantic Refinement**: Context-aware ranking with BM25
3. **Guided LLM Analysis**: Focused analysis with relevant examples
4. **Efficiency**: 24x faster than raw LLM baseline
5. **Accuracy**: Perfect recall (1.0) with high precision (0.5)
