
## README pour Drawbot

```markdown
# Drawbot — Robot dessinateur ESP32

Projet académique réalisé dans le cadre du module de systèmes bouclés à l’ECE Paris.

L’objectif du projet est de développer un robot dessinateur commandé à distance, capable de tracer automatiquement des formes sur une feuille à partir de commandes envoyées depuis une interface web.

Le robot repose sur une plateforme Gyrobot équipée d’un ESP32, de moteurs, d’encodeurs, d’une IMU et d’un magnétomètre.

## Objectifs du projet

- Commander un robot à distance depuis un ordinateur
- Utiliser un ESP32 comme point d’accès Wi-Fi
- Piloter des moteurs à courant continu
- Lire les informations des encodeurs
- Exploiter une IMU et un magnétomètre
- Mettre en place une correction PID
- Réaliser des séquences de dessin automatisées
- Tester et calibrer les déplacements du robot

## Fonctionnalités principales

- Interface web embarquée sur l’ESP32
- Commande du robot via Wi-Fi
- Pilotage des moteurs gauche et droit
- Lecture des encodeurs moteurs
- Estimation des distances parcourues
- Correction de trajectoire avec PID
- Page de calibration avec affichage des valeurs utiles
- Séquences de dessin :
  - escalier ;
  - cercle à rayon paramétrable ;
  - flèche orientée vers le Nord / rose des vents.

## Architecture générale

Le projet repose sur une architecture simple :

1. L’utilisateur se connecte au réseau Wi-Fi créé par l’ESP32.
2. Il accède à une interface web depuis un navigateur.
3. Il choisit une commande ou une séquence de dessin.
4. L’ESP32 interprète la commande.
5. Le robot pilote ses moteurs, lit ses capteurs et exécute la trajectoire demandée.

## Matériel utilisé

- NodeMCU ESP32
- Plateforme Gyrobot
- Deux motoréducteurs avec encodeurs
- Drivers moteurs DRV8837
- IMU LSM6DS3
- Magnétomètre LIS3MDL
- Alimentation par piles
- Support de feutre

## Technologies utilisées

- C / C++ embarqué
- ESP32
- Wi-Fi
- HTML pour l’interface web
- PWM
- Encodeurs
- I2C
- PID
- Odométrie
- Capteurs inertiels
- Magnétomètre

## Séquences développées

### Escalier

Le robot trace une séquence composée de plusieurs segments et rotations : avancer, tourner, avancer, tourner, puis avancer à nouveau.

Cette séquence permet de valider la précision des distances et des angles.

### Cercle

Le robot trace un cercle dont le rayon est paramétrable depuis l’interface web.

Cette séquence nécessite une coordination différente entre les deux roues, car elles ne parcourent pas la même distance pendant le mouvement circulaire.

### Flèche / rose des vents

Le robot utilise le magnétomètre pour estimer une direction et orienter le dessin vers le Nord.

Cette partie met en évidence les limites du magnétomètre, notamment sa sensibilité aux perturbations magnétiques.

## Tests et validation

Plusieurs tests ont été réalisés :

- test de communication Wi-Fi ;
- test de l’interface web ;
- calibration des moteurs ;
- calibration des encodeurs ;
- tests de déplacement en ligne droite ;
- tests de rotation ;
- réglage du PID ;
- validation des séquences de dessin.

Les tests ont permis de valider les principales fonctions du robot, tout en mettant en évidence certaines limites liées aux frottements, à l’état des piles, à la pression du feutre et aux perturbations du magnétomètre.

## Lancement du projet

Le projet peut être ouvert avec VS Code et PlatformIO.

Étapes générales :

1. Ouvrir le projet dans VS Code
2. Installer l’extension PlatformIO si nécessaire
3. Brancher l’ESP32 en USB
4. Compiler et téléverser le programme sur la carte
5. Ouvrir le moniteur série pour récupérer l’adresse IP
6. Se connecter au Wi-Fi du Drawbot
7. Accéder à l’interface web depuis un navigateur

## Compétences développées

Ce projet m’a permis de progresser en programmation embarquée, en pilotage moteur, en utilisation de capteurs, en correction PID et en validation expérimentale.

Il m’a également permis de travailler sur une chaîne complète : commande utilisateur, communication sans fil, traitement embarqué, actionneurs, capteurs, correction et tests réels.

## Auteur

Hugo Poilly  
Étudiant ingénieur à l’ECE Paris  

Portfolio : https://portfolio-hugo-poilly.vercel.app  
LinkedIn : https://www.linkedin.com/in/hugo-poilly-852421335/
