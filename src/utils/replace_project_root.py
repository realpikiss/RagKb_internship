#!/usr/bin/env python3
"""
Replace 'Project root' placeholders inside CSV (or any text) files with the
actual absolute repository root path.

Usage:
  python3 -m utils.replace_project_root <file1.csv> [file2.csv ...]

Notes:
- Edits files in place.
- Supports several common placeholder spellings:
    'Project root', 'project_root', '{project_root}', '<project_root>', '${project_root}'
- Prints a per-file summary of replacements.
"""
from __future__ import annotations

import sys
from pathlib import Path

# Common placeholder variants to replace
PLACEHOLDERS = [
    "Project root",
    "project_root",
    "PROJECT_ROOT",
    "{project_root}",
    "<project_root>",
    "${project_root}",
]


def _is_repo_root(p: Path) -> bool:
    try:
        entries = {e.name for e in p.iterdir()}
    except Exception:
        return False
    # Heuristic markers for this repo
    markers = {"src", "data", "results", "docs", "requirements.txt"}
    return bool(markers & entries)


def get_repo_root() -> Path:
    here = Path(__file__).resolve()
    cur = here.parent
    while True:
        if _is_repo_root(cur):
            return cur
        if cur.parent == cur:
            # Fallback: assume repo root is 2 levels above src/utils/
            return here.parents[2]
        cur = cur.parent


def replace_in_file(fp: Path, replacement: str) -> int:
    """Replace placeholders in fp. Returns number of substitutions made."""
    text = fp.read_text(encoding="utf-8")
    count_before = text.count  # bind method for speed
    total = 0
    for ph in PLACEHOLDERS:
        total += count_before(ph)
        text = text.replace(ph, replacement)
    if total:
        fp.write_text(text, encoding="utf-8")
    return total


def main(argv: list[str]) -> int:
    if not argv:
        print("Usage: python3 -m utils.replace_project_root <file1.csv> [file2.csv ...]")
        return 2

    repo_root = str(get_repo_root())
    exit_code = 0

    for arg in argv:
        path = Path(arg)
        if not path.exists():
            print(f"WARN: file not found, skipping: {path}")
            exit_code = 1
            continue
        n = replace_in_file(path, repo_root)
        print(f"{path}: replaced {n} occurrence(s)")

    return exit_code


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
