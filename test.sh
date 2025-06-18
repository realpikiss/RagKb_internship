cd ~/Documents/RessourcesStages/Projets/VulRAG-Hybrid-System/data/tmp/cpg_json

# Statistiques complètes
echo "=== STATISTIQUES FINALES ==="
echo "Répertoires d'instances: $(find . -maxdepth 1 -type d | wc -l)"
echo "Fichiers JSON totaux: $(find . -name "*.json" | wc -l)"
echo "Fichiers vuln_cpg.json: $(find . -name "vuln_cpg.json" | wc -l)"
echo "Fichiers patch_cpg.json: $(find . -name "patch_cpg.json" | wc -l)"

# Vérifier les erreurs
if [[ -f "extract.log" ]]; then
    echo "Erreurs: $(wc -l < extract.log)"
    echo "Premières erreurs:"
    head -n 5 extract.log
else
    echo "Aucun fichier d'erreur - parfait !"
fi

# Tailles des fichiers
echo ""
echo "Taille totale: $(du -sh . | cut -f1)"
echo "Taille moyenne par fichier: $(du -sh . | cut -f1 | numfmt --from=iec) / 4410"

# Quelques exemples
echo ""
echo "Exemples de fichiers créés:"
find . -name "*.json" | head -5 | while read f; do
    echo "  $f ($(du -h "$f" | cut -f1))"
done