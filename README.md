# Note pour run le code
Il faut modifier le lien du CMakelist vers la vidéo que l'on veut produire dans le dossier vidéo.
La stucture des fichier dans le dossier vidéo ne sont que des utilisations des fonctions définie dans le src.
Le dossier src contient 3 fichier de fonctions
  - Le fichier visual qui définit toutes les fonctions d'affichage, elle utilise raylib
  - Le fichier physics qui définie toutes les règles de la physique
  - Le fichier text2latex qui permet d'écrire du latex en remplacant "\" par "/" et l'affiche sur la fenetre
Il faut dans le dossier vidéo utiliser le dossier de la vidéo qu'on veut produire. 

Certaines modification ont été apportée dans raudio.c et raylib.h. Les modifications apportées sont aux niveaux du recorder afin de pouvoir enregistrée du son en même temps que l'enregistrement de la vidéo.
  
## TODOs 
### Text2Latex
  - Implementer une fonction /centering directement dans le fichier txt afin de pouvoir centrer un paragraphe ou certaine phrases. Implementer drawrenderboxescentered comme principal et l'autre comme cas particulier. Potentiellement ajouté une structure qui définit l'état.
  - Deux bugs sont à corriger.
    - Le passage à la ligne via // n'est pas de la meme hauteur que le passage à la ligne losque le texte dépasse la largeur autorisée
    - Les n-uple fraction | n > 2 n'ont pas un affichage incroyable
  - L'interligne lorsqu'il y'a une fraction grande est probablement mal géré
  - Il faut ajouter les begin(equation) avec un centering et potentiellement un align qu'on pourrait mettre en /align. A voir

### text2video
Etablir un txt qui transformera le tout en vidéo celui ci devrait faire lien entre numero de frame et temps irl. Ensuite le fichier text doit pouvoir etre lu par une fonction qui transforme le fichier txt en vidéo mp4.

### Solar System
  - Il faut ajouter le fond stéllaire.
  - Definir les mouvement de la camera pour bien afficheret  visionner les mouvement des planètes. Pseudo 2D ==> épicicloides
  - Permettre d'accellerer le temps sans diminiuer la qualité de calcul ==> faire les calculs par step et afficher tous les x steps
  - Ajouter une ligne qui trace l'orbite.


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
