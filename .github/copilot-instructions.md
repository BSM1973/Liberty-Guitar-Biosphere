# Instructions obligatoires pour les plugins Liberty

Toujours appliquer [PLUGIN_UI_RULES.md](../PLUGIN_UI_RULES.md).

**Deux contraintes non négociables pour chaque modification UI :**
1. ZÉRO CHEVAUCHEMENT : composants, labels, nœuds, menus, overlays et
   zones de clic ne doivent jamais se recouvrir. Tester au moins la
   taille cible d'un écran 13 pouces, dans les deux langues.
2. ZÉRO SYMBOLE CORROMPU : conserver les accents français en UTF-8
   explicite (JUCE String::fromUTF8), sans losanges, mojibake ni caractères
   décoratifs à risque. Utiliser des icônes vectorielles JUCE validées.

Garder l'interface bleu nuit et doré approuvée et une fenêtre sans ascenseur.
Ajouter des tests géométriques et UTF-8 et corriger tout build rouge avant
de proposer des artefacts. Les builds verts ne remplacent pas la revue
visuelle dans un DAW. Ne jamais masquer les problèmes par un clip/scroll.
