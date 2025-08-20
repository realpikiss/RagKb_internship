#!/usr/bin/env python3
"""
Extract comprehensive structural signatures from vulnerable CPGs.

This script processes all CPG files in data/tmp/vuln_cpgs/, extracts detailed
structural features using the unified signature extraction module, and saves
the results to signatures/vuln_signatures.csv for analysis and validation.

Usage:
    python scripts/extract_signatures.py    
"""

import os
import sys
import csv
from pathlib import Path
from typing import Dict, List, Any
import logging
from tqdm import tqdm
import pandas as pd

# Add src root to path for imports
sys.path.append(str(Path(__file__).parent.parent))

# Import unified signature extraction module from utils
from utils.signature_extraction import UnifiedSignatureExtractor, get_feature_columns

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


class SignatureBatchProcessor:
    """Batch processor for extracting signatures from multiple CPGs"""
    
    def __init__(self):
        self.extractor = UnifiedSignatureExtractor()
        self.signatures = []
        self.failed_cpgs = []
        
        logger.info("Initialized SignatureBatchProcessor with unified extractor")
    
    def process_cpg_directory(self, cpg_dir: str) -> List[Dict[str, Any]]:
        """Process all CPG files in directory"""
        cpg_path = Path(cpg_dir)
        if not cpg_path.exists():
            raise ValueError(f"CPG directory not found: {cpg_dir}")
        
        cpg_files = list(cpg_path.glob("*.json"))
        logger.info(f"Found {len(cpg_files)} CPG files to process")
        
        signatures = []
        failed_count = 0
        
        for cpg_file in tqdm(cpg_files, desc="Extracting signatures"):
            signature = self.extractor.extract_signature(str(cpg_file))
            if signature:
                signatures.append(signature)
            else:
                failed_count += 1
                self.failed_cpgs.append(str(cpg_file))
        
        logger.info(f"Successfully extracted {len(signatures)} signatures")
        logger.info(f"Failed to process {failed_count} CPGs")
        
        return signatures
    
    def save_signatures_csv(self, signatures: List[Dict[str, Any]], output_path: str):
        """Save signatures to CSV file"""
        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        if not signatures:
            logger.error("No signatures to save")
            return
        
        # Use unified feature column order
        columns = ['instance_id'] + [col for col in get_feature_columns() if col != 'instance_id']
        
        with open(output_path, 'w', newline='') as csvfile:
            writer = csv.DictWriter(csvfile, fieldnames=columns)
            writer.writeheader()
            for signature in signatures:
                writer.writerow(signature)
        
        logger.info(f"Saved {len(signatures)} signatures to {output_path}")
    
    def validate_signatures(self, signatures: List[Dict[str, Any]]):
        """Validate and show statistics for extracted signatures"""
        if not signatures:
            logger.error("No signatures to validate")
            return
        
        df = pd.DataFrame(signatures)
        
        print("\n" + "="*60)
        print("SIGNATURE VALIDATION REPORT")
        print("="*60)
        
        print(f"\n📊 BASIC STATISTICS:")
        print(f"Total signatures: {len(signatures)}")
        print(f"Flat CPGs detected: {df['is_flat_cpg'].sum()} ({df['is_flat_cpg'].mean()*100:.1f}%)")
        
        print(f"\n📈 GRAPH METRICS:")
        print(f"Nodes - Mean: {df['num_nodes'].mean():.1f}, Median: {df['num_nodes'].median():.1f}, Max: {df['num_nodes'].max()}")
        print(f"Edges - Mean: {df['num_edges'].mean():.1f}, Median: {df['num_edges'].median():.1f}, Max: {df['num_edges'].max()}")
        print(f"Density - Mean: {df['density'].mean():.4f}, Median: {df['density'].median():.4f}")
        
        print(f"\n🔧 CONTROL COMPLEXITY:")
        print(f"Cyclomatic - Mean: {df['cyclomatic_complexity'].mean():.1f}, Median: {df['cyclomatic_complexity'].median():.1f}")
        print(f"Loops - Mean: {df['loop_count'].mean():.1f}, Max: {df['loop_count'].max()}")
        print(f"Conditionals - Mean: {df['conditional_count'].mean():.1f}, Max: {df['conditional_count'].max()}")
        
        print(f"\n🚨 DANGEROUS CALLS BY CWE:")
        cwe_columns = [col for col in df.columns if col.endswith('_calls') and col != 'total_dangerous_calls']
        for col in cwe_columns:
            count = df[col].sum()
            pct_with_calls = (df[col] > 0).mean() * 100
            print(f"{col.replace('_calls', '').replace('_', ' ').title()}: {count} total ({pct_with_calls:.1f}% of CPGs)")
        
        print(f"\n💾 MEMORY OPERATIONS:")
        print(f"Total memory ops: {df['memory_ops'].sum()}")
        print(f"Malloc calls: {df['malloc_calls'].sum()}")
        print(f"Free calls: {df['free_calls'].sum()}")
        
        print(f"\n🔍 OUTLIER DETECTION:")
        # Detect outliers using IQR method
        numeric_cols = ['num_nodes', 'num_edges', 'cyclomatic_complexity', 'total_dangerous_calls']
        for col in numeric_cols:
            Q1 = df[col].quantile(0.25)
            Q3 = df[col].quantile(0.75)
            IQR = Q3 - Q1
            outliers = df[(df[col] < Q1 - 1.5*IQR) | (df[col] > Q3 + 1.5*IQR)]
            if len(outliers) > 0:
                print(f"{col}: {len(outliers)} outliers (>{Q3 + 1.5*IQR:.1f} or <{Q1 - 1.5*IQR:.1f})")
                if len(outliers) <= 5:  # Show top outliers
                    top_outliers = outliers.nlargest(3, col)['instance_id'].tolist()
                    print(f"  Top outliers: {', '.join(top_outliers)}")
        
        print(f"\n✅ VALIDATION COMPLETE")
        print("="*60)


def main():
    """Production execution: no CLI args, fixed paths resolved from repo root."""
    # Resolve repo root: src/ is one level under repo
    script_path = Path(__file__).resolve()
    repo_root = script_path.parent.parent.parent

    # Fixed input/output paths
    cpg_dir = repo_root / "data/tmp/vuln_cpgs"
    output_csv = repo_root / "data/signatures/vuln_signatures.csv"

    logger.info("Starting CPG signature extraction (production mode)")
    logger.info(f"Repo root: {repo_root}")
    logger.info(f"Input CPG dir: {cpg_dir}")
    logger.info(f"Output CSV: {output_csv}")

    if not cpg_dir.exists():
        logger.error(f"CPG directory not found: {cpg_dir}")
        return 2

    # Initialize processor
    processor = SignatureBatchProcessor()

    # Process CPGs
    signatures = processor.process_cpg_directory(str(cpg_dir))

    if not signatures:
        logger.error("No signatures extracted. Exiting.")
        return 1

    # Save to CSV
    processor.save_signatures_csv(signatures, str(output_csv))

    # Report failed CPGs (optional log file next to CSV)
    if processor.failed_cpgs:
        logger.warning(f"Failed to process {len(processor.failed_cpgs)} CPGs")
        failed_log = output_csv.parent / "failed_cpgs.txt"
        with open(failed_log, 'w') as f:
            for failed_cpg in processor.failed_cpgs:
                f.write(f"{failed_cpg}\n")
        logger.info(f"Failed CPG list saved to {failed_log}")

    logger.info("✅ Signature extraction complete")
    return 0


if __name__ == "__main__":
    main()