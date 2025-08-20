# VulRAG LLM Prompts Documentation

## Overview

The VulRAG system uses several specialized prompts for different stages of LLM analysis. Each prompt is designed for specific tasks in the vulnerability detection pipeline.

## 1. Enrichment Prompts

### 1.1 Purpose Extraction Prompt
**Function**: `_call_llm_enrichment()` - Purpose analysis
**Usage**: Extract functional purpose of code for semantic indexing

```
Analyze this C/C++ code and describe its main purpose in 1-2 sentences.
Focus on what the function is designed to do, not on vulnerabilities.

Code:
{code}

Response: Provide only the purpose description, no additional text.
```

**Parameters**:
- `{code}`: Input C/C++ source code
- Temperature: 0.1 (low creativity)
- Top-p: 0.9, Top-k: 40

### 1.2 Behavior Extraction Prompt
**Function**: `_call_llm_enrichment()` - Behavior analysis
**Usage**: Extract step-by-step execution behavior for semantic indexing

```
Analyze this C/C++ code and describe its step-by-step behavior.
List the major execution steps in order. Focus on functionality, not vulnerabilities.

Code:
{code}

Response: Provide a clear step-by-step description of the function's behavior.
```

**Parameters**:
- `{code}`: Input C/C++ source code
- Temperature: 0.1 (low creativity)
- Top-p: 0.9, Top-k: 40

## 2. Analysis Prompts

### 2.1 Initial Analysis Prompt
**Function**: `create_initial_analysis_prompt()`
**Usage**: Baseline LLM analysis without knowledge base context

```
You are a security expert analyzing C/C++ code for vulnerabilities.

QUERY CODE TO ANALYZE:
{query_code}

TASK: Analyze this code for potential security vulnerabilities.

RESPONSE FORMAT:
1. VERDICT: [VULNERABLE or SAFE]
2. CONFIDENCE: [0.0 to 1.0]
3. REASONING: Detailed explanation of your analysis

Focus on common vulnerability patterns like buffer overflows, use-after-free, 
null pointer dereferences, race conditions, and input validation issues.

Then provide detailed reasoning explaining your analysis.
```

**Parameters**:
- `{query_code}`: Code to analyze for vulnerabilities

### 2.2 Cause Detection Prompt
**Function**: `create_cause_detection_prompt()`
**Usage**: Vulnerability detection with knowledge base context

```
You are a security expert analyzing C/C++ code for vulnerabilities. 

QUERY CODE TO ANALYZE:
{query_code}

SIMILAR VULNERABILITY EXAMPLES FROM KNOWLEDGE BASE:
{context}

TASK: Determine if the query code contains vulnerabilities similar to the examples.

RESPONSE FORMAT:
1. VERDICT: [VULNERABLE or SAFE]
2. CONFIDENCE: [0.0 to 1.0]
3. REASONING: Detailed explanation of your analysis

Compare the query code against the provided vulnerability examples. 
Look for similar patterns, dangerous function calls, and control flow issues.

Then provide detailed reasoning explaining your analysis.
```

**Parameters**:
- `{query_code}`: Code to analyze
- `{context}`: Formatted knowledge base examples (top 3 candidates)

### 2.3 Solution Detection Prompt
**Function**: `create_solution_detection_prompt()`
**Usage**: Mitigation analysis with knowledge base context

```
You are a security expert analyzing C/C++ code for vulnerability mitigations.

QUERY CODE TO ANALYZE:
{query_code}

MITIGATION EXAMPLES FROM KNOWLEDGE BASE:
{context}

TASK: Determine if the query code implements proper security mitigations.

RESPONSE FORMAT:
1. VERDICT: [VULNERABLE or SAFE]
2. CONFIDENCE: [0.0 to 1.0]
3. REASONING: Detailed explanation of your analysis

Compare the query code against the provided mitigation examples.
Look for security improvements, bounds checking, input validation, and proper resource management.

Then provide detailed reasoning explaining what mitigations you found or what's missing.
```

**Parameters**:
- `{query_code}`: Code to analyze
- `{context}`: Formatted knowledge base mitigation examples

## 3. Context Formatting

### 3.1 Structured Context Format
**Function**: `VulnerabilityContext.format_structured_context()`
**Usage**: Format KB examples for LLM consumption

```
### [SIMILAR VULNERABILITY ANALYSIS #{rank}]
- Instance ID: {instance_id}
- CWE Category: {cwe}
- Vulnerability Type: {vuln_type}

**Original Vulnerable Code:**
{vulnerable_code}

**Purpose:** {purpose}

**Behavior:** {behavior}

**Patched/Safe Code:**
{patched_code}

**Key Differences:** {differences}

---
```

**Fields**:
- `rank`: Ranking position (1-3)
- `instance_id`: Unique identifier
- `cwe`: Common Weakness Enumeration category
- `vuln_type`: Type of vulnerability
- `vulnerable_code`: Original vulnerable code
- `purpose`: Functional purpose description
- `behavior`: Step-by-step behavior description
- `patched_code`: Fixed/safe version
- `differences`: Key security improvements

## 4. Prompt Engineering Principles

### 4.1 Structure
- **Clear Role Definition**: "You are a security expert..."
- **Explicit Task**: "TASK: Determine if..."
- **Structured Output**: Numbered response format
- **Context Separation**: Clear sections for code and examples

### 4.2 Temperature Settings
- **Enrichment**: 0.1 (factual, consistent)
- **Analysis**: Default (balanced creativity/accuracy)

### 4.3 Response Constraints
- **Binary Verdicts**: VULNERABLE or SAFE only
- **Confidence Scores**: 0.0 to 1.0 range
- **Structured Reasoning**: Detailed explanations required

## 5. Prompt Flow in VulRAG Pipeline

```
Input Code
    ↓
[Purpose Prompt] → Purpose Description
    ↓
[Behavior Prompt] → Behavior Description
    ↓
Structural Retrieval (FAISS) → Top 40 candidates
    ↓
Semantic Reranking (BM25) → Top 3 candidates
    ↓
[Cause Detection Prompt] → Vulnerability Analysis
    ↓
[Solution Detection Prompt] → Mitigation Analysis
    ↓
Final Verdict + Reasoning
```

## 6. Model Configuration

**Primary Model**: `deepseek-coder-v2:latest`
**Serving**: Ollama local deployment
**Fallback**: Configurable alternative models

**Default Parameters**:
- Temperature: 0.1 (enrichment), default (analysis)
- Top-p: 0.9
- Top-k: 40
- Max tokens: Model-dependent

## 7. Performance Optimization

- **Caching**: Purpose and behavior descriptions cached to avoid redundant calls
- **Context Limiting**: Maximum 3 KB examples per analysis
- **Structured Input**: Consistent formatting for reliable parsing
- **Iterative Analysis**: Multi-step cause/solution detection for thorough analysis
