"""
Vulnrag: Hybrid Structural-Semantic Vulnerability Retrieval System

This module implements the core Vulnrag hybrid retrieval algorithm that combines:
- Structural features (60% weight): Graph topology, complexity metrics, control flow
- Semantic features (40% weight): Function calls, API patterns, call diversity

Main Components:
- VulnragRetriever: Main hybrid retrieval engine
- SimilarityEngine: Structural + Semantic similarity calculations  
- FeatureNormalizer: StandardScaler for feature vectors
- ResultRanker: Context-aware ranking and post-processing

Architecture:
Input Code → CPG → Feature Extraction → Hybrid Search → Top-K Results
"""

# Import O3 LLM Engine (available now)
from .llm_engine import O3VulnragEngine, VulnerabilityAnalysis, create_vulnrag_o3_engine

# Import Configuration System
from .config import VulnragConfig, get_config, set_config, create_default_config

# Import Core Retrieval System ✅
from .vulnrag_retriever import (
    VulnragRetriever, 
    RetrievalResult, 
    VulnragRetrievalResults,
    create_vulnrag_retriever,
    quick_vulnrag_analysis
)

# Other modules (to be implemented if needed)
# from .similarity_engine import SimilarityEngine
# from .feature_normalizer import FeatureNormalizer
# from .result_ranker import ResultRanker

__all__ = [
    # LLM Engine (O3 Integration) - Available ✅
    'O3VulnragEngine',
    'VulnerabilityAnalysis', 
    'create_vulnrag_o3_engine',
    
    # Configuration System - Available ✅
    'VulnragConfig',
    'get_config',
    'set_config', 
    'create_default_config',
    
    # Core Retrieval System - Available ✅
    'VulnragRetriever',
    'RetrievalResult',
    'VulnragRetrievalResults', 
    'create_vulnrag_retriever',
    'quick_vulnrag_analysis'
]

__version__ = "1.0.0"
__author__ = "Vulnrag Research Team"
__description__ = "Hybrid Structural-Semantic Vulnerability Retrieval System"