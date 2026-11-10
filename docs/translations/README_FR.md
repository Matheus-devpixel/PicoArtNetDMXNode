<div align="center">

# Pico ArtNet DMX Node (Raspberry Pi Pico RP2040)

**Firmware natif bare-metal haute performance pour le contrôle professionnel de l'éclairage DMX512 via USB Ethernet (RNDIS / CDC-ECM)**

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

Firmware natif ultra-performant pour les microcontrôleurs **Raspberry Pi RP2040 (Raspberry Pi Pico, Pico W et Waveshare RP2040-Zero)** doté d'une architecture **100% dédiée par câble USB (Ethernet over USB: RNDIS / CDC-ECM)**.

Spécialement conçu pour la transmission professionnelle d'éclairage scénique DMX512 via Art-Net UDP (Port 6454), gérant de **1 à 8 univers DMX simultanés** (jusqu'à 4 096 canaux) en parallèle à 44 Hz constants sans gigue (jitter).

---

## Architecture Réseau USB Ethernet (Plug-and-Play)

Connecté à un ordinateur via USB, le RP2040 agit immédiatement comme un **adaptateur réseau virtuel Ethernet haut débit** :
- **Windows :** Pilote natif Remote NDIS (**RNDIS**) via les descripteurs Microsoft OS 1.0, reconnu instantanément sans installer de pilote supplémentaire.
- **macOS / Linux :** Pilote natif **CDC-ECM**.
- **Serveur DHCP Intégré :** Le Pico attribue automatiquement l'adresse IP à l'ordinateur hôte sur le même sous-réseau.

### Modes de Fonctionnement et Plages d'Adresses IP

1. **Mode Configuration Initiale (Setup Wizard) :**
   - **IP :** `10.10.0.1` (l'hôte reçoit `10.10.0.2`).
   - S'active automatiquement au premier démarrage ou après une réinitialisation d'usine.
   - Donne un accès exclusif au portail web de configuration pour régler l'identifiant du nœud, le nombre de sorties actives (1 à 8), le nom, la langue, le thème et la fréquence DMX.

2. **Mode Opérationnel Multi-Nœuds (Art-Net Actif) :**
   - **IP :** `2.0.N.1` (où `N` est l'identifiant du nœud de 1 à 16).
   - Prise en charge native de jusqu'à **16 nœuds simultanés** sur la même machine (128 univers / 65 536 canaux DMX).

---

## Support Multilingue Intégré (8 Langues)

Localisation native en : Portugais [PT], Anglais [EN], Espagnol [ES], Français [FR], Allemand [DE], Italien [IT], Russe [RU] et Chinois [ZH]. Le changement de langue est répercuté instantanément sur le serveur web et l'écran OLED SSD1306.

---

## Performances Bare-Metal C SDK

1. **Séparation Réelle Dual-Core :**
   - **CORE 0 :** Piles USB TinyUSB, TCP/IP lwIP, serveur web HTTP, récepteur UDP Art-Net, affichage OLED et Watchdog.
   - **CORE 1 :** Moteur DMX512 dédié en priorité maximale sans interruption, garantissant un signal DMX exempt de gigue.

2. **Moteur DMX512 Matériel (8 State Machines PIO + DMA) :**
   - Exploite les deux blocs PIO (PIO0 et PIO1) du RP2040.
   - Génération matérielle précise des impulsions BREAK (140 µs) et MAB (16 µs) à 250 kbauds (8N2).

3. **Double Búfer Ping-Pong Atomique & ArtSync :**
   - Échange atomique de pointeurs éliminant tout effet de déchirement visuel.
   - Prise en charge d'ArtSync (OpSync) pour une synchronisation parfaite entre les univers.

---

## Broyage Matériel (Pinout)

| Fonction | Broche GPIO | Broche Physique (Pico) | Périphérique | Note |
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
| **LED DMX Ext** | **GP10** | Broche 14 | Sortie Numérique GPIO | Témoin d'activité DMX |
| **LED Carte** | **GP25** | LED intégrée | Sortie Numérique GPIO | Miroir de GP10 |

---

## Câblage Émetteurs-Récepteurs RS-485 (MAX485 / SN75176)

Pour chaque sortie DMX (GP0 à GP7) :
- **DI (Data In) :** Relier à la broche GPIO correspondante.
- **DE + RE :** Relier ensemble au `3V3` (mode transmission permanente).
- **VCC :** Relier au `5V` (VBUS USB).
- **GND :** Relier au `GND` commun.
- **A (+) :** Signal DMX positif (Broche XLR 3).
- **B (-) :** Signal DMX négatif (Broche XLR 2).
- **XLR Broche 1 :** Masse / Blindage (GND).

---

## Binaire Compilé (.UF2)

Disponible dans [`binarios_finais/artnet_dmx.uf2`](../../binarios_finais/artnet_dmx.uf2) ou sur la page des [Releases GitHub](https://github.com/Matheus-devpixel/Pico-ArtNet-DMX-Node/releases).
Pour installer, maintenez le bouton **BOOTSEL** enfoncé, connectez le câble USB, et glissez-déposez le fichier `.uf2` sur le volume `RPI-RP2`.

---

## Auteur et Développeur

- **Développeur :** Matheus Dev
- **GitHub :** [@Matheus-devpixel](https://github.com/Matheus-devpixel)

---

## Licence

Distribué sous la **Licence MIT**, autorisant l'utilisation personnelle, éducative et commerciale.
