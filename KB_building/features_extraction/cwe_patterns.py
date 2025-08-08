"""
CWE-Specific Pattern Recognition for Linux Kernel C/C++ Code
Based on Top-10 CWEs from linuxkernelcves.com dataset (website does not exist anymore)

This module now loads patterns from JSON configuration files.
Patterns are centralized in config/cwe_config.json for better maintainability.
Scientific approach with CERT-C standards compliance.
"""

from typing import Dict, List, Any
import re
import json
from pathlib import Path
from .logging_config import LoggingMixin, get_module_logger

class CWEPatternExtractor(LoggingMixin):
    """Extract CWE-specific patterns from Linux kernel code (JSON-based)"""
    
    def __init__(self, config_file: Path = None):
        super().__init__()
        self.config_file = config_file or Path(__file__).parent / "config" / "cwe_config.json"
        self.cwe_patterns = {}
        self.cwe_risk_weights = {}
        self.dataset_stats = {}
        self._load_config()
    
    def _load_config(self):
        """Load CWE patterns from JSON configuration"""
        try:
            if self.config_file.exists():
                with open(self.config_file, 'r') as f:
                    config = json.load(f)
                
                self.cwe_patterns = config.get('cwe_patterns', {})
                
                # Extract risk weights
                for cwe_id, cwe_data in self.cwe_patterns.items():
                    self.cwe_risk_weights[cwe_id] = cwe_data.get('risk_weight', 0.5)
                
                # Load dataset stats if available
                self.dataset_stats = config.get('dataset_stats', {})
                
                self.logger.info(f"Loaded CWE patterns for {len(self.cwe_patterns)} CWE types")
                self.logger.info(f"Risk weights: {self.cwe_risk_weights}")
                
            else:
                self.logger.error(f"CWE config file not found: {self.config_file}")
                
        except Exception as e:
            self.logger.error(f"Error loading CWE config: {e}")
    
    def extract_cwe_patterns(self, code: str) -> Dict[str, Any]:
        """
        Extract CWE-specific patterns from code using JSON configuration.
        
        Args:
            code: Source code string to analyze
            
        Returns:
            Dict containing matched patterns for each CWE type
        """
        results = {}
        
        for cwe_id, cwe_data in self.cwe_patterns.items():
            cwe_results = {
                'description': cwe_data.get('description', ''),
                'risk_weight': cwe_data.get('risk_weight', 0.5),
                'matched_patterns': {},
                'total_matches': 0
            }
            
            patterns = cwe_data.get('patterns', {})
            for pattern_type, pattern_list in patterns.items():
                matches = []
                for pattern in pattern_list:
                    try:
                        pattern_matches = re.findall(pattern, code, re.MULTILINE)
                        matches.extend(pattern_matches)
                    except re.error as e:
                        self.logger.warning(f"Invalid regex pattern {pattern}: {e}")
                
                cwe_results['matched_patterns'][pattern_type] = len(matches)
                cwe_results['total_matches'] += len(matches)
            
            results[cwe_id] = cwe_results
        
        return results
    
    def get_cwe_risk_score(self, cwe_id: str) -> float:
        """Get risk score for a specific CWE"""
        # Return 0.0 risk score if CWE is not supported
        return self.cwe_risk_weights.get(cwe_id, 0.0)
    
    def get_supported_cwes(self) -> List[str]:
        """Get list of supported CWE identifiers"""
        return list(self.cwe_patterns.keys())