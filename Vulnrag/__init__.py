"""
Vulnrag: Hybrid Structural-Semantic Vulnerability Retrieval System

This module implements the core Vulnrag hybrid retrieval algorithm that combines:
- Structural features: Graph topology, complexity metrics, control flow
- Semantic features: Function calls, API patterns, call diversity
- Embedding features: OpenAI text-embedding-ada-002

Main Components:
- VulnragRetriever: Main hybrid retrieval engine with RRF fusion
- VulnragLLMEngine: LLM integration for vulnerability analysis
- VulnerabilityAnalysis: Structured output from LLM analysis

Architecture:
Input Code → CPG → Feature Extraction → RRF Hybrid Search → Top-K Results
"""

# Import LLM Engine (available now)
from .llm_engine import VulnragLLMEngine, VulnerabilityAnalysis, create_neutral_vulnrag_engine

# Import Core Retrieval System ✅
from .vulnrag_retriever import (
    VulnragRetriever, 
    RetrievalResult, 
    VulnragRetrievalResults,
    create_vulnrag_retriever,
    quick_vulnrag_analysis
)

__all__ = [
    # LLM Engine - Available ✅
    'VulnragLLMEngine',
    'VulnerabilityAnalysis', 
    'create_neutral_vulnrag_engine',
    
    # Core Retrieval System - Available ✅
    'VulnragRetriever',
    'RetrievalResult',
    'VulnragRetrievalResults', 
    'create_vulnrag_retriever',
    'quick_vulnrag_analysis'
]

__version__ = "1.0.0"
__author__ = "Vulnrag Research Team"
__description__ = "Hybrid RRF-based Vulnerability Retrieval System"