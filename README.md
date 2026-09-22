# Simulateur de Réseau Local (SAE 2.3 - Réseaux)

Ce projet est une simulation en langage C d'un réseau local informatique, développé dans le cadre de la **SAE 2.3** du département Informatique de l'IUT. Il permet de modéliser des équipements réseaux (stations et switchs), d'échanger des trames Ethernet et d'implémenter l'algorithme d'arbre couvrant (Spanning Tree Protocol - STP) pour éviter les boucles réseau.

## Objectifs du Projet

- Manipuler une structure de données sous forme de graphe étiqueté.
- Modéliser des équipements réseaux (stations, commutateurs/switchs).
- Simuler la commutation de trames Ethernet au sein du réseau.
- Implémenter le protocole STP (Spanning Tree Protocol) par l'échange de trames BPDU.

## Étapes de Réalisation

Le projet a été développé en suivant 4 grandes étapes :

### Étape 1 : Conception et définition des structures de données
Mise en place des structures en C pour représenter le réseau :
- **Stations :** Définies par leur adresse MAC et adresse IPv4.
- **Switchs :** Définis par leur adresse MAC, nombre de ports, priorité STP, et table de commutation.
- **Réseau :** Modélisé comme un graphe interconnectant les équipements (stations et switchs).

### Étape 2 : Fichier de configuration d'architecture
Les topologies de réseau sont stockées sous forme de fichiers texte (dossier `config/`). Le fichier déclare d'abord les switchs, puis les stations, et enfin les liaisons (câbles) entre ces équipements en précisant le coût (poids) de la liaison en fonction du débit simulé.

### Étape 3 : Commutation de trames Ethernet
Création d'une structure modélisant fidèlement une trame Ethernet (Préambule, SFD, adresses MAC source et destination, type, données et FCS). Le simulateur permet d'injecter des trames dans le réseau et de simuler leur parcours de switch en switch via les tables de commutation.

### Étape 4 : Spanning Tree Protocol (STP)
Pour les architectures complexes contenant des cycles, le protocole STP a été implémenté. Au lancement, les switchs ont des tables vides et échangent des messages **BPDU**. L'algorithme fait converger le réseau en élisant un "pont racine" (Root Bridge) et en bloquant automatiquement certains ports pour empêcher les boucles infinies de diffusion.

## Structure de l'Arborescence

- `app/src/` : Fichiers sources en C (`main_stp.c`, `main_sans_stp.c`, `reseau.c`, `graphe.c`).
- `app/Makefile` : Script de compilation du projet.
- `config/` : Exemples de topologies de réseaux (fichiers `configX.txt`).
- `sujet.pdf` : Le sujet détaillé du projet.

## Compilation et Exécution

Le projet utilise un système d'ordonnanceur d'événements discrets et propose deux programmes distincts : l'un avec le protocole STP activé, l'autre sans.

### 1. Compiler le projet
Dans le dossier `app/`, exécutez la commande suivante :
```bash
make
```
Cela va générer les deux exécutables dans le dossier `app/bin/`.

### 2. Exécuter le simulateur avec STP (Étape 4)
```bash
./bin/simulateur_stp <chemin_vers_fichier_config>
```

### 3. Exécuter le simulateur sans STP (Étapes 1 à 3)
```bash
./bin/simulateur_sans_stp <chemin_vers_fichier_config>
```

### 4. Nettoyer les fichiers de compilation
```bash
make clean
```
