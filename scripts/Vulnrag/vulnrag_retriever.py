"""
Vulnrag Hybrid Retriever - Core Retrieval Engine

This module implements the main Vulnrag hybrid retrieval algorithm with:
- Double indexation: VULN vs PATCH patterns
- Triple hybrid: Structural (0.4) + TF-IDF (0.3) + Embeddings (0.3)
- Min-Max normalization for fair comparison
- Simple classification: VULNERABLE vs SAFE vs UNCERTAIN

Architecture:
Input Code → Feature Extraction → Double Search → Classification → Results
"""

import json
import os
import time
import numpy as np
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Any
from dataclasses import dataclass, asdict
from sklearn.feature_extraction.text import TfidfVectorizer
from sklearn.metrics.pairwise import cosine_similarity
from sklearn.preprocessing import StandardScaler
import joblib
import faiss
import logging
from openai import OpenAI

# Add project root to path for imports
project_root = Path(__file__).parent.parent.parent
import sys
sys.path.insert(0, str(project_root))

from scripts.kb2_preprocessing import extract_kb2_features
# Configuration directe - pas de config.py

@dataclass
class RetrievalResult:
    """Single retrieval result with similarity scores and metadata"""
    entry_id: str
    hybrid_score: float
    structural_score: float
    tfidf_score: float
    embedding_score: float
    cve_id: str
    cwe_id: str
    pattern_type: str  # "VULN" or "PATCH"
    structural_features: Dict
    semantic_features: Dict
    vulnerability_info: Dict
    instance_id: Optional[str] = None

    def to_dict(self) -> Dict:
        """Convert to dictionary for serialization"""
        return asdict(self)

@dataclass
class VulnragRetrievalResults:
    """Complete retrieval results with classification and evidence"""
    query_id: str
    top_results: List[RetrievalResult]
    classification: str  # "VULNERABLE", "SAFE", "UNCERTAIN"
    best_vuln_score: float
    best_patch_score: float
    search_time_seconds: float
    retrieval_metadata: Dict
    evidence_for: List[Dict]  # Evidence supporting vulnerability
    evidence_against: List[Dict]  # Evidence against vulnerability

    def to_dict(self) -> Dict:
        """Convert to dictionary for serialization"""
        return {
            "query_id": self.query_id,
            "top_results": [r.to_dict() for r in self.top_results],
            "classification": self.classification,
            "best_vuln_score": self.best_vuln_score,
            "best_patch_score": self.best_patch_score,
            "search_time_seconds": self.search_time_seconds,
            "retrieval_metadata": self.retrieval_metadata,
            "evidence_for": self.evidence_for,
            "evidence_against": self.evidence_against
        }

class VulnragRetriever:
    """
    Main Vulnrag hybrid retrieval engine with double indexation
    
    Features:
    - Double indexation: VULN vs PATCH patterns
    - Triple hybrid: Structural + TF-IDF + Embeddings
    - Min-Max normalization for fair comparison
    - Simple classification logic
    """

    def __init__(self):
        """
        Initialize Vulnrag retriever with double indexation
        """
        self.logger = self._setup_logging()
        
        # Load KB2 metadata
        self.kb2_data = self._load_kb2()
        self.total_entries = len(self.kb2_data)
        
        # Load double indexation
        self._load_double_indexation()
        
        # Initialize OpenAI client for query embeddings
        openai_api_key = os.getenv('OPENAI_API_KEY')
        if not openai_api_key:
            raise ValueError("OPENAI_API_KEY environment variable is required")
        self.openai_client = OpenAI(api_key=openai_api_key)
        
        # Initialize embedding cache
        self.embedding_cache = {}
        # Get project root for cache path
        project_root = Path(__file__).parent.parent.parent
        self.cache_file = project_root / "data" / "cache" / "query_embeddings.json"
        self._load_embedding_cache()
        
        self.logger.info(f"VulnragRetriever initialized with {self.total_entries} KB2 entries")

    def build_evidence_from_patterns(self, vuln_results: List[RetrievalResult], 
                                   patch_results: List[RetrievalResult]) -> Dict[str, List[Dict]]:
        """
        Build evidence_for and evidence_against from retrieval patterns with rich KB2 metadata
        
        Args:
            vuln_results: VULN pattern results
            patch_results: PATCH pattern results
            
        Returns:
            Dictionary with evidence_for and evidence_against
        """
        evidence_for = []
        evidence_against = []
        
        # Build evidence_for from VULN patterns (vulnerability evidence)
        for pattern in vuln_results:
            kb1_meta = pattern.vulnerability_info  # Direct access, no double indirection
            evidence_for.append({
                "cve": pattern.cve_id,
                "cwe": pattern.cwe_id,
                "vulnerability_type": kb1_meta.get('vulnerability_type', 'N/A'),
                "trigger_condition": kb1_meta.get('trigger_condition', 'N/A'),
                "specific_behavior": kb1_meta.get('specific_code_behavior_causing_vulnerability', 'N/A'),
                "preconditions": kb1_meta.get('preconditions_for_vulnerability', 'N/A'),
                "code_before": kb1_meta.get('code_before_change', 'N/A'),
                "similarity": pattern.hybrid_score,
                "pattern_type": "VULN"
            })
        
        # Build evidence_against from PATCH patterns (safety evidence)
        for pattern in patch_results:
            kb1_meta = pattern.vulnerability_info  # Direct access, no double indirection
            evidence_against.append({
                "cve": pattern.cve_id,
                "cwe": pattern.cwe_id,
                "solution": kb1_meta.get('solution', 'N/A'),
                "gpt_analysis": kb1_meta.get('GPT_analysis', 'N/A'),
                "modified_lines": kb1_meta.get('modified_lines', 'N/A'),
                "code_after": kb1_meta.get('code_after_change', 'N/A'),
                "gpt_purpose": kb1_meta.get('gpt_purpose', 'N/A'),
                "similarity": pattern.hybrid_score,
                "pattern_type": "PATCH"
            })
        
        # Add differential analysis for high-similarity cases
        differential_insights = self._analyze_key_differences(vuln_results, patch_results)
        
        return {
            "evidence_for": evidence_for,
            "evidence_against": evidence_against,
            "differential_analysis": differential_insights
        }

    def _analyze_key_differences(self, vuln_results: List[RetrievalResult], 
                                patch_results: List[RetrievalResult]) -> Dict[str, any]:
        """
        Analyze key differences between vulnerable and patched patterns to provide
        focused insights instead of just similarity scores
        """
        if not vuln_results or not patch_results:
            return {"status": "insufficient_data"}
        
        # Get best scoring patterns
        best_vuln = max(vuln_results, key=lambda x: x.hybrid_score)
        best_patch = max(patch_results, key=lambda x: x.hybrid_score)
        
        # Check if they're the same CVE (common case - vuln/patch pair)
        same_cve = best_vuln.cve_id == best_patch.cve_id
        
        # Calculate similarity metrics
        score_diff = abs(best_vuln.hybrid_score - best_patch.hybrid_score)
        avg_similarity = (best_vuln.hybrid_score + best_patch.hybrid_score) / 2
        
        # Extract key security patterns from metadata
        vuln_meta = best_vuln.vulnerability_info
        patch_meta = best_patch.vulnerability_info
        
        # Identify security-critical differences
        security_differences = []
        
        # Check for input validation patterns
        vuln_trigger = vuln_meta.get('trigger_condition', '').lower()
        patch_solution = patch_meta.get('solution', '').lower()
        
        if 'validation' in patch_solution and 'validation' not in vuln_trigger:
            security_differences.append("Input validation added in patch")
        if 'bounds check' in patch_solution or 'range check' in patch_solution:
            security_differences.append("Bounds checking introduced")
        if 'null check' in patch_solution or 'null ptr' in patch_solution:
            security_differences.append("Null pointer validation added")
        if 'lock' in patch_solution or 'mutex' in patch_solution:
            security_differences.append("Synchronization mechanism added")
        if 'free' in patch_solution and 'after' in patch_solution:
            security_differences.append("Memory management improved")
        
        return {
            "status": "analyzed",
            "same_cve_pair": same_cve,
            "score_differential": score_diff,
            "average_similarity": avg_similarity,
            "high_similarity_both": avg_similarity > 0.9,
            "security_critical_differences": security_differences,
            "confidence_level": "low" if score_diff < 0.05 else "medium" if score_diff < 0.15 else "high",
            "recommendation": self._get_analysis_recommendation(score_diff, avg_similarity, same_cve)
        }
    
    def _get_analysis_recommendation(self, score_diff: float, avg_sim: float, same_cve: bool) -> str:
        """Get recommendation based on differential analysis"""
        if avg_sim > 0.95 and score_diff < 0.05:
            return "VERY_HIGH_SIMILARITY - Focus on subtle differences, be skeptical of vulnerability claims"
        elif same_cve and score_diff < 0.1:
            return "SAME_CVE_PAIR - Likely comparing vulnerable code to its patch, examine specific fix details"
        elif avg_sim > 0.9:
            return "HIGH_SIMILARITY - Common code patterns, look for security-critical differences"
        else:
            return "MODERATE_SIMILARITY - Standard analysis appropriate"

    def _setup_logging(self) -> logging.Logger:
        """Setup logging for retriever"""
        logger = logging.getLogger('VulnragRetriever')
        logger.setLevel(logging.INFO)

        if not logger.handlers:
            # Console handler
            console_handler = logging.StreamHandler()
            console_formatter = logging.Formatter(
                '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
            )
            console_handler.setFormatter(console_formatter)
            logger.addHandler(console_handler)
            
            # File handler
            project_root = Path(__file__).parent.parent.parent
            log_dir = project_root / "logs"
            log_dir.mkdir(exist_ok=True)
            file_handler = logging.FileHandler(log_dir / "vulnrag_retriever.log")
            file_formatter = logging.Formatter(
                '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
            )
            file_handler.setFormatter(file_formatter)
            logger.addHandler(file_handler)

        return logger

    def _load_kb2(self) -> Dict[str, Any]:
        """Load KB2 metadata from file"""
        try:
            # Get project root and KB2 path
            project_root = Path(__file__).parent.parent.parent
            kb2_path = project_root / "data" / "processed" / "kb2.json"
            with open(kb2_path, 'r', encoding='utf-8') as f:
                kb2_data = json.load(f)
            self.logger.info(f"Loaded KB2 with {len(kb2_data)} entries")
            return kb2_data
        except Exception as e:
            self.logger.error(f"Failed to load KB2: {e}")
            raise

    def _load_double_indexation(self):
        """Load double indexation: VULN vs PATCH"""
        # Get project root (2 levels up from scripts/Vulnrag)
        project_root = Path(__file__).parent.parent.parent
        data_dir = project_root / "data" / "processed"
        
        # Load VULN indexation
        self.vuln_embeddings = np.load(data_dir/'kb2_vuln_embeddings.npy')
        self.vuln_struct = np.load(data_dir/'kb2_vuln_struct.npy')
        self.vuln_tfidf_vectorizer = joblib.load(data_dir/'kb2_vuln_tfidf.pkl')
        self.vuln_tfidf_matrix = joblib.load(data_dir/'kb2_vuln_tfidf.npz')
        self.vuln_faiss = faiss.read_index(str(data_dir/'kb2_vuln_embeddings.faiss'))
        
        # Load PATCH indexation
        self.patch_embeddings = np.load(data_dir/'kb2_patch_embeddings.npy')
        self.patch_struct = np.load(data_dir/'kb2_patch_struct.npy')
        self.patch_tfidf_vectorizer = joblib.load(data_dir/'kb2_patch_tfidf.pkl')
        self.patch_tfidf_matrix = joblib.load(data_dir/'kb2_patch_tfidf.npz')
        self.patch_faiss = faiss.read_index(str(data_dir/'kb2_patch_embeddings.faiss'))
        
        # Load scalers
        self.vuln_struct_scaler = joblib.load(data_dir/'kb2_vuln_struct_scaler.pkl')
        self.patch_struct_scaler = joblib.load(data_dir/'kb2_patch_struct_scaler.pkl')
        
        self.logger.info(f"Loaded double indexation: VULN {self.vuln_embeddings.shape}, PATCH {self.patch_embeddings.shape}")

    def _load_embedding_cache(self):
        """Load embedding cache from disk"""
        if self.cache_file.exists():
            try:
                with open(self.cache_file, 'r') as f:
                    self.embedding_cache = json.load(f)
                self.logger.info(f"Loaded {len(self.embedding_cache)} cached embeddings")
            except Exception as e:
                self.logger.warning(f"Failed to load cache: {e}")

    def _save_embedding_cache(self):
        """Save embedding cache to disk"""
        try:
            self.cache_file.parent.mkdir(exist_ok=True)
            with open(self.cache_file, 'w') as f:
                json.dump(self.embedding_cache, f)
        except Exception as e:
            self.logger.warning(f"Failed to save cache: {e}")

    def get_query_embedding(self, code: str) -> List[float]:
        """Generate or retrieve embedding for query code"""
        code_hash = str(hash(code))
        
        # Check cache first
        if code_hash in self.embedding_cache:
            return self.embedding_cache[code_hash]
        
        # Generate embedding
        try:
            response = self.openai_client.embeddings.create(
                model="text-embedding-ada-002",
                input=code
            )
            embedding = response.data[0].embedding
            
            # Save to cache
            self.embedding_cache[code_hash] = embedding
            self._save_embedding_cache()
            
            return embedding
        except Exception as e:
            self.logger.error(f"Failed to generate embedding: {e}")
            return [0.0] * 1536  # Fallback

    def extract_query_features(self, code: str) -> Tuple[np.ndarray, str, List[float]]:
        """
        Extract features from query code using Joern CPG
        
        Args:
            code: Source code to analyze
            
        Returns:
            Tuple of (structural_vector, semantic_text, embedding)
        """
        self.logger.debug("Extracting features from query code")
        
        try:
            # Use Joern CPG + kb2_preprocessing
            features = self._extract_joern_features(code)
            
            # Extract structural vector (12 dimensions)
            struct_features = features.get('structural_features', {})
            structural_vector = np.array([
                struct_features.get('total_nodes', 0),
                struct_features.get('total_edges', 0),
                struct_features.get('graph_density', 0.0),
                struct_features.get('cyclomatic_complexity', 0),
                struct_features.get('nesting_depth', 0),
                struct_features.get('essential_complexity', 0),
                struct_features.get('control_structures', 0),
                struct_features.get('blocks', 0),
                struct_features.get('methods', 0),
                struct_features.get('call_entropy', 0.0),
                struct_features.get('calls_per_node', 0.0),
                struct_features.get('control_ratio', 0.0)
            ]).reshape(1, -1)
            
            # Extract semantic text
            semantic_features = features.get('semantic_features', {})
            semantic_text = semantic_features.get('combined_text', '')
            
            # Generate embedding
            embedding = self.get_query_embedding(code)
            
            self.logger.debug(f"Extracted features: structural={structural_vector.shape}, semantic_length={len(semantic_text)}")
            return structural_vector, semantic_text, embedding
            
        except Exception as e:
            self.logger.error(f"Feature extraction failed: {e}")
            # Return zero features as fallback
            zero_structural = np.zeros((1, 12))
            empty_semantic = ""
            zero_embedding = [0.0] * 1536
            return zero_structural, empty_semantic, zero_embedding

    def _extract_joern_features(self, code: str) -> Dict[str, Any]:
        """
        Extract features using Joern CPG + kb2_preprocessing pipeline
        
        Args:
            code: Source code string
            
        Returns:
            Dictionary with structural and semantic features
        """
        import tempfile
        import subprocess
        import os
        
        # Use temporary directory
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_dir_path = Path(temp_dir)
            
            # Write code to temporary C file
            temp_c_file = temp_dir_path / "query_code.c"
            with open(temp_c_file, 'w', encoding='utf-8') as f:
                f.write(code)
            
            # Generate CPG using Joern
            cpg_json_file = temp_dir_path / "query_cpg.json"
            
            if self._generate_cpg_with_joern(temp_c_file, cpg_json_file):
                # Use kb2_preprocessing to extract features
                try:
                    features = extract_kb2_features(cpg_json_file)
                    self.logger.debug("Successfully extracted features using Joern + kb2_preprocessing")
                    return features
                except Exception as e:
                    self.logger.error(f"kb2_preprocessing extraction failed: {e}")
                    raise
            else:
                raise Exception("Joern CPG generation failed")

    def _generate_cpg_with_joern(self, source_file: Path, output_json: Path) -> bool:
        """
        Generate CPG JSON using Joern
        
        Args:
            source_file: Path to C source file
            output_json: Path for output GraphSON JSON
            
        Returns:
            True if successful, False otherwise
        """
        import tempfile
        import subprocess
        
        try:
            with tempfile.TemporaryDirectory() as temp_dir:
                temp_dir_path = Path(temp_dir)
                cpg_file = temp_dir_path / "code.cpg.bin"
                export_dir = temp_dir_path / "export"
                
                # Step 1: Parse source file to CPG binary
                self.logger.debug(f"Running joern-parse on {source_file}")
                parse_cmd = ["joern-parse", "--output", str(cpg_file), str(source_file)]
                result = subprocess.run(
                    parse_cmd,
                    capture_output=True,
                    text=True,
                    timeout=30,
                    env={**os.environ, "JAVA_OPTIONS": "-Xms512m -Xmx1g"}
                )
                
                if result.returncode != 0:
                    self.logger.error(f"joern-parse failed: {result.stderr}")
                    return False
                
                # Step 2: Export CPG to GraphSON JSON
                self.logger.debug(f"Running joern-export on {cpg_file}")
                export_cmd = [
                    "joern-export", str(cpg_file),
                    "--repr=all", "--format=graphson",
                    f"--out={export_dir}"
                ]
                result = subprocess.run(
                    export_cmd,
                    capture_output=True,
                    text=True,
                    timeout=30,
                    env={**os.environ, "JAVA_OPTIONS": "-Xms512m -Xmx1g"}
                )
                
                if result.returncode != 0:
                    self.logger.error(f"joern-export failed: {result.stderr}")
                    return False
                
                # Step 3: Find and move the exported JSON file
                json_files = list(export_dir.glob("*.json")) + list(export_dir.glob("*.graphson"))
                if json_files:
                    import shutil
                    shutil.move(str(json_files[0]), str(output_json))
                    self.logger.debug(f"CPG JSON generated successfully: {output_json}")
                    return True
                else:
                    self.logger.error("No JSON/GraphSON file found after export")
                    return False
                    
        except subprocess.TimeoutExpired:
            self.logger.error("Joern process timed out")
            return False
        except Exception as e:
            self.logger.error(f"Error in Joern CPG generation: {e}")
            return False

    def minmax_norm(self, scores: np.ndarray, clip: bool = True) -> np.ndarray:
        """
        Min-Max normalization with optional clipping
        
        Args:
            scores: Array of scores to normalize
            clip: Whether to clip values to [0, 1]
            
        Returns:
            Normalized scores in [0, 1] range
        """
        mn, mx = scores.min(), scores.max()
        
        # Handle constant case
        if mx - mn < 1e-8:
            return np.zeros_like(scores)
        
        # Min-Max normalization
        normalized = (scores - mn) / (mx - mn)
        
        # Clipping to avoid floating-point issues
        if clip:
            normalized = np.clip(normalized, 0.0, 1.0)
        
        return normalized

    def search_vuln_patterns(self, code: str, top_k: int = 10) -> List[RetrievalResult]:
        """
        Search for VULN patterns using triple hybrid approach
        
        Args:
            code: Source code to analyze
            top_k: Number of top results to return
            
        Returns:
            List of RetrievalResult for VULN patterns
        """
        # Extract features
        struct_vec, semantic_text, embedding = self.extract_query_features(code)
        
        # Calculate similarities
        structural_scores = cosine_similarity(
            self.vuln_struct_scaler.transform(struct_vec), 
            self.vuln_struct
        ).flatten()
        
        tfidf_scores = cosine_similarity(
            self.vuln_tfidf_vectorizer.transform([semantic_text]),
            self.vuln_tfidf_matrix
        ).flatten()
        
        embedding_scores = cosine_similarity(
            [embedding], 
            self.vuln_embeddings
        ).flatten()
        
        # Normalize scores
        structural_norm = self.minmax_norm(structural_scores)
        tfidf_norm = self.minmax_norm(tfidf_scores)
        embedding_norm = self.minmax_norm(embedding_scores)
        
        # Triple hybrid fusion (0.4/0.3/0.3)
        hybrid_scores = (0.4 * structural_norm + 
                        0.3 * tfidf_norm + 
                        0.3 * embedding_norm)
        
        # Get top-K results
        top_indices = np.argsort(hybrid_scores)[-top_k:][::-1]
        
        # Create retrieval results
        results = []
        for i, idx in enumerate(top_indices):
            entry_id = list(self.kb2_data.keys())[idx]
            entry_data = self.kb2_data[entry_id]
            
            result = RetrievalResult(
                entry_id=entry_id,
                hybrid_score=float(hybrid_scores[idx]),
                structural_score=float(structural_scores[idx]),
                tfidf_score=float(tfidf_scores[idx]),
                embedding_score=float(embedding_scores[idx]),
                cve_id=entry_data.get('cve', 'Unknown'),
                cwe_id=entry_data.get('cwe', 'Unknown'),
                pattern_type="VULN",
                structural_features=entry_data.get('vuln_features', {}).get('structural_features', {}),
                semantic_features=entry_data.get('vuln_features', {}).get('semantic_features', {}),
                vulnerability_info=entry_data.get('kb1_metadata', {}),
                instance_id=entry_data.get('instance_id')
            )
            results.append(result)
        
        return results

    def search_patch_patterns(self, code: str, top_k: int = 10) -> List[RetrievalResult]:
        """
        Search for PATCH patterns using triple hybrid approach
        
        Args:
            code: Source code to analyze
            top_k: Number of top results to return
            
        Returns:
            List of RetrievalResult for PATCH patterns
        """
        # Extract features
        struct_vec, semantic_text, embedding = self.extract_query_features(code)
        
        # Calculate similarities
        structural_scores = cosine_similarity(
            self.patch_struct_scaler.transform(struct_vec), 
            self.patch_struct
        ).flatten()
        
        tfidf_scores = cosine_similarity(
            self.patch_tfidf_vectorizer.transform([semantic_text]),
            self.patch_tfidf_matrix
        ).flatten()
        
        embedding_scores = cosine_similarity(
            [embedding], 
            self.patch_embeddings
        ).flatten()
        
        # Normalize scores
        structural_norm = self.minmax_norm(structural_scores)
        tfidf_norm = self.minmax_norm(tfidf_scores)
        embedding_norm = self.minmax_norm(embedding_scores)
        
        # Triple hybrid fusion (0.4/0.3/0.3)
        hybrid_scores = (0.4 * structural_norm + 
                        0.3 * tfidf_norm + 
                        0.3 * embedding_norm)
        
        # Get top-K results
        top_indices = np.argsort(hybrid_scores)[-top_k:][::-1]
        
        # Create retrieval results
        results = []
        for i, idx in enumerate(top_indices):
            entry_id = list(self.kb2_data.keys())[idx]
            entry_data = self.kb2_data[entry_id]
            
            result = RetrievalResult(
                entry_id=entry_id,
                hybrid_score=float(hybrid_scores[idx]),
                structural_score=float(structural_scores[idx]),
                tfidf_score=float(tfidf_scores[idx]),
                embedding_score=float(embedding_scores[idx]),
                cve_id=entry_data.get('cve', 'Unknown'),
                cwe_id=entry_data.get('cwe', 'Unknown'),
                pattern_type="PATCH",
                structural_features=entry_data.get('patch_features', {}).get('structural_features', {}),
                semantic_features=entry_data.get('patch_features', {}).get('semantic_features', {}),
                vulnerability_info=entry_data.get('kb1_metadata', {}),
                instance_id=entry_data.get('instance_id')
            )
            results.append(result)
        
        return results

    def get_retrieval_scores(self, vuln_results: List[RetrievalResult], 
                            patch_results: List[RetrievalResult]) -> Tuple[float, float]:
        """
        Get best scores from VULN and PATCH patterns
        
        Args:
            vuln_results: VULN pattern results
            patch_results: PATCH pattern results
            
        Returns:
            Tuple of (best_vuln_score, best_patch_score)
        """
        best_vuln = max([r.hybrid_score for r in vuln_results]) if vuln_results else 0.0
        best_patch = max([r.hybrid_score for r in patch_results]) if patch_results else 0.0
        
        return best_vuln, best_patch

    def hybrid_retrieval(self, code: str, top_k: int = 10, 
                        query_id: Optional[str] = None) -> VulnragRetrievalResults:
        """
        Perform hybrid retrieval with double indexation and classification
        
        Args:
            code: Source code to analyze
            top_k: Number of top results per pattern type
            query_id: Optional query identifier
            
        Returns:
            VulnragRetrievalResults with classification
        """
        start_time = time.time()
        query_id = query_id or f"query_{int(start_time)}"
        
        self.logger.info(f"Starting hybrid retrieval for query {query_id}")
        
        # Double search
        vuln_results = self.search_vuln_patterns(code, top_k=top_k)
        patch_results = self.search_patch_patterns(code, top_k=top_k)
        
        # Get best scores (no classification)
        best_vuln_score, best_patch_score = self.get_retrieval_scores(
            vuln_results, patch_results
        )
        
        # Combine results
        all_results = vuln_results + patch_results
        all_results.sort(key=lambda r: r.hybrid_score, reverse=True)
        
        # Build evidence from patterns
        evidence = self.build_evidence_from_patterns(vuln_results, patch_results)
        
        search_time = time.time() - start_time
        
        # Create final results
        retrieval_results = VulnragRetrievalResults(
            query_id=query_id,
            top_results=all_results,
            classification="NEUTRAL",  # No classification
            best_vuln_score=best_vuln_score,
            best_patch_score=best_patch_score,
            search_time_seconds=search_time,
            retrieval_metadata={
                "total_vuln_patterns": len(vuln_results),
                "total_patch_patterns": len(patch_results),
                "hybrid_weights": "0.4×structural + 0.3×tfidf + 0.3×embedding",
                "retrieval_method": "double_indexation_no_classification"
            },
            evidence_for=evidence["evidence_for"],
            evidence_against=evidence["evidence_against"]
        )
        
        self.logger.info(f"Hybrid retrieval completed in {search_time:.3f}s: NEUTRAL")
        return retrieval_results

# Utility functions

def create_vulnrag_retriever() -> VulnragRetriever:
    """Factory function to create VulnragRetriever"""
    return VulnragRetriever()

def quick_vulnrag_analysis(code: str, top_k: int = 5) -> VulnragRetrievalResults:
    """
    Quick end-to-end Vulnrag analysis
    
    Args:
        code: Source code to analyze
        top_k: Number of similar patterns to retrieve
        
    Returns:
        VulnragRetrievalResults with classification
    """
    retriever = create_vulnrag_retriever()
    return retriever.hybrid_retrieval(code, top_k=top_k)