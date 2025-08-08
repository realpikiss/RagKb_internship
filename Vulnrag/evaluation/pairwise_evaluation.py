#!/usr/bin/env python3
"""
Pairwise Evaluation on dataset_patch.csv (586 pairs)
"""

import pandas as pd
import numpy as np
import time
from pathlib import Path
import sys
import os

# Add parent directory to path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from vulnrag_retriever import create_vulnrag_retriever
from llm_engine import create_neutral_vulnrag_engine, quick_vulnerability_analysis

def load_patch_dataset():
    """Load the patch dataset"""
    df = pd.read_csv("../../data/datasets/dataset_patch.csv")
    print(f"📊 Loaded {len(df)} pairs from dataset_patch.csv")
    return df

def analyze_pair_with_vulnrag(code_before, code_after, retriever, llm_engine):
    """Analyze a pair with Vulnrag"""
    
    # Analyze before code
    before_results = retriever.hybrid_retrieval(code_before, top_k=10)
    before_analysis = llm_engine.analyze_with_context(code_before, before_results)
    
    # Analyze after code  
    after_results = retriever.hybrid_retrieval(code_after, top_k=10)
    after_analysis = llm_engine.analyze_with_context(code_after, after_results)
    
    return {
        'before': {
            'code': code_before,
            'retrieval': before_results,
            'analysis': before_analysis
        },
        'after': {
            'code': code_after,
            'retrieval': after_results,
            'analysis': after_analysis
        }
    }

def calculate_pairwise_metrics(pair_results):
    """Calculate pairwise accuracy and other metrics"""
    
    before_vuln = pair_results['before']['analysis'].is_vulnerable
    after_vuln = pair_results['after']['analysis'].is_vulnerable
    
    # Correct pairwise classification: before=vuln, after=safe
    pairwise_correct = before_vuln and not after_vuln
    
    # Confidence scores
    before_confidence = pair_results['before']['analysis'].confidence_score
    after_confidence = pair_results['after']['analysis'].confidence_score
    
    # Retrieval quality metrics
    before_retrieval = pair_results['before']['retrieval']
    after_retrieval = pair_results['after']['retrieval']
    
    return {
        'pairwise_correct': pairwise_correct,
        'before_vuln': before_vuln,
        'after_vuln': after_vuln,
        'before_confidence': before_confidence,
        'after_confidence': after_confidence,
        'before_classification': before_retrieval.classification,
        'after_classification': after_retrieval.classification,
        'before_best_vuln_score': before_retrieval.best_vuln_score,
        'after_best_vuln_score': after_retrieval.best_vuln_score,
        'before_best_patch_score': before_retrieval.best_patch_score,
        'after_best_patch_score': after_retrieval.best_patch_score
    }

def evaluate_retrieval_quality(retrieval_results, k=10):
    """Evaluate retrieval quality metrics"""
    
    # Get top-k results and separate by pattern type
    top_results = retrieval_results.top_results[:k]
    vuln_results = [r for r in top_results if r.pattern_type == "VULN"]
    patch_results = [r for r in top_results if r.pattern_type == "PATCH"]
    
    # Calculate precision@k (assuming relevant if hybrid_score > 0.5)
    vuln_precision = sum(1 for r in vuln_results if r.hybrid_score > 0.5) / len(vuln_results) if vuln_results else 0
    patch_precision = sum(1 for r in patch_results if r.hybrid_score > 0.5) / len(patch_results) if patch_results else 0
    
    # Calculate recall@k (simplified - assuming we found relevant patterns)
    vuln_recall = 1.0 if any(r.hybrid_score > 0.5 for r in vuln_results) else 0.0
    patch_recall = 1.0 if any(r.hybrid_score > 0.5 for r in patch_results) else 0.0
    
    # Calculate NDCG@k (simplified)
    vuln_ndcg = sum(r.hybrid_score / np.log2(i+2) for i, r in enumerate(vuln_results)) if vuln_results else 0
    patch_ndcg = sum(r.hybrid_score / np.log2(i+2) for i, r in enumerate(patch_results)) if patch_results else 0
    
    return {
        'vuln_precision@k': vuln_precision,
        'patch_precision@k': patch_precision,
        'vuln_recall@k': vuln_recall,
        'patch_recall@k': patch_recall,
        'vuln_ndcg@k': vuln_ndcg,
        'patch_ndcg@k': patch_ndcg
    }

def run_pairwise_evaluation(sample_size=None):
    """Run pairwise evaluation on patch dataset"""
    
    print("🚀 Starting Pairwise Evaluation...")
    print("=" * 60)
    
    # Load dataset
    df = load_patch_dataset()
    
    if sample_size:
        df = df.sample(n=sample_size, random_state=42)
        print(f"📊 Using sample of {len(df)} pairs")
    
    # Initialize Vulnrag components
    print("🔧 Initializing Vulnrag components...")
    retriever = create_vulnrag_retriever()
    llm_engine = create_neutral_vulnrag_engine()
    
    # Results storage
    results = []
    retrieval_metrics = []
    
    print(f"🔍 Analyzing {len(df)} pairs...")
    
    for idx, row in df.iterrows():
        if idx % 10 == 0:
            print(f"📊 Progress: {idx}/{len(df)} pairs analyzed")
        
        try:
            # Analyze pair
            pair_results = analyze_pair_with_vulnrag(
                row['code_before'], 
                row['code_after'], 
                retriever, 
                llm_engine
            )
            
            # Calculate pairwise metrics
            pairwise_metrics = calculate_pairwise_metrics(pair_results)
            
            # Calculate retrieval quality
            before_retrieval_quality = evaluate_retrieval_quality(pair_results['before']['retrieval'])
            after_retrieval_quality = evaluate_retrieval_quality(pair_results['after']['retrieval'])
            
            # Store results
            results.append({
                'pair_id': idx,
                **pairwise_metrics
            })
            
            retrieval_metrics.append({
                'pair_id': idx,
                'before': before_retrieval_quality,
                'after': after_retrieval_quality
            })
            
        except Exception as e:
            print(f"❌ Error analyzing pair {idx}: {e}")
            continue
    
    # Calculate overall metrics
    df_results = pd.DataFrame(results)
    
    print("\n📊 Pairwise Evaluation Results:")
    print("=" * 60)
    
    # Pairwise accuracy
    pairwise_accuracy = df_results['pairwise_correct'].mean()
    print(f"🎯 Pairwise Accuracy: {pairwise_accuracy:.3f} ({pairwise_accuracy*100:.1f}%)")
    
    # Before/After classification breakdown
    before_vuln_rate = df_results['before_vuln'].mean()
    after_vuln_rate = df_results['after_vuln'].mean()
    print(f"📊 Before code vuln rate: {before_vuln_rate:.3f}")
    print(f"📊 After code vuln rate: {after_vuln_rate:.3f}")
    
    # Confidence analysis
    avg_before_confidence = df_results['before_confidence'].mean()
    avg_after_confidence = df_results['after_confidence'].mean()
    print(f"📊 Avg before confidence: {avg_before_confidence:.3f}")
    print(f"📊 Avg after confidence: {avg_after_confidence:.3f}")
    
    # Retrieval quality metrics
    print("\n📊 Retrieval Quality Metrics:")
    print("-" * 40)
    
    # Aggregate retrieval metrics
    all_before_metrics = []
    all_after_metrics = []
    
    for rm in retrieval_metrics:
        all_before_metrics.append(rm['before'])
        all_after_metrics.append(rm['after'])
    
    before_df = pd.DataFrame(all_before_metrics)
    after_df = pd.DataFrame(all_after_metrics)
    
    print("Before code retrieval:")
    for col in before_df.columns:
        print(f"  {col}: {before_df[col].mean():.3f}")
    
    print("\nAfter code retrieval:")
    for col in after_df.columns:
        print(f"  {col}: {after_df[col].mean():.3f}")
    
    # Save detailed results
    output_dir = Path("evaluation_results")
    output_dir.mkdir(exist_ok=True)
    
    df_results.to_csv(output_dir / "pairwise_results.csv", index=False)
    
    # Save retrieval metrics
    retrieval_df = pd.DataFrame([
        {
            'pair_id': rm['pair_id'],
            **{f"before_{k}": v for k, v in rm['before'].items()},
            **{f"after_{k}": v for k, v in rm['after'].items()}
        }
        for rm in retrieval_metrics
    ])
    retrieval_df.to_csv(output_dir / "retrieval_quality.csv", index=False)
    
    print(f"\n💾 Results saved to {output_dir}/")
    
    return df_results, retrieval_metrics

if __name__ == "__main__":
    # Run on a sample first (faster for testing)
    print("🧪 Running pairwise evaluation on sample...")
    results, retrieval_metrics = run_pairwise_evaluation(sample_size=10)
    
    print("\n✅ Pairwise evaluation complete!") 