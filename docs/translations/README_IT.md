<div align="center">

# Pico ArtNet DMX Node (Raspberry Pi Pico RP2040)

**Firmware nativo bare-metal ad alte prestazioni per il controllo professionale dell'illuminazione scenica DMX512 via USB Ethernet (RNDIS / CDC-ECM)**

[![Português](https://img.shields.io/badge/Idioma-Portugu%C3%AAs-blue?style=for-the-badge)](../../README.md)
[![English](https://img.shields.io/badge/Language-English-green?style=for-the-badge)](README_EN.md)
[![Español](https://img.shields.io/badge/Idioma-Espa%C3%B1ol-yellow?style=for-the-badge)](README_ES.md)
[![Français](https://img.shields.io/badge/Langue-Fran%C3%A7ais-red?style=for-the-badge)](README_FR.md)
[![Deutsch](https://img.shields.io/badge/Sprache-Deutsch-orange?style=for-the-badge)](README_DE.md)
[![Italiano](https://img.shields.io/badge/Lingua-Italiano-teal?style=for-the-badge)](#)
[![Русский](https://img.shields.io/badge/%D0%AF%D0%B7%D1%8B%D0%BA-%D0%A0%D1%83%D1%81%D1%81%D0%BA%D0%B8%D0%B9-purple?style=for-the-badge)](README_RU.md)
[![中文](https://img.shields.io/badge/%E8%AF%AD%E8%A8%80-%E4%B8%AD%E6%96%87-lightgrey?style=for-the-badge)](README_ZH.md)

</div>

---

Firmware nativo ad altissime prestazioni per microcontrollori **Raspberry Pi RP2040 (Raspberry Pi Pico, Pico W e Waveshare RP2040-Zero)** con architettura **100% dedicata tramite cavo USB (Ethernet over USB: RNDIS / CDC-ECM)**.

Progettato specificamente per la trasmissione professionale di illuminazione scenica DMX512 tramite Art-Net UDP (Porta 6454), elaborando da **1 a 8 universi DMX simultanei** (fino a 4.096 canali) in parallelo a 44 Hz stabili senza jitter.

---

## Architettura di Rete USB Ethernet (Plug-and-Play)

Collegato a qualsiasi computer tramite cavo USB, l'RP2040 funge direttamente da **scheda di rete Ethernet virtuale ad alta velocità**:
- **Windows:** Driver nativo Remote NDIS (**RNDIS**) tramite descrittori Microsoft OS 1.0, riconosciuto istantaneamente senza driver di terze parti.
- **macOS / Linux:** Driver nativo **CDC-ECM**.
- **Server DHCP Integrato:** Assegnazione automatica dell'indirizzo IP all'host nella stessa sottorete.

### Modalità Operative e Indirizzi IP

1. **Modalità di Configurazione Iniziale (Setup Wizard):**
   - **IP:** `10.10.0.1` (l'host riceve `10.10.0.2`).
   - Si attiva automaticamente al primo avvio o dopo un ripristino di fabbrica per impostare ID del nodo, numero di uscite (1-8), nome, lingua e velocità DMX.

2. **Modalità Operativa Multi-Nodo (Art-Net Attivo):**
   - **IP:** `2.0.N.1` (dove `N` è l'ID del nodo da 1 a 16).
   - Supporto nativo fino a **16 nodi simultanei** (128 universi / 65.536 canali DMX).

---

## Supporto Multilingue (8 Lingue)

Supporto integrato per: Portoghese [PT], Inglese [EN], Spagnolo [ES], Francese [FR], Tedesco [DE], Italiano [IT], Russo [RU] e Cinese [ZH].

---

## Mappatura dei Pin di Hardware (Pinout)

| Funzione | Pin GPIO | Pin Fisico (Pico) | Periferica / Bus | Note |
| :--- | :--- | :--- | :--- | :--- |
| **Uscita DMX 1** | **GP0** | Pin 1 | PIO0 - SM 0 | Universo iniziale + 0 |
| **Uscita DMX 2** | **GP1** | Pin 2 | PIO0 - SM 1 | Universo iniziale + 1 |
| **Uscita DMX 3** | **GP2** | Pin 4 | PIO0 - SM 2 | Universo iniziale + 2 |
| **Uscita DMX 4** | **GP3** | Pin 5 | PIO0 - SM 3 | Universo iniziale + 3 |
| **Uscita DMX 5** | **GP4** | Pin 6 | PIO1 - SM 0 | Universo iniziale + 4 |
| **Uscita DMX 6** | **GP5** | Pin 7 | PIO1 - SM 1 | Universo iniziale + 5 |
| **Uscita DMX 7** | **GP6** | Pin 9 | PIO1 - SM 2 | Universo iniziale + 6 |
| **Uscita DMX 8** | **GP7** | Pin 10 | PIO1 - SM 3 | Universo iniziale + 7 |
| **OLED SDA** | **GP8** | Pin 11 | I2C0 SDA (400 kHz) | Display SSD1306 128x64 |
| **OLED SCL** | **GP9** | Pin 12 | I2C0 SCL (400 kHz) | Display SSD1306 128x64 |
| **LED DMX Ext** | **GP10** | Pin 14 | Uscita Digitale GPIO | LED di attività DMX esterno |
| **LED Onboard** | **GP25** | LED integrato | Uscita Digitale GPIO | Specchio di GP10 |

---

## Binario Compilato (.UF2)

Scaricabile in [`binarios_finais/artnet_dmx.uf2`](../../binarios_finais/artnet_dmx.uf2) o nella sezione [GitHub Releases](https://github.com/Matheus-devpixel/Pico-ArtNet-DMX-Node/releases).

---

## Sviluppatore e Licenza

- **Sviluppatore:** Matheus Dev ([@Matheus-devpixel](https://github.com/Matheus-devpixel))
- **Licenza:** **Licenza MIT**
