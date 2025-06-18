
"""
KB2 Integration Example
Usage example for production RAG system
"""
import numpy as np
import json
from pathlib import Path

class ProductionKB2Engine:
    def __init__(self, export_dir):
        # Load precomputed embeddings
        self.embeddings = np.load(export_dir / "embeddings_matrix.npy")

        with open(export_dir / "entry_keys.json") as f:
            self.entry_keys = json.load(f)

        with open(export_dir / "metadata_index.json") as f:
            self.metadata = json.load(f)

        with open(export_dir / "kb2_config.json") as f:
            self.config = json.load(f)

    def search_similar(self, query_embedding, top_k=10):
        # Normalize query
        query_norm = query_embedding / np.linalg.norm(query_embedding)

        # Compute similarities
        similarities = np.dot(self.embeddings, query_norm)

        # Get top-k
        top_indices = np.argsort(similarities)[-top_k:][::-1]

        results = []
        for idx in top_indices:
            entry_key = self.entry_keys[idx]
            similarity = float(similarities[idx])
            metadata = self.metadata[entry_key]
            results.append((entry_key, similarity, metadata))

        return results

# Usage:
# kb2_engine = ProductionKB2Engine(Path("kb2_production_export"))
# results = kb2_engine.search_similar(your_query_embedding)
