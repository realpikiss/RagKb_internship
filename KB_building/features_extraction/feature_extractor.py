from __future__ import annotations

import math
import re
from collections import Counter
from typing import Any, Dict, List

# ------------------------
# Optional Tree-sitter
# ------------------------
_PARSER_AVAILABLE = True
try:
    from tree_sitter import Parser  # type: ignore
    try:
        from tree_sitter_languages import get_language  # type: ignore
        _LANG_C = get_language("c")
        _LANG_CPP = get_language("cpp")
    except Exception:
        _LANG_C = None
        _LANG_CPP = None
        _PARSER_AVAILABLE = False
except Exception:
    _PARSER_AVAILABLE = False

# ------------------------
# Utility helpers (pure Python, stable)
# ------------------------

_UNBOUNDED_FUNCS = [r"\bstrcpy\b", r"\bstrcat\b", r"\bsprintf\b", r"\bgets\b"]
_BOUNDED_FUNCS   = [r"\bstrncpy\b", r"\bstrncat\b", r"\bsnprintf\b"]
_MISC_RISK_FUNCS = [r"\bmemcpy\b", r"\bmemmove\b", r"\bscanf\b", r"\bvf?printf\b"]

_PAT_UNBOUNDED = re.compile("|".join(_UNBOUNDED_FUNCS))
_PAT_BOUNDED   = re.compile("|".join(_BOUNDED_FUNCS))
_PAT_MISC      = re.compile("|".join(__MISC_RISK_FUNCS))

_IDENT_RE = re.compile(r"\b([A-Za-z_]\w*)\b")
_CALL_RE  = re.compile(r"\b([A-Za-z_]\w*)\s*\(")

_CTRL_TOKENS = re.compile(r"\b(if|for|while|switch|case)\b")
_RET_TOKENS  = re.compile(r"\breturn\b")

def _entropy_from_counts(counter: Counter) -> float:
    total = sum(counter.values())
    if total <= 0:
        return 0.0
    ent = 0.0
    for _, c in counter.items():
        p = c / total
        if p > 0:
            ent -= p * math.log(p + 1e-12, 2)
    return float(ent)

def _diversity_ratio(unique: int, total: int) -> float:
    if total <= 0:
        return 0.0
    return float(unique) / float(total)

def _nesting_depth_by_braces(code: str) -> int:
    depth = 0
    max_depth = 0
    for ch in code:
        if ch == '{':
            depth += 1
            if depth > max_depth:
                max_depth = depth
        elif ch == '}':
            depth = max(depth - 1, 0)
    return max_depth

def _api_pattern_counts(identifiers: List[str]) -> Dict[str, int]:
    ids = [str(x).lower() for x in identifiers]
    return {
        "error_handling": sum(int(any(k in s for k in ["error", "err", "fail", "exception"])) for s in ids),
        "memory_management": sum(int(any(k in s for k in ["ptr", "alloc", "free", "mem", "buffer"])) for s in ids),
        "security_checks": sum(int(any(k in s for k in ["check", "valid", "auth", "perm", "secure"])) for s in ids),
    }

def _combined_text(danger_calls: List[str], top_calls: List[str], top_identifiers: List[str]) -> str:
    payload: List[str] = []
    payload.extend(danger_calls * 3)  # weight dangerous calls
    payload.extend(top_calls)
    payload.extend(top_identifiers)
    return " ".join(payload)

# ------------------------
# Regex path (no Tree-sitter)
# ------------------------

def _regex_extract(code: str) -> Dict[str, Any]:
    code = code or ""
    lines = code.splitlines()

    calls = _CALL_RE.findall(code)
    call_counter = Counter(calls)
    identifiers = _IDENT_RE.findall(code)

    ctrl_count = len(_CTRL_TOKENS.findall(code))
    ret_count  = len(_RET_TOKENS.findall(code))

    danger_unbounded = len(_PAT_UNBOUNDED.findall(code))
    danger_bounded   = len(_PAT_BOUNDED.findall(code))
    misc_risky       = len(_PAT_MISC.findall(code))

    total_nodes = max(len(lines), 1)  # proxy
    cfg_edges_approx = 2 * ctrl_count
    total_edges = total_nodes + cfg_edges_approx  # simple proxy
    graph_density = (total_edges / total_nodes) if total_nodes else 0.0
    avg_degree = (2 * total_edges) / total_nodes if total_nodes else 0.0

    cyclomatic = 1 + ctrl_count + max(0, len(re.findall(r"\bcase\b", code)) - 0)
    nesting = _nesting_depth_by_braces(code)

    call_entropy = _entropy_from_counts(call_counter)
    calls_per_node = (sum(call_counter.values()) / total_nodes) if total_nodes else 0.0
    control_ratio = (ctrl_count / total_nodes) if total_nodes else 0.0
    data_flow_ratio = cfg_edges_approx / total_nodes if total_nodes else 0.0

    # semantic
    top_calls = [name for name, _ in call_counter.most_common(10)]
    id_counter = Counter(identifiers)
    top_identifiers = [name for name, _ in id_counter.most_common(15)]

    api_patterns = _api_pattern_counts(identifiers)
    danger_names = list(call_counter.keys()) if danger_unbounded > 0 else []
    combined = _combined_text(danger_names, top_calls, top_identifiers)

    total_calls = sum(call_counter.values())
    unique_calls = len(call_counter)
    call_diversity_ratio = _diversity_ratio(unique_calls, total_calls)
    call_distribution_entropy = _entropy_from_counts(call_counter)
    semantic_complexity_score = call_diversity_ratio * call_distribution_entropy

    structural_features = {
        "total_nodes": total_nodes,
        "total_edges": total_edges,
        "graph_density": graph_density,
        "avg_degree": avg_degree,
        "cyclomatic_complexity": cyclomatic,
        "nesting_depth": nesting,
        "essential_complexity": ctrl_count + (cfg_edges_approx // 10),
        "control_structures": ctrl_count,
        "blocks": nesting,  # proxy
        "methods": len(re.findall(r"\b[A-Za-z_]\w*\s+\*?\s*[A-Za-z_]\w*\s*\([^)]*\)\s*\{", code)),
        "call_entropy": call_entropy,
        "calls_per_node": calls_per_node,
        "control_ratio": control_ratio,
        "data_flow_ratio": data_flow_ratio,
    }

    semantic_features = {
        "function_calls": top_calls[:20],  # keep names only as in Hybrid
        "top_calls_vector": [c for _, c in call_counter.most_common(10)],
        "combined_text": combined,
        "call_diversity_ratio": call_diversity_ratio,
        "dangerous_calls": [
            {"name": fn, "category": "unbounded"} for fn in call_counter if fn in {"strcpy", "strcat", "sprintf", "gets"}
        ] + [
            {"name": fn, "category": "bounded"} for fn in call_counter if fn in {"strncpy", "strncat", "snprintf"}
        ] + [
            {"name": fn, "category": "misc"} for fn in call_counter if fn in {"memcpy", "memmove", "scanf", "vprintf", "printf"}
        ],
        "api_patterns": api_patterns,
        "cwe_patterns": {},  # not available here
        "identifiers": id_counter.most_common(15),
        "field_identifiers": [],
    }

    structural_complete = (structural_features["total_nodes"] > 0 and structural_features["methods"] >= 0)
    semantic_complete = (total_calls > 0 or len(identifiers) > 0 or len(combined) > 0)
    quality_score = (0.6 if structural_complete else 0.0) + (0.4 if semantic_complete else 0.0)

    extraction_metadata = {
        "parser_validation": {"treesitter": False},
        "feature_completeness": {
            "structural_complete": bool(structural_complete),
            "semantic_complete": bool(semantic_complete),
            "quality_score": float(quality_score),
        },
    }

    return {
        "structural_features": structural_features,
        "semantic_features": semantic_features,
        "extraction_metadata": extraction_metadata,
    }

# ------------------------
# Tree-sitter path
# ------------------------

def _ts_extract(code: str, language: str = "c") -> Dict[str, Any]:
    if not (_PARSER_AVAILABLE and code):
        return _regex_extract(code or "")

    lang = _LANG_C if language == "c" else _LANG_CPP
    if lang is None:
        return _regex_extract(code)

    parser = Parser()
    parser.set_language(lang)
    tree = parser.parse(code.encode("utf8"))
    root = tree.root_node

    # Traversal
    stack = [root]
    callees: List[str] = []
    identifiers: List[str] = []
    field_ids: List[str] = []
    ctrl_count = 0
    ret_count = 0
    blocks = 0
    methods = 0
    total_nodes = 0

    while stack:
        node = stack.pop()
        total_nodes += 1
        # push children
        try:
            for ch in node.children:
                stack.append(ch)
        except Exception:
            pass

        t = node.type
        if t == "call_expression":
            segment = code[node.start_byte:node.end_byte]
            m = re.match(r"\s*([A-Za-z_]\w*)\s*\(", segment)
            if m:
                callees.append(m.group(1))
        elif t in {"if_statement", "for_statement", "while_statement", "switch_statement", "case_statement"}:
            ctrl_count += 1
        elif t == "return_statement":
            ret_count += 1
        elif t == "compound_statement":  # block
            blocks += 1
        elif t in {"function_definition", "declaration"}:
            # heuristically count functions
            if t == "function_definition":
                methods += 1
        elif t == "identifier":
            identifiers.append(code[node.start_byte:node.end_byte])
        elif t == "field_identifier":
            field_ids.append(code[node.start_byte:node.end_byte])

    # Edges approximation (Tree-sitter does not store edges)
    cfg_edges_approx = 2 * ctrl_count
    total_edges = total_nodes + cfg_edges_approx
    graph_density = (total_edges / total_nodes) if total_nodes else 0.0
    avg_degree = (2 * total_edges) / total_nodes if total_nodes else 0.0

    # Complexity
    cyclomatic = 1 + ctrl_count + max(0, callees.count("case") - 0)
    nesting = _nesting_depth_by_braces(code)

    # Call stats
    call_counter = Counter(callees)
    call_entropy = _entropy_from_counts(call_counter)
    calls_per_node = (sum(call_counter.values()) / total_nodes) if total_nodes else 0.0
    control_ratio = (ctrl_count / total_nodes) if total_nodes else 0.0
    data_flow_ratio = cfg_edges_approx / total_nodes if total_nodes else 0.0

    # Danger
    danger_unbounded = len(_PAT_UNBOUNDED.findall(code))
    danger_bounded   = len(_PAT_BOUNDED.findall(code))
    misc_risky       = len(_PAT_MISC.findall(code))

    # Semantic aggregates
    top_calls = [name for name, _ in call_counter.most_common(10)]
    id_counter = Counter(identifiers)
    top_identifiers = [name for name, _ in id_counter.most_common(15)]

    api_patterns = _api_pattern_counts(identifiers)
    # list dangerous call names seen
    danger_names = [fn for fn in call_counter if fn in {"strcpy", "strcat", "sprintf", "gets", "strncpy", "strncat", "snprintf", "memcpy", "memmove", "scanf"}]
    combined = _combined_text(danger_names, top_calls, top_identifiers)

    total_calls = sum(call_counter.values())
    unique_calls = len(call_counter)
    call_diversity_ratio = _diversity_ratio(unique_calls, total_calls)
    call_distribution_entropy = _entropy_from_counts(call_counter)
    semantic_complexity_score = call_diversity_ratio * call_distribution_entropy

    structural_features = {
        "total_nodes": total_nodes,
        "total_edges": total_edges,
        "graph_density": graph_density,
        "avg_degree": avg_degree,
        "cyclomatic_complexity": cyclomatic,
        "nesting_depth": nesting,
        "essential_complexity": ctrl_count + (cfg_edges_approx // 10),
        "control_structures": ctrl_count,
        "blocks": blocks,
        "methods": methods,
        "call_entropy": call_entropy,
        "calls_per_node": calls_per_node,
        "control_ratio": control_ratio,
        "data_flow_ratio": data_flow_ratio,
    }

    semantic_features = {
        "function_calls": top_calls[:20],
        "top_calls_vector": [c for _, c in call_counter.most_common(10)],
        "combined_text": combined,
        "call_diversity_ratio": call_diversity_ratio,
        "dangerous_calls": [
            {"name": fn, "category": "unbounded"} for fn in call_counter if fn in {"strcpy", "strcat", "sprintf", "gets"}
        ] + [
            {"name": fn, "category": "bounded"} for fn in call_counter if fn in {"strncpy", "strncat", "snprintf"}
        ] + [
            {"name": fn, "category": "misc"} for fn in call_counter if fn in {"memcpy", "memmove", "scanf"}
        ],
        "api_patterns": api_patterns,
        "cwe_patterns": {},  # not available here
        "identifiers": id_counter.most_common(15),
        "field_identifiers": Counter(field_ids).most_common(10),
    }

    structural_complete = (structural_features["total_nodes"] > 0 and methods >= 0)
    semantic_complete = (total_calls > 0 or len(identifiers) > 0 or len(combined) > 0)
    quality_score = (0.6 if structural_complete else 0.0) + (0.4 if semantic_complete else 0.0)

    extraction_metadata = {
        "parser_validation": {"treesitter": True},
        "feature_completeness": {
            "structural_complete": bool(structural_complete),
            "semantic_complete": bool(semantic_complete),
            "quality_score": float(quality_score),
        },
    }

    return {
        "structural_features": structural_features,
        "semantic_features": semantic_features,
        "extraction_metadata": extraction_metadata,
    }

# ------------------------
# Public API
# ------------------------

def extract_fallback_features(code: str, language: str = "c") -> Dict[str, Any]:
    """
    Exhaustive fallback feature extraction compatible with HybridFeatureExtractor.
    Prefers Tree-sitter; falls back to regex heuristics.
    """
    try:
        return _ts_extract(code, language) if (_PARSER_AVAILABLE and code) else _regex_extract(code or "")
    except Exception:
        # Hard fallback to regex if anything goes wrong
        return _regex_extract(code or "")

