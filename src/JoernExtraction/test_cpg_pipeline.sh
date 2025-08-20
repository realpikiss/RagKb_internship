#!/usr/bin/env bash
set -euo pipefail

# === Resolve project root: prefer git top-level, fallback to two levels up from this script
if root_dir="$(git rev-parse --show-toplevel 2>/dev/null)"; then
  PROJECT_ROOT="$root_dir"
else
  script_dir_init="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
  PROJECT_ROOT="$(cd "$script_dir_init/../.." && pwd -P)"
fi

CODE_DIR="$PROJECT_ROOT/data/datasets/test_c_files"      # doit contenir CWE-CSV/ créé par le script Python
CPG_DIR="$PROJECT_ROOT/data/datasets/test_cpg_files"
LOG_DIR="$PROJECT_ROOT/logs"

# === Logs uniques & TMPDIR isolé ===
mkdir -p "$LOG_DIR"
ts="$(date +%Y%m%d_%H%M%S)"
LOG_FILE="$LOG_DIR/cpg_pipeline_${ts}_$$.log"
echo "📜 Logs for this run: $LOG_FILE"
exec >"$LOG_FILE" 2>&1

# TMPDIR spécifique à ce run (utile si plusieurs jobs tournent en parallèle)
export TMPDIR="$PROJECT_ROOT/tmp/joern_${ts}_$$"
mkdir -p "$TMPDIR"

# === Préparation des répertoires de travail ===
mkdir -p "$CPG_DIR/vuln" "$CPG_DIR/patch" "$CODE_DIR/CWE-CSV"

echo "PROJECT_ROOT: $PROJECT_ROOT"
echo "CODE_DIR    : $CODE_DIR"
echo "CPG_DIR     : $CPG_DIR"
echo "TMPDIR      : $TMPDIR"
echo "VULN count :  $(find "$CODE_DIR" -type f -name '*_vuln.c'  | wc -l)"
echo "PATCH count:  $(find "$CODE_DIR" -type f -name '*_patch.c' | wc -l)"

# === 1) Pass VULN (direct) ===
echo "==> Extract VULN CPGs"
bash "$PROJECT_ROOT/KB_building/scripts/extract_vuln_cpg.sh" \
  "$CODE_DIR" "$CPG_DIR/vuln"

# === 2) Pass PATCH (copie -> *_vuln.c, extraction, renommage) ===
echo "==> Extract PATCH CPGs (via copie -> *_vuln.c)"
TMP_PATCH_AS_VULN="$(mktemp -d)"
mkdir -p "$TMP_PATCH_AS_VULN/CWE-CSV"

# 2.1 Copier les *_patch.c en *_vuln.c dans le tmp (compat macOS)
find "$CODE_DIR" -type f -name "*_patch.c" | while IFS= read -r f; do
  base="$(basename "$f")"
  as_vuln="${base%_patch.c}_vuln.c"
  cp "$f" "$TMP_PATCH_AS_VULN/CWE-CSV/$as_vuln"
done

PATCH_FAKE_COUNT=$(find "$TMP_PATCH_AS_VULN/CWE-CSV" -type f -name "*_vuln.c" | wc -l | tr -d ' ')
if [ "$PATCH_FAKE_COUNT" -eq 0 ]; then
  echo "ℹ️  Aucun *_patch.c à traiter."
else
  bash "$PROJECT_ROOT/KB_building/scripts/extract_vuln_cpg.sh" \
    "$TMP_PATCH_AS_VULN" "$CPG_DIR/patch"

  # 2.3 Renommer les sorties en *_patch.cpg.json
  find "$CPG_DIR/patch" -type f -name "*_vuln.cpg.json" | while IFS= read -r jf; do
    mv "$jf" "${jf%_vuln.cpg.json}_patch.cpg.json"
  done
fi

# Nettoyage
rm -rf "$TMP_PATCH_AS_VULN"

echo "✅ CPGs générés dans: $CPG_DIR/{vuln,patch}"
echo "📜 Log complet: $LOG_FILE"
