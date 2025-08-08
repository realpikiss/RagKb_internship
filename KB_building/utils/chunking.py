"""
Chunking utilities for code embedding preparation.

Provides token counting aligned with the embedding model, logical boundary
finding, and robust chunking with overlap suitable for downstream
embedding APIs.
"""
from __future__ import annotations

import os
import re
from typing import Any, Dict, List
from bisect import bisect_right

import tiktoken

# Defaults suitable for OpenAI text-embedding-3-* models
EMBED_MODEL = os.getenv("EMBED_MODEL", "text-embedding-3-large")
CHUNK_SIZE = int(os.getenv("CHUNK_SIZE", 800))
CHUNK_OVERLAP = int(os.getenv("CHUNK_OVERLAP", 100))
# MAX_CHUNKS_PER_FUNCTION caps pathological long functions
MAX_CHUNKS_PER_FUNCTION = int(os.getenv("MAX_CHUNKS_PER_FUNCTION", 32))
# Boundary search window (in characters) when aligning the end of a chunk
BOUNDARY_BACKTRACK_CHARS = int(os.getenv("BOUNDARY_BACKTRACK_CHARS", 150))


def _get_encoding_for_embeddings():
    """Return a tiktoken encoding aligned with the embedding model.

    Fallback order:
    - encoding_for_model(EMBED_MODEL)
    - cl100k_base
    - o200k_base (broad 4o/o3 family); only if available
    """
    try:
        return tiktoken.encoding_for_model(EMBED_MODEL)
    except Exception:
        try:
            return tiktoken.get_encoding("cl100k_base")
        except Exception:
            # Last resort, only if available in this tiktoken build
            return tiktoken.get_encoding("o200k_base")


def count_tokens(text: str, model: str | None = None) -> int:
    """Count tokens using tiktoken, aligned with embedding model.

    Args:
        text: Input string to tokenize
        model: Optional model name. If None, uses EMBED_MODEL
    """
    enc = None
    if model:
        try:
            enc = tiktoken.encoding_for_model(model)
        except Exception:
            pass
    if enc is None:
        enc = _get_encoding_for_embeddings()
    return len(enc.encode(text or ""))


def find_logical_boundaries(code: str) -> List[int]:
    """Heuristic logical boundaries for chunk alignment.

    We look for braces and common C/C++ control structures and function defs.
    Returns absolute character offsets in the input string.
    """
    if not code:
        return [0]

    boundaries = [0]
    lines = code.splitlines(True)  # keepends to track positions accurately

    patterns = [
        r"^\s*\{",
        r"^\s*\}",
        r"^\s*if\s*\(",
        r"^\s*for\s*\(",
        r"^\s*while\s*\(",
        r"^\s*switch\s*\(",
        r"^\s*case\s+",
        r"^\s*\w[\w\s\*]+\s+\w+\s*\([^)]*\)\s*\{",  # func def (rough)
        r"^\s*/\*",   # start of block comment
        r"^\s*\*/",   # end of block comment
        r"^\s*//",     # line comment
    ]

    cur = 0
    for line in lines:
        matched = False
        for pat in patterns:
            if re.match(pat, line):
                boundaries.append(cur)
                matched = True
                break
        # Inline comment markers: add boundary at exact marker position
        # Handle // inline
        idx = line.find("//")
        if idx != -1:
            boundaries.append(cur + idx)
        # Handle /* ... */ block comment markers inline
        idx2 = line.find("/*")
        if idx2 != -1:
            boundaries.append(cur + idx2)
        idx3 = line.find("*/")
        if idx3 != -1:
            boundaries.append(cur + idx3 + 2)  # after end of comment
        cur += len(line)

    boundaries.append(len(code))
    return sorted(set(boundaries))


def _build_token_char_index(enc: tiktoken.core.Encoding, tokens: List[int]) -> List[int]:
    """Build a mapping from token index to character offset in the decoded string.

    char_pos[i] gives the character offset after decoding the first i tokens.
    char_pos has length len(tokens) + 1 with char_pos[0] == 0 and
    char_pos[-1] == len(decoded_string).
    """
    char_pos: List[int] = [0]
    off = 0
    # decode per token once; avoids repeated decoding of long prefixes
    for tok in tokens:
        s = enc.decode([tok])
        off += len(s)
        char_pos.append(off)
    return char_pos


def chunk_code(
    code: str,
    max_tokens: int = CHUNK_SIZE,
    overlap_tokens: int = CHUNK_OVERLAP,
) -> List[Dict[str, Any]]:
    """Chunk code into overlapping windows aligned to token counts.

    - Uses the embedding tokenizer.
    - Attempts to align the end of a chunk to the nearest logical boundary
      within ~100 chars behind the token cutoff.
    - Ensures overlap in token space by decoding to text.
    """
    if not code:
        return []

    enc = _get_encoding_for_embeddings()
    tokens = enc.encode(code)
    n = len(tokens)

    if n <= max_tokens:
        return [
            {
                "text": code,
                "start_pos": 0,
                "end_pos": len(code),
                "tokens": n,
                "chunk_id": 0,
            }
        ]

    boundaries = find_logical_boundaries(code)
    # Precompute token->char index for fast slicing and position math
    char_pos = _build_token_char_index(enc, tokens)
    chunks: List[Dict[str, Any]] = []

    stride = max(max_tokens - overlap_tokens, 1)
    start_tok = 0
    chunk_id = 0

    while start_tok < n and chunk_id < MAX_CHUNKS_PER_FUNCTION:
        # Initial cutoff in token space
        end_tok = min(start_tok + max_tokens, n)

        # Approximate end char position and align to boundary within window
        approx_end_pos = char_pos[end_tok]
        start_char = char_pos[start_tok]
        back = BOUNDARY_BACKTRACK_CHARS
        candidates = [b for b in boundaries if start_char < b <= approx_end_pos and b >= approx_end_pos - back]
        if candidates:
            target_end_char = max(candidates)
            # Map char -> token index (rightmost token ending at or before target_end_char)
            new_end_tok = bisect_right(char_pos, target_end_char) - 1
            # Ensure at least one token to progress
            if new_end_tok <= start_tok:
                new_end_tok = min(start_tok + 1, n)
        else:
            new_end_tok = end_tok

        # Slice text using precomputed char positions
        text = code[char_pos[start_tok]:char_pos[new_end_tok]]
        chunks.append(
            {
                "text": text,
                "start_pos": char_pos[start_tok],
                "end_pos": char_pos[new_end_tok],
                "tokens": new_end_tok - start_tok,
                "chunk_id": chunk_id,
            }
        )

        chunk_id += 1
        if new_end_tok >= n:
            break
        start_tok = min(start_tok + stride, n)

    return chunks
