"""
KB2 Hybrid Feature Extractor

Extracts structural (60%) and semantic (40%) features from Joern CPG data
for the KB2 hybrid knowledge base construction.

Features extracted:
- Structural Features (60%): Graph metrics, complexity, control flow patterns
- Semantic Features (40%): Function calls, API patterns, call diversity
"""

import json
import math
from pathlib import Path
from collections import Counter, defaultdict
from typing import Dict, List, Tuple, Optional, Any, Set
try:
    from .graphson_parser import GraphSONParser
    from .cwe_patterns import CWEPatternExtractor
    from .feature_utils import FeatureUtils
    from .logging_config import LoggingMixin, get_module_logger
except ImportError:
    # For direct execution
    from graphson_parser import GraphSONParser
    from cwe_patterns import CWEPatternExtractor
    from feature_utils import FeatureUtils
    from logging_config import LoggingMixin, get_module_logger


class StructuralFeatureExtractor(LoggingMixin):
    """Extracts structural features (60% of KB2 retrieval weight)"""
    
    def __init__(self, parser: GraphSONParser):
        super().__init__()
        self.parser = parser
        self.vertices = parser.vertices
        self.edges = parser.edges
        
    def extract_basic_graph_metrics(self) -> Dict[str, float]:
        """Extract basic graph topology metrics"""
        vertex_count = len(self.vertices)
        edge_count = len(self.edges)
        
        return {
            'total_nodes': vertex_count,
            'total_edges': edge_count,
            'graph_density': edge_count / vertex_count if vertex_count > 0 else 0.0,
            'avg_degree': (2 * edge_count) / vertex_count if vertex_count > 0 else 0.0
        }
    
    def extract_complexity_metrics(self) -> Dict[str, int]:
        """Extract code complexity metrics"""
        vertex_types = self.parser.get_vertex_types()
        edge_types = self.parser.get_edge_types()
        
        # Cyclomatic complexity approximation
        # McCabe: M = E - N + 2P (E=edges, N=nodes, P=connected components)
        # Simplified: decision points + 1
        control_structures = vertex_types.get('CONTROL_STRUCTURE', 0)
        blocks = vertex_types.get('BLOCK', 0)
        methods = vertex_types.get('METHOD', 0)
        
        # Control flow complexity
        cfg_edges = edge_types.get('CFG', 0)
        dominate_edges = edge_types.get('DOMINATE', 0)
        
        # Approximated metrics based on CPG structure
        cyclomatic_complexity = max(1, cfg_edges - vertex_types.get('BLOCK', 0) + methods)
        nesting_depth = min(10, blocks // max(1, methods))  # Estimated nesting
        essential_complexity = control_structures + (cfg_edges // 10)  # Simplified
        
        return {
            'cyclomatic_complexity': cyclomatic_complexity,
            'nesting_depth': nesting_depth,
            'essential_complexity': essential_complexity,
            'control_structures': control_structures,
            'blocks': blocks,
            'methods': methods
        }
    
    def extract_control_flow_patterns(self) -> Dict[str, float]:
        """Extract control flow and data flow patterns"""
        vertex_types = self.parser.get_vertex_types()
        edge_types = self.parser.get_edge_types()
        
        total_vertices = len(self.vertices)
        calls = vertex_types.get('CALL', 0)
        
        # Flow metrics
        call_entropy = self._calculate_call_entropy()
        calls_per_node = calls / total_vertices if total_vertices > 0 else 0.0
        control_ratio = vertex_types.get('CONTROL_STRUCTURE', 0) / total_vertices if total_vertices > 0 else 0.0
        
        # Data flow complexity
        reaching_def = edge_types.get('REACHING_DEF', 0)
        ref_edges = edge_types.get('REF', 0)
        data_flow_ratio = (reaching_def + ref_edges) / total_vertices if total_vertices > 0 else 0.0
        
        return {
            'call_entropy': call_entropy,
            'calls_per_node': calls_per_node,
            'control_ratio': control_ratio,
            'data_flow_ratio': data_flow_ratio,
            'cfg_complexity': edge_types.get('CFG', 0) / total_vertices if total_vertices > 0 else 0.0
        }
    
    def _calculate_call_entropy(self) -> float:
        """Calculate entropy of function call distribution"""
        call_vertices = [v for v in self.vertices if v.get('label') == 'CALL']
        
        if not call_vertices:
            return 0.0
            
        # Get call names from properties
        call_names = []
        for vertex in call_vertices:
            props = self.parser.extract_vertex_properties(vertex)
            if 'NAME' in props:
                call_names.append(props['NAME'])
        
        if not call_names:
            return 0.0
            
        # Calculate entropy using FeatureUtils
        call_counts = Counter(call_names)
        entropy = FeatureUtils.calculate_call_entropy(call_counts)
        self.logger.debug(f"Call entropy calculated: {entropy:.3f}")
        return entropy
    
    def extract_structural_features(self) -> Dict[str, Any]:
        """Extract all structural features (60% weight)"""
        return {
            'graph_metrics': self.extract_basic_graph_metrics(),
            'complexity_metrics': self.extract_complexity_metrics(),
            'control_flow_patterns': self.extract_control_flow_patterns()
        }


class SemanticFeatureExtractor(LoggingMixin):
    """Extracts semantic features (40% of KB2 retrieval weight)"""
    
    def __init__(self, parser: GraphSONParser):
        super().__init__()
        self.parser = parser
        self.vertices = parser.vertices
        self.edges = parser.edges
        
        # Initialize CWE pattern extractor for kernel C/C++ code
        self.cwe_extractor = CWEPatternExtractor()
        
        # Enhanced dangerous functions for kernel C/C++ code
        self.dangerous_functions = {
            'memory': ['malloc', 'calloc', 'realloc', 'free', 'alloca', 'kmalloc', 'kfree', 'vmalloc', 'vfree'],
            'string': ['strcpy', 'strcat', 'sprintf', 'scanf', 'gets', 'strncpy', 'memcpy', 'memmove'],
            'buffer': ['memcpy', 'memmove', 'memset', 'bcopy', 'copy_from_user', 'copy_to_user'],
            'system': ['system', 'exec', 'popen', 'fork', 'do_fork', 'do_execve'],
            'file': ['fopen', 'fread', 'fwrite', 'fgets', 'fputs', 'filp_open', 'vfs_read', 'vfs_write'],
            'kernel_specific': ['kmalloc', 'kfree', 'devm_kmalloc', 'devm_kfree', 'printk', 'dev_info']
        }
    
    def extract_function_calls(self) -> Dict[str, Any]:
        """Extract function call patterns and frequency"""
        call_vertices = [v for v in self.vertices if v.get('label') == 'CALL']
        
        all_calls = []
        dangerous_calls = []
        
        for vertex in call_vertices:
            props = self.parser.extract_vertex_properties(vertex)
            if 'NAME' in props:
                call_name = str(props['NAME']).lower()
                all_calls.append(props['NAME'])  # Keep original case
                
                # Check for dangerous patterns
                for category, funcs in self.dangerous_functions.items():
                    for dangerous_func in funcs:
                        if dangerous_func in call_name:
                            dangerous_calls.append({
                                'name': props['NAME'],
                                'category': category,
                                'pattern': dangerous_func
                            })
                            break
        
        # Top function calls
        call_counter = Counter(all_calls)
        top_calls = dict(call_counter.most_common(20))
        top_calls_vector = list(call_counter.most_common(10))  # Top 10 for vector
        
        return {
            'all_function_calls': all_calls,
            'top_calls': top_calls,
            'top_calls_vector': [count for _, count in top_calls_vector],
            'dangerous_calls': dangerous_calls,
            'total_call_count': len(all_calls),
            'unique_call_count': len(set(all_calls))
        }
    
    def extract_api_patterns(self) -> Dict[str, Any]:
        """Extract API usage patterns and identifiers"""
        identifier_vertices = [v for v in self.vertices if v.get('label') == 'IDENTIFIER']
        field_vertices = [v for v in self.vertices if v.get('label') == 'FIELD_IDENTIFIER']
        
        identifiers = []
        field_identifiers = []
        
        # Extract identifier names
        for vertex in identifier_vertices:
            props = self.parser.extract_vertex_properties(vertex)
            if 'NAME' in props:
                identifiers.append(props['NAME'])
        
        for vertex in field_vertices:
            props = self.parser.extract_vertex_properties(vertex)
            if 'NAME' in props:
                field_identifiers.append(props['NAME'])
        
        # Common API patterns
        api_patterns = {
            'error_handling': len([id for id in identifiers if any(pattern in str(id).lower() 
                                 for pattern in ['error', 'err', 'fail', 'exception'])]),
            'memory_management': len([id for id in identifiers if any(pattern in str(id).lower() 
                                    for pattern in ['ptr', 'alloc', 'free', 'mem', 'buffer'])]),
            'security_checks': len([id for id in identifiers if any(pattern in str(id).lower() 
                                  for pattern in ['check', 'valid', 'auth', 'perm', 'secure'])])
        }
        
        return {
            'identifiers': Counter(identifiers).most_common(15),
            'field_identifiers': Counter(field_identifiers).most_common(10),
            'api_patterns': api_patterns,
            'total_identifiers': len(identifiers),
            'unique_identifiers': len(set(identifiers))
        }
    
    def extract_call_diversity(self) -> Dict[str, float]:
        """Extract call diversity and complexity metrics"""
        function_calls = self.extract_function_calls()
        
        total_calls = function_calls['total_call_count']
        unique_calls = function_calls['unique_call_count']
        
        # Diversity metrics using FeatureUtils
        call_diversity_ratio = FeatureUtils.calculate_diversity_ratio(unique_calls, total_calls)
        dangerous_call_ratio = FeatureUtils.calculate_ratio(len(function_calls['dangerous_calls']), total_calls)
        
        # Semantic complexity based on call patterns
        top_calls_vector = function_calls['top_calls_vector']
        call_distribution_entropy = FeatureUtils.calculate_entropy([count/total_calls for count in top_calls_vector if count > 0]) if total_calls > 0 else 0.0
        
        return {
            'call_diversity_ratio': call_diversity_ratio,
            'dangerous_call_ratio': dangerous_call_ratio,
            'call_distribution_entropy': call_distribution_entropy,
            'semantic_complexity_score': call_diversity_ratio * call_distribution_entropy
        }
    
    def extract_combined_text(self) -> str:
        """Extract combined semantic text for TF-IDF similarity with CWE patterns"""
        function_calls = self.extract_function_calls()
        api_patterns = self.extract_api_patterns()
        
        # Combine function calls and identifiers into semantic text
        top_calls = [call for call, _ in function_calls['top_calls'].items()][:10]
        top_identifiers = [id for id, _ in api_patterns['identifiers']][:10]
        
        # Add dangerous calls with higher weight
        dangerous_calls = function_calls['dangerous_calls']
        dangerous_call_names = [call['name'] for call in dangerous_calls]
        
        # Add CWE patterns if available
        cwe_elements = []
        if hasattr(self.parser, 'source_code') and self.parser.source_code:
            cwe_patterns = self.extract_cwe_patterns(self.parser.source_code)
            for cwe_type, patterns in cwe_patterns.items():
                if patterns.get('risk_score', 0) > 0.5:  # High risk patterns
                    cwe_elements.append(f"{cwe_type}_pattern")
        
        # Weighted combination: dangerous calls get higher weight
        combined_elements = []
        combined_elements.extend(dangerous_call_names * 3)  # 3x weight for dangerous calls
        combined_elements.extend(top_calls)
        combined_elements.extend(top_identifiers)
        combined_elements.extend(cwe_elements)
        
        combined_text = ' '.join(str(elem) for elem in combined_elements)
        
        return combined_text
    
    def extract_cwe_patterns(self, code: str) -> Dict[str, Any]:
        """Extract CWE-specific patterns from kernel code"""
        return self.cwe_extractor.extract_cwe_patterns(code)
    
    def extract_semantic_features(self) -> Dict[str, Any]:
        """Extract all semantic features (40% weight)"""
        function_calls = self.extract_function_calls()
        api_patterns = self.extract_api_patterns()
        call_diversity = self.extract_call_diversity()
        combined_text = self.extract_combined_text()
        
        # Extract CWE patterns if code is available
        cwe_patterns = {}
        if hasattr(self.parser, 'source_code') and self.parser.source_code:
            cwe_patterns = self.extract_cwe_patterns(self.parser.source_code)
        
        return {
            'function_calls': function_calls,
            'api_patterns': api_patterns,
            'call_diversity': call_diversity,
            'combined_text': combined_text,
            'cwe_patterns': cwe_patterns
        }


class HybridFeatureExtractor(LoggingMixin):
    """Main class for extracting KB2 hybrid features (60% structural + 40% semantic)"""
    
    def __init__(self, cpg_file: Path):
        super().__init__()
        self.cpg_file = Path(cpg_file)
        self.parser = GraphSONParser(cpg_file)
        self.structural_extractor = None
        self.semantic_extractor = None
        
    def extract_features(self) -> Dict[str, Any]:
        """Extract complete KB2 hybrid features"""
        # Parse CPG file
        if not self.parser.parse():
            return {'error': f'Failed to parse CPG file: {self.cpg_file}'}
        
        # Initialize extractors
        self.structural_extractor = StructuralFeatureExtractor(self.parser)
        self.semantic_extractor = SemanticFeatureExtractor(self.parser)
        
        # Extract features
        structural_features = self.structural_extractor.extract_structural_features()
        semantic_features = self.semantic_extractor.extract_semantic_features()
        basic_stats = self.parser.get_basic_stats()
        
        # Log extraction results
        self.log_feature_extraction({
            'structural_features': structural_features,
            'semantic_features': semantic_features
        }, str(self.cpg_file))
        
        # Combine into KB2 format
        kb2_features = {
            'file_info': {
                'source_file': self.cpg_file.name,
                'file_path': str(self.cpg_file),
                'file_size_kb': basic_stats['file_size_kb'],
                'extraction_timestamp': None  # Will be set by builder
            },
            
            # Structural Features (60% weight)
            'structural_features': {
                'total_nodes': structural_features['graph_metrics']['total_nodes'],
                'total_edges': structural_features['graph_metrics']['total_edges'],
                'graph_density': structural_features['graph_metrics']['graph_density'],
                'cyclomatic_complexity': structural_features['complexity_metrics']['cyclomatic_complexity'],
                'nesting_depth': structural_features['complexity_metrics']['nesting_depth'],
                'essential_complexity': structural_features['complexity_metrics']['essential_complexity'],
                'control_structures': structural_features['complexity_metrics']['control_structures'],
                'blocks': structural_features['complexity_metrics']['blocks'],
                'methods': structural_features['complexity_metrics']['methods'],
                'call_entropy': structural_features['control_flow_patterns']['call_entropy'],
                'calls_per_node': structural_features['control_flow_patterns']['calls_per_node'],
                'control_ratio': structural_features['control_flow_patterns']['control_ratio']
            },
            
            # Semantic Features (40% weight)
            'semantic_features': {
                'function_calls': [call['name'] if isinstance(call, dict) else call 
                                 for call in semantic_features['function_calls']['all_function_calls'][:20]],
                'top_calls_vector': semantic_features['function_calls']['top_calls_vector'],
                'combined_text': semantic_features['combined_text'],
                'call_diversity_ratio': semantic_features['call_diversity']['call_diversity_ratio'],
                'dangerous_calls': semantic_features['function_calls']['dangerous_calls'],
                'api_patterns': semantic_features['api_patterns']['api_patterns'],
                'cwe_patterns': semantic_features.get('cwe_patterns', {})
            },
            
            # Additional metadata for analysis
            'extraction_metadata': {
                'parser_validation': self.parser.validate_cpg_types(),
                'feature_completeness': self._assess_feature_completeness(structural_features, semantic_features)
            }
        }
        
        return kb2_features
    
    def _assess_feature_completeness(self, structural: Dict, semantic: Dict) -> Dict[str, Any]:
        """Assess completeness and quality of extracted features"""
        completeness = {
            'structural_complete': all([
                structural['graph_metrics']['total_nodes'] > 0,
                structural['graph_metrics']['total_edges'] > 0,
                structural['complexity_metrics']['methods'] > 0
            ]),
            'semantic_complete': all([
                len(semantic['function_calls']['all_function_calls']) > 0,
                len(semantic['combined_text']) > 0
            ]),
            'quality_score': 0.0
        }
        
        # Calculate quality score
        score = 0.0
        if completeness['structural_complete']:
            score += 0.6
        if completeness['semantic_complete']:
            score += 0.4
            
        completeness['quality_score'] = score
        return completeness


def extract_kb2_features(cpg_file: Path) -> Dict[str, Any]:
    """Convenience function to extract KB2 features from a CPG file"""
    extractor = HybridFeatureExtractor(cpg_file)
    return extractor.extract_features()


if __name__ == "__main__":
    # Test the feature extractor
    import json
    from pathlib import Path
    
    PROJECT_ROOT = Path(__file__).parent.parent.parent
    CPG_DIR = PROJECT_ROOT / 'data' / 'tmp' / 'cpg_json'
    
    # Find a sample file
    sample_files = list(CPG_DIR.rglob("*vuln_cpg.json"))
    
    if sample_files:
        print("🔧 Testing KB2 Feature Extractor...")
        features = extract_kb2_features(sample_files[0])
        
        print(f"\n📊 Extracted Features Summary:")
        print(f"  • Structural features: {len(features.get('structural_features', {}))}")
        print(f"  • Semantic features: {len(features.get('semantic_features', {}))}")
        print(f"  • Quality score: {features.get('extraction_metadata', {}).get('feature_completeness', {}).get('quality_score', 0):.2f}")
        
        # Show sample structural features
        structural = features.get('structural_features', {})
        print(f"\n🏗️ Structural Features Sample:")
        print(f"  • Graph density: {structural.get('graph_density', 0):.2f}")
        print(f"  • Cyclomatic complexity: {structural.get('cyclomatic_complexity', 0)}")
        print(f"  • Call entropy: {structural.get('call_entropy', 0):.2f}")
        
        # Show sample semantic features
        semantic = features.get('semantic_features', {})
        print(f"\n🧠 Semantic Features Sample:")
        print(f"  • Function calls: {len(semantic.get('function_calls', []))}")
        print(f"  • Call diversity: {semantic.get('call_diversity_ratio', 0):.2f}")
        print(f"  • Combined text: {semantic.get('combined_text', '')[:50]}...")
        
    else:
        print("❌ No CPG files found for testing")