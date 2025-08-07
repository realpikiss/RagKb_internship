#!/usr/bin/env python3
"""
🔍 MONITOR FULL EVALUATION - Real-time Progress Tracking
Monitor the progress of the full dataset evaluation
"""

import os
import time
import pandas as pd
from pathlib import Path

def monitor_evaluation():
    """Monitor evaluation progress in real-time"""
    
    results_dir = Path("evaluation_results")
    results_file = results_dir / "classification_results.csv"
    
    print("🔍 MONITORING FULL EVALUATION")
    print("=" * 60)
    print(f"📁 Watching: {results_file}")
    print(f"🎯 Target: 1172 samples")
    print("=" * 60)
    
    last_count = 0
    start_time = time.time()
    
    while True:
        try:
            if results_file.exists():
                # Read current results
                df = pd.read_csv(results_file)
                current_count = len(df)
                
                if current_count != last_count:
                    # Calculate progress
                    progress = (current_count / 1172) * 100
                    elapsed = time.time() - start_time
                    
                    if current_count > 0:
                        # Estimate remaining time
                        avg_time_per_sample = elapsed / current_count
                        remaining_samples = 1172 - current_count
                        eta_seconds = remaining_samples * avg_time_per_sample
                        eta_minutes = eta_seconds / 60
                        
                        # Calculate current metrics
                        correct_predictions = (df['result'] == '✅ CORRECT').sum()
                        current_accuracy = correct_predictions / current_count * 100
                        
                        # Count by type
                        vulnerable_count = (df['ground_truth'] == 'VULNERABLE').sum()
                        safe_count = (df['ground_truth'] == 'SAFE').sum()
                        
                        print(f"\n⏱️  {time.strftime('%H:%M:%S')} - Progress Update:")
                        print(f"📊 Samples: {current_count:4d}/1172 ({progress:5.1f}%)")
                        print(f"🎯 Current Accuracy: {current_accuracy:5.1f}%")
                        print(f"🔴 Vulnerable: {vulnerable_count:3d}")
                        print(f"🟢 Safe: {safe_count:3d}")
                        print(f"⏳ ETA: {eta_minutes:5.1f} minutes")
                        print(f"⚡ Speed: {current_count/elapsed*60:.1f} samples/min")
                        print("-" * 50)
                    
                    last_count = current_count
                    
                    # Check if complete
                    if current_count >= 1172:
                        print("\n🎉 EVALUATION COMPLETE!")
                        break
            else:
                print(f"📝 Waiting for results file to be created...")
                
        except Exception as e:
            print(f"⚠️  Error reading results: {e}")
        
        time.sleep(10)  # Check every 10 seconds

if __name__ == "__main__":
    monitor_evaluation()
