#!/usr/bin/env python3
"""
Classification Evaluation on dataset_detection.csv (1,172 samples)
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

def load_detection_dataset():
    """Load the detection dataset"""
    df = pd.read_csv("../../../data/datasets/dataset_detection.csv")
    print(f"📊 Loaded {len(df)} samples from dataset_detection.csv")
    print(f"📊 Label distribution: {df['label'].value_counts().to_dict()}")
    return df

def analyze_sample_with_vulnrag(code, label, retriever, llm_engine):
    """Analyze a single sample with Vulnrag"""
    
    # Analyze with Vulnrag
    retrieval_results = retriever.hybrid_retrieval(code, top_k=10)
    analysis = llm_engine.analyze_with_context(code, retrieval_results)
    
    return {
        'code': code,
        'true_label': label,
        'retrieval': retrieval_results,
        'analysis': analysis
    }

def calculate_classification_metrics(sample_results):
    """Calculate classification metrics for a single sample"""
    
    true_label = sample_results['true_label']
    analysis = sample_results['analysis']
    retrieval = sample_results['retrieval']
    
    predicted_vuln = analysis.is_vulnerable
    confidence = analysis.confidence_score
    retrieval_classification = retrieval.classification
    
    # Binary classification metrics
    correct = (true_label == 1 and predicted_vuln) or (true_label == 0 and not predicted_vuln)
    
    # TP, FP, TN, FN
    tp = (true_label == 1 and predicted_vuln)
    fp = (true_label == 0 and predicted_vuln)
    tn = (true_label == 0 and not predicted_vuln)
    fn = (true_label == 1 and not predicted_vuln)
    
    # Detailed results for manual analysis
    return {
        # Basic metrics
        'correct': correct,
        'true_label': true_label,
        'predicted_vuln': predicted_vuln,
        'confidence': confidence,
        'retrieval_classification': retrieval_classification,
        'tp': tp,
        'fp': fp,
        'tn': tn,
        'fn': fn,
        
        # Detailed LLM Analysis
        'vulnerability_type': getattr(analysis, 'vulnerability_type', None),
        'cwe_prediction': getattr(analysis, 'cwe_prediction', None),
        'explanation': getattr(analysis, 'explanation', ''),
        'risk_level': getattr(analysis, 'risk_level', None),
        'recommended_fix': getattr(analysis, 'recommended_fix', None),
        
        # Evidence from LLM
        'evidence_for': str(getattr(analysis, 'evidence_for', [])),
        'evidence_against': str(getattr(analysis, 'evidence_against', [])),
        'similar_patterns': str(getattr(analysis, 'similar_patterns', [])),
        'limitations': str(getattr(analysis, 'limitations', [])),
        
        # Retrieval scores
        'best_vuln_score': retrieval.best_vuln_score,
        'best_patch_score': retrieval.best_patch_score,
        'search_time_seconds': retrieval.search_time_seconds,
        
        # Retrieval metadata
        'total_vuln_patterns': retrieval.retrieval_metadata.get('total_vuln_patterns', 0),
        'total_patch_patterns': retrieval.retrieval_metadata.get('total_patch_patterns', 0),
        
        # Code snippet for review (truncated)
        'code_snippet': sample_results['code'][:500] + '...' if len(sample_results['code']) > 500 else sample_results['code']
    }

def run_classification_evaluation(sample_size=None):
    """Run classification evaluation on detection dataset"""
    
    print("🚀 Starting Classification Evaluation...")
    print("=" * 60)
    
    # Load dataset
    df = load_detection_dataset()
    
    if sample_size:
        df = df.sample(n=sample_size, random_state=42)
        print(f"📊 Using sample of {len(df)} samples")
    
    # Initialize Vulnrag components
    print("🔧 Initializing Vulnrag components...")
    retriever = create_vulnrag_retriever()
    llm_engine = create_neutral_vulnrag_engine()
    
    # Results storage
    results = []
    
    print(f"🔍 Analyzing {len(df)} samples...")
    
    for idx, row in df.iterrows():
        if idx % 10 == 0:
            print(f"📊 Progress: {idx}/{len(df)} samples analyzed")
        
        try:
            # Analyze sample
            sample_results = analyze_sample_with_vulnrag(
                row['func'], 
                row['label'], 
                retriever, 
                llm_engine
            )
            
            # Calculate metrics
            metrics = calculate_classification_metrics(sample_results)
            
            # Store results
            results.append({
                'sample_id': idx,
                **metrics
            })
            
        except Exception as e:
            print(f"❌ Error analyzing sample {idx}: {e}")
            continue
    
    # Calculate overall metrics
    df_results = pd.DataFrame(results)
    
    if len(df_results) == 0:
        print("❌ No results to analyze!")
        return None
    
    print("\n📊 Classification Evaluation Results:")
    print("=" * 60)
    
    # Overall accuracy
    accuracy = df_results['correct'].mean()
    print(f"🎯 Overall Accuracy: {accuracy:.3f} ({accuracy*100:.1f}%)")
    
    # Per-class metrics
    vuln_samples = df_results[df_results['true_label'] == 1]
    safe_samples = df_results[df_results['true_label'] == 0]
    
    if len(vuln_samples) > 0:
        vuln_accuracy = vuln_samples['correct'].mean()
        print(f"📊 Vulnerable samples accuracy: {vuln_accuracy:.3f}")
    
    if len(safe_samples) > 0:
        safe_accuracy = safe_samples['correct'].mean()
        print(f"📊 Safe samples accuracy: {safe_accuracy:.3f}")
    
    # Confusion matrix
    tp = df_results['tp'].sum()
    fp = df_results['fp'].sum()
    tn = df_results['tn'].sum()
    fn = df_results['fn'].sum()
    
    print(f"\n📊 Confusion Matrix:")
    print(f"  True Positives: {tp}")
    print(f"  False Positives: {fp}")
    print(f"  True Negatives: {tn}")
    print(f"  False Negatives: {fn}")
    
    # Precision, Recall, F1
    precision = tp / (tp + fp) if (tp + fp) > 0 else 0
    recall = tp / (tp + fn) if (tp + fn) > 0 else 0
    f1 = 2 * (precision * recall) / (precision + recall) if (precision + recall) > 0 else 0
    
    print(f"\n📊 Detailed Metrics:")
    print(f"  Precision: {precision:.3f}")
    print(f"  Recall: {recall:.3f}")
    print(f"  F1-Score: {f1:.3f}")
    
    # Confidence analysis
    avg_confidence = df_results['confidence'].mean()
    print(f"  Average Confidence: {avg_confidence:.3f}")
    
    # Retrieval classification breakdown
    retrieval_counts = df_results['retrieval_classification'].value_counts()
    print(f"\n📊 Retrieval Classification Breakdown:")
    for classification, count in retrieval_counts.items():
        print(f"  {classification}: {count}")
    
    # Save detailed results
    output_dir = Path("evaluation_results")
    output_dir.mkdir(exist_ok=True)
    
    # Save main results
    df_results.to_csv(output_dir / "classification_results.csv", index=False)
    
    # Save a human-readable summary
    summary_df = df_results[[
        'sample_id', 'true_label', 'predicted_vuln', 'confidence', 
        'correct', 'vulnerability_type', 'cwe_prediction', 'risk_level',
        'best_vuln_score', 'best_patch_score', 'explanation'
    ]].copy()
    
    # Add readable labels
    summary_df['ground_truth'] = summary_df['true_label'].map({1: 'VULNERABLE', 0: 'SAFE'})
    summary_df['prediction'] = summary_df['predicted_vuln'].map({True: 'VULNERABLE', False: 'SAFE'})
    summary_df['result'] = summary_df['correct'].map({True: '✅ CORRECT', False: '❌ WRONG'})
    
    # Reorder columns for readability
    summary_cols = [
        'sample_id', 'ground_truth', 'prediction', 'result', 'confidence',
        'vulnerability_type', 'cwe_prediction', 'risk_level',
        'best_vuln_score', 'best_patch_score', 'explanation'
    ]
    summary_df = summary_df[summary_cols]
    
    summary_df.to_csv(output_dir / "classification_summary.csv", index=False)
    
    # Save errors/misclassifications for detailed review
    errors_df = df_results[df_results['correct'] == False].copy()
    if len(errors_df) > 0:
        errors_df.to_csv(output_dir / "classification_errors.csv", index=False)
        print(f"\n⚠️  Found {len(errors_df)} misclassifications - saved to classification_errors.csv")
    
    print(f"\n💾 Results saved to {output_dir}/:")
    print(f"  📊 classification_results.csv - Full detailed results")
    print(f"  📋 classification_summary.csv - Human-readable summary")
    if len(errors_df) > 0:
        print(f"  ❌ classification_errors.csv - Misclassifications for review")
    
    return df_results

if __name__ == "__main__":
    # Run on a sample first (faster for testing)
    print("🧪 Running classification evaluation on sample...")
    results = run_classification_evaluation(sample_size=20)
    
    print("\n✅ Classification evaluation complete!") 