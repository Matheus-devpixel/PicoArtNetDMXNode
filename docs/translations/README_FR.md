<div align="center">

# Pico ArtNet DMX Node (Raspberry Pi Pico RP2040)

**Firmware natif bare-metal haute performance pour le contrôle professionnel de l'éclairage scénique DMX512 via USB Ethernet (RNDIS / CDC-ECM)**

[![Português](https://img.shields.io/badge/Idioma-Portugu%C3%AAs-blue?style=for-the-badge)](../../README.md)
[![English](https://img.shields.io/badge/Language-English-green?style=for-the-badge)](README_EN.md)
[![Español](https://img.shields.io/badge/Idioma-Espa%C3%B1ol-yellow?style=for-the-badge)](README_ES.md)
[![Français](https://img.shields.io/badge/Langue-Fran%C3%A7ais-red?style=for-the-badge)](#)
[![Deutsch](https://img.shields.io/badge/Sprache-Deutsch-orange?style=for-the-badge)](README_DE.md)
[![Italiano](https://img.shields.io/badge/Lingua-Italiano-teal?style=for-the-badge)](README_IT.md)
[![Русский](https://img.shields.io/badge/%D0%AF%D0%B7%D1%8B%D0%BA-%D0%A0%D1%83%D1%81%D1%81%D0%BA%D0%B8%D0%B9-purple?style=for-the-badge)](README_RU.md)
[![中文](https://img.shields.io/badge/%E8%AF%AD%E8%A8%80-%E4%B8%AD%E6%96%87-lightgrey?style=for-the-badge)](README_ZH.md)

</div>

---

## Vue d'Ensemble du Projet

Firmware natif ultra-performant pour microcontrôleurs **Raspberry Pi RP2040 (Raspberry Pi Pico, Pico W et Waveshare RP2040-Zero)** doté d'une architecture **100% dédiée par câble USB (Ethernet over USB: RNDIS / CDC-ECM)**.

Spécialement conçu pour la transmission professionnelle d'éclairage scénique DMX512 via Art-Net UDP (Port 6454), gérant de **1 à 8 univers DMX simultanés** (jusqu'à 4 096 canaux) en parallèle à 44 Hz constants sans gigue (jitter).

---

## Architecture Réseau USB Ethernet (Plug-and-Play)

Connecté à un ordinateur via câble USB, le RP2040 agit immédiatement comme un **adaptateur réseau virtuel Ethernet haut débit** :
- **Windows :** Pilote natif Remote NDIS (**RNDIS**) via les descripteurs Microsoft OS 1.0, reconnu instantanément sans installer de pilote externe.
- **macOS / Linux :** Pilote natif **CDC-ECM**.
- **Serveur DHCP Intégré :** Le Pico attribue automatiquement l'adresse IP à l'ordinateur hôte sur le même sous-réseau, éliminant toute configuration manuelle.

### Modes de Fonctionnement et Plages d'Adresses IP

Pour éviter les conflits d'adresses avec les réseaux Wi-Fi ou d'entreprise (`192.168.x.x`), le système utilise des sous-réseaux dédiés :

1. **Mode Configuration Initiale (Setup Wizard) :**
   - **IP :** `10.10.0.1` (l'hôte reçoit `10.10.0.2`).
   - S'active automatiquement au premier démarrage ou après une réinitialisation d'usine.
   - Donne un accès exclusif au portail web de configuration pour régler l'identifiant du nœud, le nombre de sorties actives (1 à 8), le nom descriptif, la langue, le thème et la fréquence DMX.

2. **Mode Opérationnel Multi-Nœuds (Art-Net Actif) :**
   - **IP :** `2.0.N.1` (où `N` est l'identifiant du nœud de 1 à 16).
   - Prise en charge native de jusqu'à **16 nœuds simultanés** connectés au même ordinateur ou hub USB (128 univers / 65 536 canaux DMX) :
     - **Nœud 1 :** IP `2.0.1.1` -> Univers 0 à 7
     - **Nœud 2 :** IP `2.0.2.1` -> Univers 8 à 15
     - **Nœud 3 :** IP `2.0.3.1` -> Univers 16 à 23
     - *(configurable jusqu'au Nœud 16 : IP `2.0.16.1` -> Univers 120 à 127)*

---

## Support Multilingue Intégré (8 Langues)

Le firmware inclut une localisation native dans les 8 langues suivantes :
- **Português (Brésil)** [PT]
- **English** [EN]
- **Español** [ES]
- **Français** [FR]
- **Deutsch** [DE]
- **Italiano** [IT]
- **Русский (Russe)** [RU]
- **中文 (Chinois Simplifié)** [ZH]

Le changement de langue est répercuté instantanément sur toutes les pages web et sur l'écran OLED I2C, restant enregistré de façon permanente dans la mémoire Flash SPI interne.

---

## Performances Bare-Metal C SDK

1. **Séparation Réelle Dual-Core :**
   - **CORE 0 :** Piles USB TinyUSB, TCP/IP lwIP, serveur web HTTP (Port 80), récepteur UDP Art-Net (Port 6454), affichage OLED SSD1306 et surveillance Watchdog de sécurité.
   - **CORE 1 :** Moteur DMX512 dédié en priorité maximale sans interruption, générant des trames DMX avec zéro gigue et latence nulle, totalement isolé du trafic réseau.

2. **Moteur DMX512 Matériel (8 State Machines PIO + DMA) :**
   - Exploite les deux blocs PIO (PIO0 et PIO1) du RP2040 avec 8 machines d'état fonctionnant en parallèle.
   - Génération matérielle précise des impulsions BREAK (140 µs) et MAB (16 µs) à 250 kbauds (8N2).
   - Sortie simultanée continue de jusqu'à 8 univers calés à 44 Hz.

3. **Double Búfer Ping-Pong Atomique & ArtSync :**
   - Échange atomique de pointeurs éliminant tout effet de déchirement visuel sur les projecteurs.
   - Prise en charge des paquets ArtSync (OpSync) pour une synchronisation absolue entre les univers.

---

## Brochage Matériel (Pinout)

Brochage contigu, propre et robuste :

| Fonction | Broche GPIO | Broche Physique (Pico) | Périphérique / Bus | Note |
| :--- | :--- | :--- | :--- | :--- |
| **Sortie DMX 1** | **GP0** | Broche 1 | PIO0 - SM 0 | Univers de début + 0 |
| **Sortie DMX 2** | **GP1** | Broche 2 | PIO0 - SM 1 | Univers de début + 1 |
| **Sortie DMX 3** | **GP2** | Broche 4 | PIO0 - SM 2 | Univers de début + 2 |
| **Sortie DMX 4** | **GP3** | Broche 5 | PIO0 - SM 3 | Univers de début + 3 |
| **Sortie DMX 5** | **GP4** | Broche 6 | PIO1 - SM 0 | Univers de début + 4 |
| **Sortie DMX 6** | **GP5** | Broche 7 | PIO1 - SM 1 | Univers de début + 5 |
| **Sortie DMX 7** | **GP6** | Broche 9 | PIO1 - SM 2 | Univers de début + 6 |
| **Sortie DMX 8** | **GP7** | Broche 10 | PIO1 - SM 3 | Univers de début + 7 |
| **OLED SDA** | **GP8** | Broche 11 | I2C0 SDA (400 kHz) | Écran SSD1306 128x64 |
| **OLED SCL** | **GP9** | Broche 12 | I2C0 SCL (400 kHz) | Écran SSD1306 128x64 |
| **LED DMX Ext** | **GP10** | Broche 14 | Sortie Numérique GPIO | Témoin externe d'activité DMX |
| **LED Carte** | **GP25** | LED intégrée | Sortie Numérique GPIO | Miroir de GP10 |

*Les broches DMX non configurées sont basculées en haute impédance logicielle pour une protection électrique maximale.*

---

## Câblage des Transceivers RS-485 (MAX485 / SN75176)

Pour chaque sortie DMX (GP0 à GP7) :
- **DI (Data In) :** Connecter à la broche GPIO correspondante du RP2040.
- **DE + RE (Driver/Receiver Enable) :** Relier ensemble au `3V3` (maintient le mode transmission continue).
- **VCC :** Connecter au `5V` (VBUS du câble USB).
- **GND :** Connecter au `GND` commun du circuit.
- **A (+) :** Ligne DMX positive (Broche XLR 3).
- **B (-) :** Ligne DMX négative (Broche XLR 2).
- **XLR Broche 1 :** Masse / Blindage (GND).

---

## Binaire Compilé Prêt à l'Emploi (.UF2)

Le binaire prêt à flasher est disponible dans :
- **`binarios_finais/artnet_dmx.uf2`** (ou sur la page des [Releases GitHub](https://github.com/Matheus-devpixel/PicoArtNetDMXNode/releases))

### Comment Flasher sur le Raspberry Pi RP2040 :
1. Maintenez le bouton **BOOTSEL** enfoncé sur la carte Pico.
2. Branchez le câble USB à l'ordinateur et relâchez le bouton. Un lecteur nommé `RPI-RP2` apparaîtra.
3. Glissez-déposez le fichier `artnet_dmx.uf2` sur `RPI-RP2`.
4. La carte redémarre instantanément et s'affiche comme carte réseau Ethernet USB à l'IP `10.10.0.1` (Setup) ou `2.0.N.1` (Opérationnel).

---

## Compilation depuis le Code Source

Pour recompiler le firmware à partir des sources dans `src/` :
- Exécutez : `python compile_targets.py`
- Le script compile le code avec Ninja, convertit le fichier au format officiel `.uf2` et actualise le dossier `binarios_finais/`.

---

## Auteur et Développeur

- **Développeur :** Matheus Dev
- **GitHub :** [@Matheus-devpixel](https://github.com/Matheus-devpixel)

---

## Utilitaire de Test de Transmission (ArtNet_Test.py)

Le dépôt comprend un script de test en Python de haute précision ([`ArtNet_Test.py`](../../ArtNet_Test.py)) capable d'émettre jusqu'à 4 096 canaux DMX simultanés (8 univers de 512 canaux) à 60 FPS verrouillés sans gigue :

```bash
# Simulation scénique standard (lyres, wash RGB, barres de pixels, stroboscopes)
python ArtNet_Test.py

# Définition de l'IP cible et du taux de rafraîchissement (ex: 60 FPS sur 2.0.1.1)
python ArtNet_Test.py 60 2.0.1.1

# Mode stroboscope synchronisé sur l'ensemble des 4 096 canaux
python ArtNet_Test.py --ip=2.0.1.1 --mode=strobe

# Mode dégradé continu de roue des couleurs RGB
python ArtNet_Test.py --ip=2.0.1.1 --mode=rgb
```

---

## Crédits et Remerciements

Ce projet s'appuie sur des contributions open source remarquables :

1. **Renzo Mischianti ([www.mischiant.org](https://www.mischiant.org)) :**
   - Schémas graphiques haute résolution des brochages du Raspberry Pi Pico et RP2040-Zero.
2. **Sergey Fetisov ([fetisov/lrndis](https://github.com/fetisov/lrndis)) :**
   - Implémentation légère de Microsoft RNDIS et serveurs DHCP/DNS pour lwIP (Licence MIT).
3. **Colin O'Flynn (NewAE Technology / Bibliothèque LUFA) :**
   - Définitions protocolaires et fichiers d'en-tête de Microsoft RNDIS / NDIS (Licence BSD-3-Clause).
4. **Ha Thach ([tinyusb.org](https://github.com/hathach/tinyusb)) :**
   - Pile de périphériques TinyUSB Device Stack (Licence MIT).
5. **Raspberry Pi Ltd. :**
   - SDK C/C++ Raspberry Pi Pico, architecture PIO et compilateur pioasm (Licence BSD-3-Clause).
6. **Adam Dunkels / Swedish Institute of Computer Science (SICS) :**
   - Pile TCP/IP lwIP pour systèmes embarqués (Licence BSD).

---

## Licence

Distribué sous la **Licence MIT**, autorisant l'utilisation libre personnelle, éducative et commerciale dans le respect des mentions de droits d'auteur d'origine.
