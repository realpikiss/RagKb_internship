#!/usr/bin/env python3
"""
CPG Sanity Check Utility

Usage:
  python scripts/kb2_preprocessing/check_cpg.py /path/to/your_cpg.json

What it does:
  - Parses GraphSON v3 (Joern) CPG
  - Prints top vertex types and missing expected types (CALL, IDENTIFIER, CONTROL_STRUCTURE)
  - Prints basic stats and validation summary
  - Exits with non-zero status if parsing fails
"""

import sys
import json
import argparse
from pathlib import Path
from typing import Set

# Robust import so the script works both in-package and standalone
try:
    from .graphson_parser import GraphSONParser
except Exception:
    # When executed directly as a script
    sys.path.append(str(Path(__file__).parent))
    from graphson_parser import GraphSONParser  # type: ignore


EXPECTED: Set[str] = {"CALL", "IDENTIFIER", "CONTROL_STRUCTURE"}


def check_cpg(cpg_input: str) -> int:
    """Run sanity checks on a CPG file (GraphSON v3) and print diagnostics.

    Args:
        cpg_input: Path to CPG JSON file (or '-' to read JSON from stdin)

    Returns:
        Exit code: 0 if OK, non-zero on failure
    """
    # Accept file path or stdin JSON
    cpg_obj = cpg_input
    if cpg_input == "-":
        try:
            cpg_obj = json.load(sys.stdin)
        except Exception as e:
            print(f"Failed to read JSON from stdin: {e}")
            return 2

    parser = GraphSONParser(cpg_obj)
    if not parser.parse():
        print("CPG parse failed")
        return 1

    vt = parser.get_vertex_types()
    print("\nTop vertex types:")
    for name, count in vt.most_common(10):
        print(f"  {name}: {count}")

    missing = EXPECTED - set(vt.keys())
    print(f"\nMissing expected types: {sorted(missing) if missing else 'None'}")

    basic = parser.get_basic_stats()
    print("\nBasic stats:")
    for k, v in basic.items():
        print(f"  {k}: {v}")

    validation = parser.validate_cpg_types()
    print("\nValidation summary:")
    print("  Vertex types: {expected_found}/{total_expected} expected found".format(
        expected_found=validation.get('vertex_validation', {}).get('expected_found', 0),
        total_expected=validation.get('vertex_validation', {}).get('total_expected', 0)
    ))
    print("  Edge types:   {expected_found}/{total_expected} expected found".format(
        expected_found=validation.get('edge_validation', {}).get('expected_found', 0),
        total_expected=validation.get('edge_validation', {}).get('total_expected', 0)
    ))

    # Gentle hint when CPG is likely "flat"
    if missing:
        print("\nHint: CPG appears to miss semantic nodes. If using Joern, ensure headers/includes are resolved\n   (e.g., compile_commands.json, proper include paths, or preprocess with gcc -E before parsing).")

    return 0


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description="CPG sanity check (GraphSON v3)")
    p.add_argument("cpg", help="Path to CPG JSON file, or '-' to read JSON from stdin")
    args = p.parse_args(argv)

    return check_cpg(args.cpg)


if __name__ == "__main__":
    raise SystemExit(main())
