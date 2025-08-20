"""
CPG Sanity Check Utility (GraphSON v3 - Joern 4.0.398)
- Single file or directory (recursive scan of *.json)
- Prints top vertex/edge types, stats, and missing essential types
- Exit code 0 if OK, non-zero if parse fails

Usage:
    # File
    python utils/check_cpg.py /path/to/export.json

    # Directory
    python utils/check_cpg.py /path/to/OUT_DIR
"""

import sys
import json
import csv
from pathlib import Path
from typing import Set, Tuple, List, Dict, Any
from collections import Counter
from datetime import datetime


try:
    from .graphson_parser import GraphSONParser
except Exception:
    sys.path.append(str(Path(__file__).parent))
    from graphson_parser import GraphSONParser  # type: ignore

    # Essential types (minimal sanity for structural retrieval)
EXPECTED_VERT: Set[str] = {"CALL", "IDENTIFIER", "CONTROL_STRUCTURE"}
EXPECTED_EDGE: Set[str] = {"AST", "CFG", "REACHING_DEF"}


def check_one(path: Path) -> Tuple[bool, str, Dict[str, Any]]:
    """Analyze a CPG file and return results for CSV output."""
    result = {
        "filename": path.name,
        "relative_path": str(path),
        "status": "KO",
        "reason": "",
        "vertex_count": 0,
        "edge_count": 0,
        "vertex_types": "",
        "edge_types": "",
        "missing_vertices": "",
        "missing_edges": "",
        "top_vertex_types": "",
        "top_edge_types": "",
        "file_size_mb": round(path.stat().st_size / (1024 * 1024), 2) if path.exists() else 0
    }
    
    try:
        parser = GraphSONParser(path)
        if not parser.parse():
            result["reason"] = "parse_failed"
            return False, "parse_failed", result

        vt: Counter = parser.get_vertex_types()
        et: Counter = parser.get_edge_types()
        stats = parser.get_basic_stats()

        result["vertex_count"] = int(stats["vertex_count"])
        result["edge_count"] = int(stats["edge_count"])
        result["vertex_types"] = len(vt)
        result["edge_types"] = len(et)

        if stats["vertex_count"] == 0 or stats["edge_count"] == 0:
            result["reason"] = f"empty_graph: nodes={stats['vertex_count']}, edges={stats['edge_count']}"
            return False, result["reason"], result

        missing_v = sorted(EXPECTED_VERT - set(vt.keys()))
        missing_e = sorted(EXPECTED_EDGE - set(et.keys()))
        
        result["missing_vertices"] = "|".join(missing_v) if missing_v else "None"
        result["missing_edges"] = "|".join(missing_e) if missing_e else "None"

        # In non-strict mode we do not fail on missing expected types; we only record them.

        # Top types for analysis
        result["top_vertex_types"] = "|".join([f"{k}:{v}" for k, v in vt.most_common(6)])
        result["top_edge_types"] = "|".join([f"{k}:{v}" for k, v in et.most_common(6)])

        result["status"] = "OK"
        result["reason"] = "success"
        
        msg = (f"OK | nodes={int(stats['vertex_count'])} edges={int(stats['edge_count'])} "
               f"| topV: {result['top_vertex_types'].replace('|', ', ')} | topE: {result['top_edge_types'].replace('|', ', ')} "
               f"| missingV={missing_v or 'None'} missingE={missing_e or 'None'}")
        return True, msg, result

    except Exception as e:
        result["reason"] = f"exception: {e}"
        return False, result["reason"], result


def run_file(path: Path, csv_output: Path = None) -> int:
    ok, msg, result = check_one(path)
    print(msg)
    
    if csv_output:
        # Write a single file result to the CSV
        write_csv_results([result], csv_output)
        print(f"📄 Results saved to: {csv_output}")
    
    return 0 if ok else 1


def write_csv_results(results: List[Dict[str, Any]], csv_path: Path):
    """Write results to a CSV file."""
    csv_path.parent.mkdir(parents=True, exist_ok=True)
    
    fieldnames = [
        "filename", "relative_path", "status", "reason", "vertex_count", "edge_count",
        "vertex_types", "edge_types", "missing_vertices", "missing_edges",
        "top_vertex_types", "top_edge_types", "file_size_mb"
    ]
    
    with open(csv_path, 'w', newline='', encoding='utf-8') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(results)


def run_dir(root: Path, maxshow: int, csv_output: Path = None) -> int:
    files = sorted(root.rglob("*.json"))
    if not files:
        print(f"ERROR: no *.json under {root}")
        return 2

    total, oks = len(files), 0
    failures: List[Tuple[str, str]] = []
    all_results: List[Dict[str, Any]] = []

    print(f"🔍 Scanning {total} JSON files under: {root}\n")
    for i, f in enumerate(files, 1):
        ok, msg, result = check_one(f)
        
        # Compute relative path for cleaner display
        try:
            relative_path = f.relative_to(root)
            result["relative_path"] = str(relative_path)
        except ValueError:
            result["relative_path"] = str(f)
        
        all_results.append(result)
        
        if ok: 
            oks += 1
        else: 
            failures.append((str(f), msg))
        
        # Display progress
        print(f"[{i:4d}/{total}] {result['relative_path']}: {'OK' if ok else 'KO'} — {msg}")

    # Final statistics
    print(f"\n✅ OK: {oks} | ❌ KO: {total - oks} | Total: {total}")
    
    if failures:
        print(f"\n📋 Examples (max {maxshow}):")
        for fp, reason in failures[:maxshow]:
            print(" -", Path(fp).name, "→", reason)

    # Save CSV if requested
    if csv_output:
        write_csv_results(all_results, csv_output)
        print(f"\n📊 Detailed results saved to: {csv_output}")
        
        # CSV summary statistics
        ok_count = sum(1 for r in all_results if r["status"] == "OK")
        ko_count = len(all_results) - ok_count
        print(f"📈 CSV contains {len(all_results)} entries: {ok_count} OK, {ko_count} KO")

    # Exit code: 0 if no KO
    return 0 if oks == total else 1


def main(argv=None) -> int:
    """Production execution: no CLI args. Scan two repo-relative folders and save CSVs.

    - Scans: data/datasets/test_cpg_files/ and data/tmp/vuln_cpgs/
    - Saves CSVs under: results/cpg_extraction/
    - No hardcoded absolute paths
    """
    # Resolve repo root from this file: src/utils/check_cpg.py -> repo_root
    script_path = Path(__file__).resolve()
    repo_root = script_path.parent.parent.parent

    # Input folders
    test_dir = repo_root / "data/datasets/test_cpg_files"
    vuln_dir = repo_root / "data/tmp/vuln_cpgs"

    # Output folder
    results_dir = repo_root / "results/cpg_extraction"
    results_dir.mkdir(parents=True, exist_ok=True)

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")

    # Config
    maxshow = 5

    # Run on test_cpg_files
    status_codes = []
    if test_dir.exists():
        csv_test = results_dir / f"cpg_check_test_cpg_files_{timestamp}.csv"
        print("="*60)
        print(f"🔍 Checking: {test_dir}")
        code = run_dir(test_dir, maxshow, csv_test)
        status_codes.append(code)
    else:
        print(f"WARN: directory not found, skipping: {test_dir}")

    # Run on vuln_cpgs
    if vuln_dir.exists():
        csv_vuln = results_dir / f"cpg_check_vuln_cpgs_{timestamp}.csv"
        print("="*60)
        print(f"🔍 Checking: {vuln_dir}")
        code = run_dir(vuln_dir, maxshow, csv_vuln)
        status_codes.append(code)
    else:
        print(f"WARN: directory not found, skipping: {vuln_dir}")

    # Overall exit code: 0 if all OK, else 1
    return 0 if status_codes and all(c == 0 for c in status_codes) else 1


if __name__ == "__main__":
    raise SystemExit(main())
