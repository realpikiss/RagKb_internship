# RAG-Hybrid Knowledge Base Builder for VulRAG Extension

This repository provides the **knowledge base construction pipeline** for a hybrid vulnerability detection system extending the VulRAG approach [1].
It covers **data preparation, exploratory analysis, CPG generation, structural signature extraction, and FAISS index construction** for efficient structural pre-filtering.

➡️ The hybrid detector implementation itself is available here: [Hybrid-detector](https://github.com/realpikiss/Hybrid-detector).

[1] Original VulRAG benchmark: [KnowledgeRAG4LLMVulD](https://github.com/KnowledgeRAG4LLMVulD/KnowledgeRAG4LLMVulD)

---

> ⚠️ **Disclaimer:** Scripts assume the **exact repository structure**.
> Keep all datasets and generated files under the documented directories, otherwise commands will fail.

---

## 1) Quick Start

```bash
# Create and activate a virtual environment
python3 -m venv .venv
source .venv/bin/activate

# Install dependencies
pip install -r requirements.txt
```


## 2) Data: Download raw VulRAG corpora

Download the raw VulRAG benchmark datasets and place them under:

* Train → `data/raw/vulrag_kb/`
* Test → `data/datasets/test_samples/`

**Links:**

* [Train set](https://github.com/KnowledgeRAG4LLMVulD/KnowledgeRAG4LLMVulD/tree/717cc1d2d44b83b8cdc21a23508265fb80f2dae7/benchmark/train)
* [Test set](https://github.com/KnowledgeRAG4LLMVulD/KnowledgeRAG4LLMVulD/tree/717cc1d2d44b83b8cdc21a23508265fb80f2dae7/benchmark/test)

---

## 3) Exploratory Analysis

* Jupyter notebooks → `notebooks/`
* EDA results → `results/exploration/`
* Temporary working folders → `data/tmp/`

⚠️ Some exploration fields may already exist. Re-running the notebooks will overwrite them, which is expected behavior.

---

## 4) CPG Generation (Joern)


## Pre-generated CPGs (Optional but Recommended)

For better reproducibility and structural traceability, pre-generated Code Property Graphs (CPGs) are available on Zenodo:

- **Zenodo DOI:** [10.5281/zenodo.16914183](https://doi.org/10.5281/zenodo.16914183)
- **Archive files (compressed):**
  - `vuln_cpgs.tar.gz` (~364 MB) – vulnerable CPGs (train set)
  - `test_cpgs.tar.gz` (~187 MB) – test vulnerable and patched CPGs (test set)

**Download & placement instructions**:

1. Download both archives from Zenodo.
2. Place them in the following directories so that the scripts will locate them correctly:

data/tmp/vuln_cpgs/
data/datasets/test_cpg_files/{vuln,patch}/

3. Extract each archive in place. For example:

```bash
tar -xzvf vuln_cpgs.tar.gz -C data/tmp/vuln_cpgs/
tar -xzvf test_cpgs.tar.gz -C data/datasets/test_cpg_files/
```

Make sure the folder structure exactly matches the layout defined earlier in this README—if not, commands may fail.


* Validate CPGs and export summary CSVs:
  <pre class="overflow-visible!" data-start="2728" data-end="2774"><div class="contain-inline-size rounded-2xl relative bg-token-sidebar-surface-primary"><div class="sticky top-9"><div class="absolute end-0 bottom-0 flex h-9 items-center pe-2"><div class="bg-token-bg-elevated-secondary text-token-text-secondary flex items-center gap-4 rounded-sm px-2 font-sans text-xs"><span class="" data-state="closed"></span></div></div></div><div class="overflow-y-auto p-4" dir="ltr"><code class="whitespace-pre! language-bash"><span><span>python3 src/utils/check_cpg.py
  </span></span></code></div></div></pre>

Results are stored under `results/cpg_extraction/`.

For full reproducibility:

<pre class="overflow-visible!" data-start="2237" data-end="2294"><div class="contain-inline-size rounded-2xl relative bg-token-sidebar-surface-primary"><div class="sticky top-9"><div class="absolute end-0 bottom-0 flex h-9 items-center pe-2"><div class="bg-token-bg-elevated-secondary text-token-text-secondary flex items-center gap-4 rounded-sm px-2 font-sans text-xs"><span class="" data-state="closed"></span></div></div></div><div class="overflow-y-auto p-4" dir="ltr"><code class="whitespace-pre! language-bash"><span><span>bash src/JoernExtraction/vuln_cpg_pipeline.sh
</span></span></code></div></div></pre>



<pre class="overflow-visible!" data-start="2622" data-end="2683"><div class="contain-inline-size rounded-2xl relative bg-token-sidebar-surface-primary"><div class="sticky top-9"><div class="absolute end-0 bottom-0 flex h-9 items-center pe-2"><div class="bg-token-bg-elevated-secondary text-token-text-secondary flex items-center gap-4 rounded-sm px-2 font-sans text-xs"><span class="" data-state="closed"></span></div></div></div><div class="overflow-y-auto p-4" dir="ltr"><code class="whitespace-pre! language-bash"><span><span>bash src/JoernExtraction/test_cpg_pipeline.sh
</span></span></code></div></div></pre>


Ensure **Joern v4.0.398** is installed and available in your `PATH`.

---

## 5) Testing Workflow

* Replace project root placeholders before using test CSVs:
  <pre class="overflow-visible!" data-start="2459" data-end="2596"><div class="contain-inline-size rounded-2xl relative bg-token-sidebar-surface-primary"><div class="sticky top-9"><div class="absolute end-0 bottom-0 flex h-9 items-center pe-2"><div class="bg-token-bg-elevated-secondary text-token-text-secondary flex items-center gap-4 rounded-sm px-2 font-sans text-xs"><span class="" data-state="closed"></span></div></div></div><div class="overflow-y-auto p-4" dir="ltr"><code class="whitespace-pre! language-bash"><span><span>python3 src/utils/replace_project_root.py data/datasets/dataset_pairs.csv \
      data/datasets/evaluation_subset_100.csv
  </span></span></code></div></div></pre>

---

## 6) Structural Signatures and FAISS Index

* Extract structural signatures:
  <pre class="overflow-visible!" data-start="2916" data-end="2973"><div class="contain-inline-size rounded-2xl relative bg-token-sidebar-surface-primary"><div class="sticky top-9"><div class="absolute end-0 bottom-0 flex h-9 items-center pe-2"><div class="bg-token-bg-elevated-secondary text-token-text-secondary flex items-center gap-4 rounded-sm px-2 font-sans text-xs"><span class="" data-state="closed"></span></div></div></div><div class="overflow-y-auto p-4" dir="ltr"><code class="whitespace-pre! language-bash"><span><span>python3 src/scripts/extract_signatures.py
  </span></span></code></div></div></pre>
* Build FAISS index:
  <pre class="overflow-visible!" data-start="2998" data-end="3059"><div class="contain-inline-size rounded-2xl relative bg-token-sidebar-surface-primary"><div class="sticky top-9"><div class="absolute end-0 bottom-0 flex h-9 items-center pe-2"><div class="bg-token-bg-elevated-secondary text-token-text-secondary flex items-center gap-4 rounded-sm px-2 font-sans text-xs"><span class="" data-state="closed"></span></div></div></div><div class="overflow-y-auto p-4" dir="ltr"><code class="whitespace-pre! language-bash"><span><span>python3 src/scripts/build_structural_index.py
  </span></span></code></div></div></pre>

Expected outputs:

* Signatures → `data/signatures/vuln_signatures.csv`
* FAISS index → `data/signatures/faiss_structural.index`

---

## 7) Repository Structure (Full Layout)

The repository must follow this structure:

<pre class="overflow-visible!" data-start="3287" data-end="4026"><div class="contain-inline-size rounded-2xl relative bg-token-sidebar-surface-primary"><div class="sticky top-9"><div class="absolute end-0 bottom-0 flex h-9 items-center pe-2"><div class="bg-token-bg-elevated-secondary text-token-text-secondary flex items-center gap-4 rounded-sm px-2 font-sans text-xs"><span class="" data-state="closed"></span></div></div></div><div class="overflow-y-auto p-4" dir="ltr"><code class="whitespace-pre!"><span><span>── data
│   ├── datasets
│   │   ├── test_c_files
│   │   │   └── CWE-CSV (test files)
│   │   ├── test_cpg_files
│   │   │   ├── patch
│   │   │   └── vuln
│   │   └── test_samples
│   ├── processed
│   ├── raw
│   │   └── vulrag_kb (train files)
│   ├── signatures
│   └── tmp
│       ├── temp_code_files
│       │   ├── CWE-</span><span>119</span><span>
│       │   ├── CWE-</span><span>125</span><span>
│       │   ├── CWE-</span><span>20</span><span>
│       │   ├── CWE-</span><span>200</span><span>
│       │   ├── CWE-</span><span>264</span><span>
│       │   ├── CWE-</span><span>362</span><span>
│       │   ├── CWE-</span><span>401</span><span>
│       │   ├── CWE-</span><span>416</span><span>
│       │   ├── CWE-</span><span>476</span><span>
│       │   └── CWE-</span><span>787</span><span>
│       └── vuln_cpgs
├── docs
├── logs
├── notebooks
├── results
│   ├── cpg_extraction
│   └── exploration
└── </span><span>src</span><span>
    ├── config
    ├── JoernExtraction
    ├── scripts
    └── utils
</span></span></code></div></div></pre>

---

## 8) Requirements and Tools

* **Joern** : 4.0.398 (required)
* **Python** : 3.9+
* **Dependencies** : see `requirements.txt`
* numpy, pandas, networkx, tqdm, faiss-cpu, matplotlib, seaborn, scikit-learn

---

## 9) References

* VulRAG datasets and baseline:

  [KnowledgeRAG4LLMVulD](https://github.com/KnowledgeRAG4LLMVulD/KnowledgeRAG4LLMVulD)
* Benchmarks used: `benchmark/train`, `benchmark/test`

If you use this repository, please cite VulRAG and related works accordingly.

---

## 10) Tips & Troubleshooting

* Always respect repository-relative paths.
* If CPG generation fails → check Joern installation and version.
* For long-running CPG steps, prefer pre-generated CPGs when available.
* Always run `replace_project_root.py` before using test CSVs with placeholders.

---

## 11) Acknowledgements

* **VulRAG** : This work builds upon the benchmark and methodology introduced by [KnowledgeRAG4LLMVulD](https://github.com/KnowledgeRAG4LLMVulD/KnowledgeRAG4LLMVulD).
* **Joern** : Code Property Graph (CPG) generation relies on [Joern](), a powerful open-source code analysis platform.
