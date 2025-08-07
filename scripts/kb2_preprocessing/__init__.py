"""
KB2 Preprocessing Module

This module provides tools for building the KB2 hybrid knowledge base
from Joern CPG (Code Property Graph) data.

Components:
- graphson_parser: GraphSON v3 TinkerPop parsing utilities
- feature_extractor: Structural and semantic feature extraction
- cwe_patterns: CWE-specific pattern detection for Linux kernel
- feature_utils: Common utility functions for feature extraction
- logging_config: Centralized logging configuration
- kb2_builder: Complete KB2 construction pipeline
- kb1_integration: Integration with existing KB1 metadata

Usage:
    from scripts.kb2_preprocessing import extract_kb2_features
    features = extract_kb2_features(cpg_file)
    
    from scripts.kb2_preprocessing import CWEPatternExtractor
    cwe_extractor = CWEPatternExtractor()
    patterns = cwe_extractor.extract_cwe_patterns(code)
"""

__version__ = "1.0.0"
__author__ = "VulRAG-Hybrid-System"

# ✅ Core parsing and feature extraction
from .graphson_parser import GraphSONParser, analyze_cpg_file
from .feature_extractor import (
    StructuralFeatureExtractor, 
    SemanticFeatureExtractor,
    HybridFeatureExtractor,
    extract_kb2_features
)

# ✅ CWE pattern detection
from .cwe_patterns import CWEPatternExtractor

# ✅ Utility functions
from .feature_utils import FeatureUtils

# ✅ Logging configuration
from .logging_config import (
    LoggingMixin, 
    setup_logging, 
    get_module_logger,
    setup_default_logging
)

# Future imports (when modules are created):
# from .kb2_builder import KB2Builder
# from .kb1_integration import KB1Integrator

__all__ = [
    # Core components
    'GraphSONParser',
    'analyze_cpg_file',
    'StructuralFeatureExtractor', 
    'SemanticFeatureExtractor',
    'HybridFeatureExtractor',
    'extract_kb2_features',
    
    # CWE pattern detection
    'CWEPatternExtractor',
    
    # Utility functions
    'FeatureUtils',
    
    # Logging configuration
    'LoggingMixin',
    'setup_logging',
    'get_module_logger',
    'setup_default_logging'
    
    # Future components
    # 'KB2Builder',
    # 'KB1Integrator'
]