"""
Feature Extraction Utilities

Common utilities for feature extraction including entropy calculations,
ratio computations, and statistical metrics.
"""

import math
import logging
from typing import Dict, List, Any, Counter
from collections import Counter as CounterType

logger = logging.getLogger(__name__)

class FeatureUtils:
    """Utility class for common feature extraction calculations"""
    
    @staticmethod
    def calculate_entropy(probabilities: List[float]) -> float:
        """
        Calculate Shannon entropy for a list of probabilities.
        
        Args:
            probabilities: List of probability values
            
        Returns:
            float: Shannon entropy value
        """
        entropy = 0.0
        for prob in probabilities:
            if prob > 0:
                entropy -= prob * math.log2(prob)
        return entropy
    
    @staticmethod
    def calculate_ratio(numerator: int, denominator: int) -> float:
        """
        Calculate ratio with safe division.
        
        Args:
            numerator: Numerator value
            denominator: Denominator value
            
        Returns:
            float: Ratio value (0.0 if denominator is 0)
        """
        return numerator / denominator if denominator > 0 else 0.0
    
    @staticmethod
    def calculate_diversity_ratio(unique_count: int, total_count: int) -> float:
        """
        Calculate diversity ratio (unique/total).
        
        Args:
            unique_count: Number of unique items
            total_count: Total number of items
            
        Returns:
            float: Diversity ratio
        """
        return FeatureUtils.calculate_ratio(unique_count, total_count)
    
    @staticmethod
    def calculate_call_entropy(call_counts: CounterType[str]) -> float:
        """
        Calculate call entropy from call frequency counts.
        
        Args:
            call_counts: Counter of call frequencies
            
        Returns:
            float: Call entropy value
        """
        if not call_counts:
            return 0.0
        
        total_calls = sum(call_counts.values())
        probabilities = [count / total_calls for count in call_counts.values()]
        return FeatureUtils.calculate_entropy(probabilities)
    
    @staticmethod
    def normalize_risk_score(risk_score: float, max_score: float = 1.0) -> float:
        """
        Normalize risk score to [0, 1] range.
        
        Args:
            risk_score: Raw risk score
            max_score: Maximum possible score
            
        Returns:
            float: Normalized risk score
        """
        return min(1.0, risk_score / max_score)
    
    @staticmethod
    def calculate_weighted_score(scores: Dict[str, float], weights: Dict[str, float]) -> float:
        """
        Calculate weighted average score.
        
        Args:
            scores: Dictionary of scores
            weights: Dictionary of weights
            
        Returns:
            float: Weighted average score
        """
        if not scores or not weights:
            return 0.0
        
        weighted_sum = 0.0
        total_weight = 0.0
        
        for key, score in scores.items():
            weight = weights.get(key, 0.0)
            weighted_sum += score * weight
            total_weight += weight
        
        return weighted_sum / total_weight if total_weight > 0 else 0.0
    
    @staticmethod
    def filter_by_threshold(data: Dict[str, Any], threshold: float, key: str = 'risk_score') -> Dict[str, Any]:
        """
        Filter dictionary by threshold value.
        
        Args:
            data: Dictionary to filter
            threshold: Threshold value
            key: Key to check against threshold
            
        Returns:
            Dict: Filtered dictionary
        """
        return {k: v for k, v in data.items() 
                if isinstance(v, dict) and v.get(key, 0) > threshold}
    
    @staticmethod
    def extract_top_items(items: List[Any], max_items: int = 10) -> List[Any]:
        """
        Extract top N items from a list.
        
        Args:
            items: List of items
            max_items: Maximum number of items to return
            
        Returns:
            List: Top N items
        """
        return items[:max_items]
    
    @staticmethod
    def count_pattern_matches(text: str, patterns: List[str]) -> int:
        """
        Count pattern matches in text.
        
        Args:
            text: Text to search in
            patterns: List of regex patterns
            
        Returns:
            int: Number of matches
        """
        import re
        total_matches = 0
        for pattern in patterns:
            matches = re.findall(pattern, text, re.MULTILINE | re.DOTALL)
            total_matches += len(matches)
        return total_matches
    
    @staticmethod
    def validate_feature_completeness(features: Dict[str, Any]) -> Dict[str, Any]:
        """
        Validate completeness of extracted features.
        
        Args:
            features: Dictionary of extracted features
            
        Returns:
            Dict: Validation results
        """
        validation = {
            'has_structural': 'structural_features' in features,
            'has_semantic': 'semantic_features' in features,
            'has_cwe_patterns': 'cwe_patterns' in features.get('semantic_features', {}),
            'completeness_score': 0.0
        }
        
        # Calculate completeness score
        score = 0.0
        if validation['has_structural']:
            score += 0.4
        if validation['has_semantic']:
            score += 0.4
        if validation['has_cwe_patterns']:
            score += 0.2
        
        validation['completeness_score'] = score
        return validation
    
    @staticmethod
    def log_feature_extraction_stats(features: Dict[str, Any], logger: logging.Logger = None):
        """
        Log feature extraction statistics.
        
        Args:
            features: Extracted features
            logger: Logger instance
        """
        if logger is None:
            logger = logging.getLogger(__name__)
        
        if 'structural_features' in features:
            structural = features['structural_features']
            logger.info(f"Structural features: {len(structural)} metrics extracted")
        
        if 'semantic_features' in features:
            semantic = features['semantic_features']
            logger.info(f"Semantic features: {len(semantic)} categories extracted")
            
            if 'cwe_patterns' in semantic:
                cwe_count = len(semantic['cwe_patterns'])
                logger.info(f"CWE patterns: {cwe_count} types detected")
        
        validation = FeatureUtils.validate_feature_completeness(features)
        logger.info(f"Feature completeness: {validation['completeness_score']:.2f}") 