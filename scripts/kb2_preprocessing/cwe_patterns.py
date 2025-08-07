"""
CWE-Specific Pattern Recognition for Linux Kernel C/C++ Code
Based on Top-10 CWEs from linuxkernelcves.com dataset
"""

from typing import Dict, List, Any
import re
import json
from pathlib import Path

class CWEPatternExtractor:
    """Extract CWE-specific patterns from Linux kernel code"""
    
    def __init__(self, config_file: Path = None):
        self.config_file = config_file or Path(__file__).parent / "config" / "cwe_config.json"
        self.cwe_patterns = {}
        self.cwe_risk_weights = {}
        self.dataset_stats = {}
        self._load_config()
        # CWE-416: Use After Free (660 pairs)
        self.cwe_416_patterns = {
            'free_after_use': [
                r'kfree\s*\(\s*([^)]+)\s*\)',
                r'free\s*\(\s*([^)]+)\s*\)',
                r'devm_kfree\s*\(\s*([^)]+)\s*\)'
            ],
            'use_after_free_indicators': [
                r'if\s*\(\s*([^)]+)\s*\)\s*{[^}]*kfree\s*\(\s*\1\s*\)[^}]*}\s*([^;]+)\1',
                r'kfree\s*\(\s*([^)]+)\s*\)[^;]*;\s*([^;]+)\1'
            ]
        }
        
        # CWE-476: NULL Pointer Dereference (281 pairs)
        self.cwe_476_patterns = {
            'null_checks': [
                r'if\s*\(\s*([^)]+)\s*==\s*NULL\s*\)',
                r'if\s*\(\s*!([^)]+)\s*\)',
                r'if\s*\(\s*([^)]+)\s*\)\s*{[^}]*return\s*-?ENOMEM'
            ],
            'dereference_after_null': [
                r'([^)]+)\s*=\s*([^;]+);\s*([^;]+)\1->',
                r'([^)]+)\s*=\s*([^;]+);\s*([^;]+)\1\.'
            ]
        }
        
        # CWE-362: Race Condition (320 pairs)
        self.cwe_362_patterns = {
            'locking_patterns': [
                r'mutex_lock\s*\(\s*([^)]+)\s*\)',
                r'spin_lock\s*\(\s*([^)]+)\s*\)',
                r'rtnl_lock\s*\(\s*\)',
                r'rcu_read_lock\s*\(\s*\)'
            ],
            'unlocking_patterns': [
                r'mutex_unlock\s*\(\s*([^)]+)\s*\)',
                r'spin_unlock\s*\(\s*([^)]+)\s*\)',
                r'rtnl_unlock\s*\(\s*\)',
                r'rcu_read_unlock\s*\(\s*\)'
            ]
        }
        
        # CWE-119/787: Buffer Overflow/Out-of-bounds Write (360 pairs)
        self.cwe_119_787_patterns = {
            'buffer_operations': [
                r'copy_from_user\s*\(\s*([^,]+),\s*([^,]+),\s*([^)]+)\s*\)',
                r'copy_to_user\s*\(\s*([^,]+),\s*([^,]+),\s*([^)]+)\s*\)',
                r'memcpy\s*\(\s*([^,]+),\s*([^,]+),\s*([^)]+)\s*\)',
                r'strcpy\s*\(\s*([^,]+),\s*([^)]+)\s*\)',
                r'strncpy\s*\(\s*([^,]+),\s*([^,]+),\s*([^)]+)\s*\)'
            ],
            'size_checks': [
                r'if\s*\(\s*([^)]+)\s*>\s*([^)]+)\s*\)',
                r'if\s*\(\s*([^)]+)\s*<\s*([^)]+)\s*\)',
                r'sizeof\s*\(\s*([^)]+)\s*\)'
            ]
        }
        
        # CWE-20: Input Validation (182 pairs)
        self.cwe_20_patterns = {
            'validation_checks': [
                r'if\s*\(\s*([^)]+)\s*==\s*NULL\s*\)',
                r'if\s*\(\s*([^)]+)\s*<=\s*0\s*\)',
                r'if\s*\(\s*([^)]+)\s*>\s*([^)]+)\s*\)',
                r'validate_user_input\s*\(\s*([^)]+)\s*\)',
                r'check_user_input\s*\(\s*([^)]+)\s*\)'
            ]
        }
        
        # CWE-200: Information Exposure (152 pairs)
        self.cwe_200_patterns = {
            'info_exposure': [
                r'printk\s*\(\s*([^)]+)\s*\)',
                r'dev_info\s*\(\s*([^)]+)\s*\)',
                r'dev_err\s*\(\s*([^)]+)\s*\)',
                r'pr_info\s*\(\s*([^)]+)\s*\)',
                r'pr_err\s*\(\s*([^)]+)\s*\)'
            ]
        }
        
        # CWE-125: Out-of-bounds Read (140 pairs)
        self.cwe_125_patterns = {
            'read_operations': [
                r'copy_from_user\s*\(\s*([^,]+),\s*([^,]+),\s*([^)]+)\s*\)',
                r'get_user\s*\(\s*([^,]+),\s*([^)]+)\s*\)',
                r'read\s*\(\s*([^,]+),\s*([^,]+),\s*([^)]+)\s*\)'
            ]
        }
        
        # CWE-264: Access Control (120 pairs)
        self.cwe_264_patterns = {
            'access_control': [
                r'capable\s*\(\s*([^)]+)\s*\)',
                r'current_user_ns\s*\(\s*\)',
                r'ns_capable\s*\(\s*([^)]+)\s*\)',
                r'permission\s*\(\s*([^)]+)\s*\)'
            ]
        }
        
        # CWE-401: Memory Leak (101 pairs)
        self.cwe_401_patterns = {
            'memory_allocation': [
                r'kmalloc\s*\(\s*([^)]+)\s*\)',
                r'vmalloc\s*\(\s*([^)]+)\s*\)',
                r'kzalloc\s*\(\s*([^)]+)\s*\)',
                r'devm_kmalloc\s*\(\s*([^)]+)\s*\)'
            ],
            'memory_cleanup': [
                r'kfree\s*\(\s*([^)]+)\s*\)',
                r'vfree\s*\(\s*([^)]+)\s*\)',
                r'devm_kfree\s*\(\s*([^)]+)\s*\)'
            ]
        }
    
    def _load_config(self):
        """Load CWE configuration from JSON file."""
        try:
            if self.config_file.exists():
                with open(self.config_file, 'r') as f:
                    config = json.load(f)
                    self.cwe_patterns = config.get('cwe_patterns', {})
                    self.dataset_stats = config.get('dataset_statistics', {})
                    
                    # Extract risk weights
                    for cwe_type, cwe_config in self.cwe_patterns.items():
                        self.cwe_risk_weights[cwe_type] = cwe_config.get('risk_weight', 0.5)
                    
                    print(f"✅ Loaded CWE configuration from {self.config_file}")
                    print(f"   - {len(self.cwe_patterns)} CWE patterns loaded")
                    print(f"   - {len(self.cwe_risk_weights)} risk weights configured")
            else:
                print(f"⚠️  CWE config file not found: {self.config_file}")
                print("   Using hardcoded patterns as fallback")
        except Exception as e:
            print(f"❌ Error loading CWE config: {e}")
            print("   Using hardcoded patterns as fallback")
    
    def extract_cwe_patterns(self, code: str) -> Dict[str, Any]:
        """Extract CWE-specific patterns from kernel code"""
        
        patterns_found = {}
        
        # CWE-416: Use After Free
        cwe_416_matches = self._find_patterns(code, self.cwe_416_patterns)
        if cwe_416_matches:
            patterns_found['CWE-416'] = {
                'free_operations': len(cwe_416_matches.get('free_after_use', [])),
                'use_after_free_indicators': len(cwe_416_matches.get('use_after_free_indicators', [])),
                'risk_score': self._calculate_cwe_risk_score(cwe_416_matches, 'CWE-416')
            }
        
        # CWE-476: NULL Pointer Dereference
        cwe_476_matches = self._find_patterns(code, self.cwe_476_patterns)
        if cwe_476_matches:
            patterns_found['CWE-476'] = {
                'null_checks': len(cwe_476_matches.get('null_checks', [])),
                'dereference_after_null': len(cwe_476_matches.get('dereference_after_null', [])),
                'risk_score': self._calculate_cwe_risk_score(cwe_476_matches, 'CWE-476')
            }
        
        # CWE-362: Race Condition
        cwe_362_matches = self._find_patterns(code, self.cwe_362_patterns)
        if cwe_362_matches:
            patterns_found['CWE-362'] = {
                'locking_operations': len(cwe_362_matches.get('locking_patterns', [])),
                'unlocking_operations': len(cwe_362_matches.get('unlocking_patterns', [])),
                'risk_score': self._calculate_cwe_risk_score(cwe_362_matches, 'CWE-362')
            }
        
        # CWE-119/787: Buffer Overflow/Out-of-bounds Write
        cwe_119_787_matches = self._find_patterns(code, self.cwe_119_787_patterns)
        if cwe_119_787_matches:
            patterns_found['CWE-119/787'] = {
                'buffer_operations': len(cwe_119_787_matches.get('buffer_operations', [])),
                'size_checks': len(cwe_119_787_matches.get('size_checks', [])),
                'risk_score': self._calculate_cwe_risk_score(cwe_119_787_matches, 'CWE-119/787')
            }
        
        # CWE-20: Input Validation
        cwe_20_matches = self._find_patterns(code, self.cwe_20_patterns)
        if cwe_20_matches:
            patterns_found['CWE-20'] = {
                'validation_checks': len(cwe_20_matches.get('validation_checks', [])),
                'risk_score': self._calculate_cwe_risk_score(cwe_20_matches, 'CWE-20')
            }
        
        # CWE-200: Information Exposure
        cwe_200_matches = self._find_patterns(code, self.cwe_200_patterns)
        if cwe_200_matches:
            patterns_found['CWE-200'] = {
                'info_exposure': len(cwe_200_matches.get('info_exposure', [])),
                'risk_score': self._calculate_cwe_risk_score(cwe_200_matches, 'CWE-200')
            }
        
        # CWE-125: Out-of-bounds Read
        cwe_125_matches = self._find_patterns(code, self.cwe_125_patterns)
        if cwe_125_matches:
            patterns_found['CWE-125'] = {
                'read_operations': len(cwe_125_matches.get('read_operations', [])),
                'risk_score': self._calculate_cwe_risk_score(cwe_125_matches, 'CWE-125')
            }
        
        # CWE-264: Access Control
        cwe_264_matches = self._find_patterns(code, self.cwe_264_patterns)
        if cwe_264_matches:
            patterns_found['CWE-264'] = {
                'access_control': len(cwe_264_matches.get('access_control', [])),
                'risk_score': self._calculate_cwe_risk_score(cwe_264_matches, 'CWE-264')
            }
        
        # CWE-401: Memory Leak
        cwe_401_matches = self._find_patterns(code, self.cwe_401_patterns)
        if cwe_401_matches:
            patterns_found['CWE-401'] = {
                'memory_allocation': len(cwe_401_matches.get('memory_allocation', [])),
                'memory_cleanup': len(cwe_401_matches.get('memory_cleanup', [])),
                'risk_score': self._calculate_cwe_risk_score(cwe_401_matches, 'CWE-401')
            }
        
        return patterns_found
    
    def _find_patterns(self, code: str, patterns: Dict[str, List[str]]) -> Dict[str, List[str]]:
        """Find all pattern matches in code"""
        matches = {}
        
        for pattern_type, pattern_list in patterns.items():
            pattern_matches = []
            for pattern in pattern_list:
                found = re.findall(pattern, code, re.MULTILINE | re.DOTALL)
                pattern_matches.extend(found)
            if pattern_matches:
                matches[pattern_type] = pattern_matches
        
        return matches
    
    def _calculate_cwe_risk_score(self, matches: Dict[str, List[str]], cwe_type: str) -> float:
        """Calculate risk score based on CWE patterns found"""
        
        # Use configured risk weights or fallback to hardcoded values
        base_weight = self.cwe_risk_weights.get(cwe_type, {
            'CWE-416': 0.9,    # Use After Free - Very High Risk
            'CWE-476': 0.8,    # NULL Pointer - High Risk
            'CWE-362': 0.7,    # Race Condition - High Risk
            'CWE-119/787': 0.9, # Buffer Overflow - Very High Risk
            'CWE-20': 0.6,     # Input Validation - Medium Risk
            'CWE-200': 0.4,    # Information Exposure - Low-Medium Risk
            'CWE-125': 0.7,    # Out-of-bounds Read - High Risk
            'CWE-264': 0.5,    # Access Control - Medium Risk
            'CWE-401': 0.6     # Memory Leak - Medium Risk
        }.get(cwe_type, 0.5))
        
        total_matches = sum(len(match_list) for match_list in matches.values())
        
        # Normalize by pattern count
        risk_score = min(1.0, (total_matches * base_weight) / 10.0)
        
        return risk_score
    
    def get_cwe_statistics(self) -> Dict[str, Any]:
        """Get CWE statistics from the dataset"""
        if self.dataset_stats:
            return self.dataset_stats
        else:
            # Fallback to hardcoded statistics
            return {
                'total_pairs': 2903,
                'training_pairs': 2317,
                'test_pairs': 586,
                'cwe_distribution': {
                    'CWE-416': {'pairs': 660, 'percentage': 22.7},  # Use After Free
                    'CWE-476': {'pairs': 281, 'percentage': 9.7},   # NULL Pointer
                    'CWE-362': {'pairs': 320, 'percentage': 11.0},  # Race Condition
                    'CWE-119': {'pairs': 173, 'percentage': 6.0},   # Buffer Overflow
                    'CWE-787': {'pairs': 187, 'percentage': 6.4},   # Out-of-bounds Write
                    'CWE-20': {'pairs': 182, 'percentage': 6.3},    # Input Validation
                    'CWE-200': {'pairs': 152, 'percentage': 5.2},   # Information Exposure
                    'CWE-125': {'pairs': 140, 'percentage': 4.8},   # Out-of-bounds Read
                    'CWE-264': {'pairs': 120, 'percentage': 4.1},   # Access Control
                    'CWE-401': {'pairs': 101, 'percentage': 3.5}    # Memory Leak
                }
            } 