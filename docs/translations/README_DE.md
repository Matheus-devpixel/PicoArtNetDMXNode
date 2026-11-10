<div align="center">

# Pico ArtNet DMX Node (Raspberry Pi Pico RP2040)

**Leistungsstarke Bare-Metal-Firmware für professionelle DMX512-Bühnenbeleuchtungssteuerung über USB-Ethernet (RNDIS / CDC-ECM)**

[![Português](https://img.shields.io/badge/Idioma-Portugu%C3%AAs-blue?style=for-the-badge)](../../README.md)
[![English](https://img.shields.io/badge/Language-English-green?style=for-the-badge)](README_EN.md)
[![Español](https://img.shields.io/badge/Idioma-Espa%C3%B1ol-yellow?style=for-the-badge)](README_ES.md)
[![Français](https://img.shields.io/badge/Langue-Fran%C3%A7ais-red?style=for-the-badge)](README_FR.md)
[![Deutsch](https://img.shields.io/badge/Sprache-Deutsch-orange?style=for-the-badge)](#)
[![Italiano](https://img.shields.io/badge/Lingua-Italiano-teal?style=for-the-badge)](README_IT.md)
[![Русский](https://img.shields.io/badge/%D0%AF%D0%B7%D1%8B%D0%BA-%D0%A0%D1%83%D1%81%D1%81%D0%BA%D0%B8%D0%B9-purple?style=for-the-badge)](README_RU.md)
[![中文](https://img.shields.io/badge/%E8%AF%AD%E8%A8%80-%E4%B8%AD%E6%96%87-lightgrey?style=for-the-badge)](README_ZH.md)

</div>

---

Native Bare-Metal-Firmware für **Raspberry Pi RP2040**-Mikrocontroller (**Raspberry Pi Pico, Pico W und Waveshare RP2040-Zero**) mit **100% dedizierter USB-Kabelarchitektur (Ethernet over USB: RNDIS / CDC-ECM)**.

Entwickelt für professionelle DMX512-Bühnenlichtübertragung über Art-Net UDP (Port 6454) mit **1 bis 8 parallelen DMX-Universen** (bis zu 4.096 Kanälen) bei stabilen 44 Hz ohne Jitter.

---

## USB-Ethernet-Netzwerkarchitektur (Plug-and-Play)

Über ein USB-Kabel mit dem PC verbunden, fungiert der RP2040 als **virtuelle Hochgeschwindigkeits-Ethernet-Netzwerkkarte**:
- **Windows:** Nativer Remote-NDIS-Treiber (**RNDIS**) über Microsoft OS 1.0 Deskriptoren – sofort einsatzbereit ohne Treiberinstallation.
- **macOS / Linux:** Nativer **CDC-ECM**-Treiber.
- **Integrierter DHCP-Server:** Automatische IP-Zuweisung an den Host-Rechner im selben Subnetz.

### Betriebsmodi und IP-Adressbereiche

1. **Initialer Setup-Modus (Setup Wizard):**
   - **IP:** `10.10.0.1` (Host erhält `10.10.0.2`).
   - Automatisch aktiv beim ersten Start oder nach einem Werksreset. Bietet das Webportal zur Konfiguration von Knoten-ID, Ausgängen (1-8), Name, Sprache und DMX-Rate.

2. **Multi-Node-Betriebsmodus (Art-Net Aktiv):**
   - **IP:** `2.0.N.1` (wobei `N` die Knoten-ID von 1 bis 16 ist).
   - Nativer Betrieb von bis zu **16 Knoten gleichzeitig** (128 Universen / 65.536 DMX-Kanäle).

---

## Mehrsprachige Benutzeroberfläche (8 Sprachen)

Integrierte Unterstützung für: Portugiesisch [PT], Englisch [EN], Spanisch [ES], Französisch [FR], Deutsch [DE], Italienisch [IT], Russisch [RU] und Chinesisch [ZH].

---

## Hardware-Pinbelegung (Pinout)

| Funktion | GPIO-Pin | Physischer Pin (Pico) | Peripherie | Anmerkung |
| :--- | :--- | :--- | :--- | :--- |
| **DMX Ausgang 1** | **GP0** | Pin 1 | PIO0 - SM 0 | Start-Universum + 0 |
| **DMX Ausgang 2** | **GP1** | Pin 2 | PIO0 - SM 1 | Start-Universum + 1 |
| **DMX Ausgang 3** | **GP2** | Pin 4 | PIO0 - SM 2 | Start-Universum + 2 |
| **DMX Ausgang 4** | **GP3** | Pin 5 | PIO0 - SM 3 | Start-Universum + 3 |
| **DMX Ausgang 5** | **GP4** | Pin 6 | PIO1 - SM 0 | Start-Universum + 4 |
| **DMX Ausgang 6** | **GP5** | Pin 7 | PIO1 - SM 1 | Start-Universum + 5 |
| **DMX Ausgang 7** | **GP6** | Pin 9 | PIO1 - SM 2 | Start-Universum + 6 |
| **DMX Ausgang 8** | **GP7** | Pin 10 | PIO1 - SM 3 | Start-Universum + 7 |
| **OLED SDA** | **GP8** | Pin 11 | I2C0 SDA (400 kHz) | SSD1306 128x64 Display |
| **OLED SCL** | **GP9** | Pin 12 | I2C0 SCL (400 kHz) | SSD1306 128x64 Display |
| **DMX LED Ext** | **GP10** | Pin 14 | Digitaler Ausgang | Externe DMX-Aktivitäts-LED |
| **Onboard-LED** | **GP25** | LED auf Platine | Digitaler Ausgang | Spiegelt GP10 |

---

## Vorkompilierte Binärdatei (.UF2)

Verfügbar unter [`binarios_finais/artnet_dmx.uf2`](../../binarios_finais/artnet_dmx.uf2) oder in den [GitHub Releases](https://github.com/Matheus-devpixel/Pico-ArtNet-DMX-Node/releases).
Zur Installation halten Sie die **BOOTSEL**-Taste gedrückt, verbinden Sie das USB-Kabel und kopieren Sie die `.uf2`-Datei auf das Laufwerk `RPI-RP2`.

---

## Entwickler & Lizenz

- **Entwickler:** Matheus Dev ([@Matheus-devpixel](https://github.com/Matheus-devpixel))
- **Lizenz:** **MIT-Lizenz**
