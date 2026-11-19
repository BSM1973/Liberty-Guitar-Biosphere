# LIBERTY MUSIC SOFTWARE — règles UI obligatoires

Ces règles s'appliquent à **tous les instruments virtuels, effets audio et DAW Liberty Music Software**, sans avoir à les rappeler à chaque itération.

## 1. ZÉRO CHEVAUCHEMENT

- Aucune collision entre textes, icônes, potentiomètres, boutons, menus, analyseurs, sélecteurs, cadres ou zones cliquables.
- Texte jamais tronqué ni masqué par un autre composant : mesurer les zones réelles et utiliser `drawFittedText`, des zones élargies ou déplacer/réduire le texte.
- Respecter en permanence la taille de fenêtre prévue, y compris sur un écran MacBook 13 pouces. Aucune barre de défilement ajoutée pour masquer le problème.
- Vérifier **tous les panneaux**, menus ouverts, sélection de preset, langue FR et EN, états actif/inactif, survol et focus, ainsi que les différents formats AU/VST3.
- Les zones cliquables doivent correspondre aux contrôles visibles, ne pas masquer les autres éléments et être exclusives.
- Garder un espacement positif entre zones. En cas de conflit : réduire légèrement la police, déplacer le label ou redistribuer l'espace, **sans sacrifier la lisibilité**.

## 2. ZÉRO CARACTÈRE CORROMPU

- Français par défaut, avec accents impeccables : é, è, ê, à, ç, ù, etc.
- Les chaînes UTF-8 issues de littéraux `const char*` doivent être transmises par `juce::String::fromUTF8` (et non par conversion implicite) ; compilation Windows en `/utf-8`.
- Vérifier toutes les pages, tooltips, menus déroulants, popups, noms de presets, automatismes et messages d'erreur, en français **et** en anglais.
- Aucun losange de remplacement, texte du type `Ã©`, séquence mojibake ou glyphes manquants.
- Les symboles Unicode décoratifs sont interdits dans les chaînes d'interface : préférer du texte clair ou des icônes **dessinées par JUCE** avec un rendu vérifié.

## 3. VALIDATION AVANT LIVRAISON

- Ajouter des tests de collision **partageant les mêmes coordonnées que l'interface** ; ne pas simplement vérifier des valeurs recopiées à la main.
- Ajouter un audit UTF-8 et une compilation macOS ARM64 AU/VST3 + Windows x64 VST3, avec artefacts vérifiés.
- Compléter avec une inspection graphique réelle sur les deux OS et dans les DAW, car des tests géométriques ne peuvent pas certifier à eux seuls le rendu des polices.
- Ne pas annoncer de « correction définitive » avant les tests et la validation visuelle.

## GUITAR BIOSPHERE : conformité initiale

`Source/Ui/EcosystemLayout.h` est partagé entre l'affichage, les zones de clic et
`Tests/EcosystemLayoutSmoke.cpp`. Ce test contrôle HARMONIC LIFE,
« ÉTAT VIVANT / CALME », les six icônes, les libellés, le titre,
les zones cliquables et le footer. Les autres pages demandent toujours
une inspection visuelle complète dans l'hôte.

Cette charte doit être copiée/adaptée à **chaque nouveau dépôt Liberty**
avant d'entamer le développement de son interface.
