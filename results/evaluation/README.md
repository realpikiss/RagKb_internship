# Vulnrag Evaluation Results

Ce dossier contient les résultats des évaluations de Vulnrag contre les baselines.

## Structure des Résultats

### Fichiers Générés

- `evaluation_results_YYYYMMDD_HHMMSS.json` : Résultats complets d'évaluation
- `evaluation.log` : Logs détaillés de l'évaluation
- `comparison_report_*.json` : Rapports de comparaison spécifiques

### Format des Résultats

Chaque fichier de résultats contient :

```json
{
  "evaluation_metadata": {
    "timestamp": "2024-XX-XX HH:MM:SS",
    "total_samples": 586,
    "total_evaluation_time": 1234.56,
    "vulnrag_kb2_entries": 2317
  },
  "sample_results": [
    {
      "sample_id": "CVE-2022-12345",
      "ground_truth_vulnerable": true,
      "vulnrag_vulnerable": true,
      "vulnrag_confidence": 0.85,
      "vulnrag_time": 2.34,
      "vanilla_result": {...},
      "static_result": {...}
    }
  ],
  "metrics": {
    "vulnrag": {
      "accuracy": 0.85,
      "precision": 0.82,
      "recall": 0.87,
      "f1_score": 0.84
    },
    "vanilla_llm": {...},
    "static_analysis": {...}
  },
  "comparison": {
    "ranking_by_accuracy": [...],
    "vulnrag_advantages": {...}
  }
}
```

## Métriques Évaluées

### Métriques de Classification
- **Accuracy** : Pourcentage de prédictions correctes
- **Precision** : TP / (TP + FP)
- **Recall** : TP / (TP + FN) 
- **F1-Score** : Moyenne harmonique de Precision et Recall

### Métriques CWE
- **CWE Accuracy** : Pourcentage de CWE correctement prédits

### Métriques de Performance
- **Temps moyen** : Temps moyen par analyse
- **Temps total** : Temps total d'évaluation

### Métriques de Confiance
- **Confiance moyenne** : Score de confiance moyen
- **Corrélation confiance** : Corrélation entre confiance et précision

## Baselines Évaluées

1. **Vulnrag** : Système complet avec retrieval hybride KB2
2. **LLM Vanille** : GPT-4 sans contexte
3. **Static Analysis** : Outils statiques (cppcheck, clang-tidy, flawfinder)
4. **Static + LLM** : LLM avec contexte d'analyse statique

## Usage

Voir `../../evaluate.sh` pour lancer les évaluations.