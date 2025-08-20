# CPG Features - Complete Reference

## 25 Structural Features Extracted from Code Property Graphs

| #  | Feature Name               | Category      | Description                          | CWE Relevance                    |
| -- | -------------------------- | ------------- | ------------------------------------ | -------------------------------- |
| 1  | `num_nodes`              | Graph Metrics | Total number of nodes in CPG         | General complexity               |
| 2  | `num_edges`              | Graph Metrics | Total number of edges in CPG         | Structural complexity            |
| 3  | `density`                | Graph Metrics | Graph density (edges/possible_edges) | Code interconnectedness          |
| 4  | `avg_degree`             | Graph Metrics | Average node degree                  | Control flow complexity          |
| 5  | `cyclomatic_complexity`  | Control Flow  | McCabe cyclomatic complexity         | General complexity               |
| 6  | `loop_count`             | Control Flow  | Number of loop constructs            | Iteration complexity             |
| 7  | `conditional_count`      | Control Flow  | Number of conditional statements     | Decision complexity              |
| 8  | `buffer_overflow_calls`  | CWE-119/787   | Dangerous string/memory functions    | Buffer overflow vulnerabilities  |
| 9  | `use_after_free_calls`   | CWE-416       | Memory deallocation functions        | Use-after-free vulnerabilities   |
| 10 | `buffer_underread_calls` | CWE-125       | Dangerous read operations            | Buffer underread vulnerabilities |
| 11 | `race_condition_calls`   | CWE-362       | Synchronization primitives           | Race condition vulnerabilities   |
| 12 | `info_disclosure_calls`  | CWE-200       | Information exposure functions       | Information disclosure           |
| 13 | `input_validation_calls` | CWE-20        | Input processing functions           | Input validation issues          |
| 14 | `privilege_calls`        | CWE-264       | Privilege/capability functions       | Privilege escalation             |
| 15 | `resource_leak_calls`    | CWE-401       | Resource allocation functions        | Resource leak vulnerabilities    |
| 16 | `null_deref_calls`       | CWE-476       | Functions returning NULL             | Null pointer dereference         |
| 17 | `malloc_calls`           | Memory Ops    | Memory allocation calls              | Memory management                |
| 18 | `free_calls`             | Memory Ops    | Memory deallocation calls            | Memory management                |
| 19 | `memory_ops`             | Memory Ops    | Total memory operations              | Memory safety                    |
| 20 | `total_dangerous_calls`  | Aggregated    | Sum of all dangerous calls           | Overall risk assessment          |
| 21 | `reaching_def_edges`     | Data Flow     | Reaching definition edges            | Data flow analysis               |
| 22 | `cfg_edges`              | Control Flow  | Control flow graph edges             | Control dependencies             |
| 23 | `cdg_edges`              | Control Flow  | Control dependence edges             | Control dependencies             |
| 24 | `ast_edges`              | Syntax        | Abstract syntax tree edges           | Syntactic structure              |
| 25 | `is_flat_cpg`            | Meta          | Boolean: flat vs hierarchical CPG    | Graph structure type             |

## Feature Categories

### 1. Graph Metrics (Features 1-4)

Basic structural properties of the CPG that capture code complexity and interconnectedness.

### 2. Control Flow Analysis (Features 5-7)

Measures of control flow complexity including loops, conditionals, and cyclomatic complexity.

### 3. CWE-Specific Dangerous Calls (Features 8-16)

Counts of function calls associated with specific Common Weakness Enumeration (CWE) categories:

#### CWE-119/787: Buffer Overflow

- `strcpy`, `strcat`, `sprintf`, `gets`, `memcpy`, `copy_from_user`

#### CWE-416: Use After Free

- `free`, `kfree`, `put_device`, `sock_put`, `mutex_destroy`

#### CWE-125: Buffer Underread

- `memchr`, `strlen`, `strstr`, `array_index_nospec`

#### CWE-362: Race Conditions

- `mutex_lock`, `spin_lock`, `atomic_read`, `smp_mb`

#### CWE-200: Information Disclosure

- `printk`, `dev_info`, `copy_to_user`, `seq_printf`

#### CWE-20: Input Validation

- `sscanf`, `kstrtoul`, `recvfrom`, `read`

#### CWE-264: Privilege Escalation

- `capable`, `setuid`, `current_uid`, `inode_permission`

#### CWE-401: Resource Leaks

- `kmalloc`, `fopen`, `socket`, `init_timer`

#### CWE-476: Null Pointer Dereference

- `kmalloc`, `find_get_page`, `dev_get_by_name`

### 4. Memory Operations (Features 17-19)

Specific tracking of memory allocation and deallocation patterns.

### 5. Edge Type Analysis (Features 21-24)

Different types of edges in the CPG representing various program relationships:

- **Reaching Definition**: Data flow dependencies
- **Control Flow**: Execution order dependencies
- **Control Dependence**: Control-based dependencies
- **AST**: Syntactic structure relationships

### 6. Meta Features (Feature 25)

Structural properties of the CPG itself.

## Feature Extraction Process

1. **CPG Loading**: Parse GraphSON format CPG files using Joern
2. **Node Analysis**: Iterate through all CPG nodes to identify function calls
3. **Edge Analysis**: Count different edge types for flow analysis
4. **Pattern Matching**: Match function names against CWE-categorized dangerous call sets
5. **Metric Calculation**: Compute graph metrics and complexity measures
6. **Vector Assembly**: Combine all 25 features into normalized feature vector

## Usage in system pipeline

These 25 features form the structural signature used for:

- **FAISS Indexing**: Build similarity search index for 2317 vulnerability patterns
- **Retrieval**: Find structurally similar vulnerabilities via cosine similarity
- **Pre-filtering**: Reduce search space from 2317 to 40 candidates (98.3% reduction)

The structural approach ensures robustness to:

- Syntax variations (variable names, formatting)
- Code obfuscation techniques
- Semantic equivalence with different implementations
