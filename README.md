# Note pour run le code
Il faut modifier le lien du CMakelist vers la vidéo que l'on veut produire dans le dossier vidéo.
La stucture des fichier dans le dossier vidéo ne sont que des utilisations des fonctions définie dans le src.
Le dossier src contient 3 fichier de fonctions
  - Le fichier visual qui définit toutes les fonctions d'affichage, elle utilise raylib
  - Le fichier physics qui définie toutes les règle de la physique implémenter
  - Le fichier text2latex qui permet d'écrire du latex en remplacant "\" par "/" et l'affiche sur la fenetre
Il faut dans le doisser vidéo utiliser le dossier de la vidéo qu'on veut produire. 

Certaines modification ont été apportée dans raudio.c et raylib.h. Les modifications apportées sont aux niveaux du recorder afin de pouvoir enregistrée du son en même temps que l'enregistrement de la vidéo.
  
## ⚡ TODOs Principaux
  - Develloper les différentes animations en fonction des choses que l'on veut montrer
  - Cleaner la partie latex ( introduction de case ?)
  - introdurie le son avec mini audio
  - Modifier l'output pour que celui-ci aille directement dans le bon dossier, implementer des racourcis clavier pour pouvoir nommer le jet ainsi que le supprimer si la sécance est mauvaise
  - Raster to Vector Graphics Converter est une librairie en rust qui permet de vectoriser une image. Ce serait bien d'implémenter son utilisation afin de pouvoir automatiser image to fourrier
  
### Faire la vidéo sur la décomposition de fourier ( short?).
- Montrer que le modèle où la terre est au centre peut fonctionner mais les trajectoires des corps celestes deviennent plus compliquées. Les anciens modélisaient ces trajectoires à l'aide de cercle concentrique tournant à des vitesse différentes.
- Pourquoi ce modèle fonctionnaient ( qui plus est à l'époque mieux que le modèle héliocentrée) ==> Decomposition de fourier
- Introduction de Fourrier. Histoire tout aussi fausse que Newton et sa pomme. Fourrier s'est brulé avec une barre en métal et s'est demandé pourquoi la chaleur passaient à travers la barre en métal. Equation de la chaleur ==> série de fourier pour la résoudre. Intéprétation d'une fonction comme une somme infinie de sinus et cosinus
- Interprétation modèles ancien et série de Fourier
#### Animation à implémenter
- Introduire les déphasage entre les aphélies
- Introduire une simulation qui retrace les (un) cercle avec des epi cicloyde pour questionner sur le pourquoi ca fonctionne ?
- Réfléchir à la partie pour introduire fourier. ( Equation de la chaleur, voir comment implementer un bonhome avec une barre ?)
- Introduire les caractères  de l'equation de la chaleur et des transformées de fourier dans text2latex 
  
### Vidéo sur les chaines de Markov

### Vidéo sur l'islande
