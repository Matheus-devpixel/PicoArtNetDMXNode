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

## Projektübersicht

Native Bare-Metal-Firmware höchster Leistung für **Raspberry Pi RP2040**-Mikrocontroller (**Raspberry Pi Pico, Pico W und Waveshare RP2040-Zero**) mit **100% dedizierter USB-Kabelarchitektur (Ethernet over USB: RNDIS / CDC-ECM)**.

Entwickelt für professionelle DMX512-Bühnenlichtübertragung über Art-Net UDP (Port 6454), mit paralleler Verarbeitung von **1 bis 8 gleichzeitigen DMX-Universen** (bis zu 4.096 Kanälen) bei exakt 44 Hz ohne Jitter.

---

## USB-Ethernet-Netzwerkarchitektur (Plug-and-Play)

Über ein normales USB-Kabel mit dem PC verbunden, arbeitet der RP2040 direkt als **virtuelle Hochgeschwindigkeits-Ethernet-Netzwerkkarte**:
- **Windows:** Nativer Remote-NDIS-Treiber (**RNDIS**) über Microsoft OS 1.0 Deskriptoren – wird sofort ohne manuelle Treiberinstallation erkannt.
- **macOS / Linux:** Nativer **CDC-ECM**-Treiber.
- **Integrierter DHCP-Server:** Der Pico weist dem Host-Rechner automatisch eine IP-Adresse im selben Subnetz zu.

### Betriebsmodi und sichere IP-Bereiche

Zur Vermeidung von Konflikten mit WLAN- oder Unternehmensnetzwerken (`192.168.x.x`):

1. **Initialer Einrichtungsmodus (Setup Wizard):**
   - **IP:** `10.10.0.1` (Host erhält `10.10.0.2`).
   - Aktiviert sich automatisch beim ersten Booten oder nach einem Werksreset. Stellt ausschließlich das Webportal bereit zur Auswahl von Knoten-ID, Ausgängen (1-8), Gerätename, Sprache, Farbthema und DMX-Frequenz.

2. **Multi-Node-Betriebsmodus (Art-Net Aktiv):**
   - **IP:** `2.0.N.1` (wobei `N` die Knoten-ID von 1 bis 16 ist).
   - Nativer Betrieb von bis zu **16 Knoten gleichzeitig** am selben PC oder USB-Hub (128 Universen / 65.536 DMX-Kanäle):
     - **Knoten 1:** IP `2.0.1.1` -> Universen 0 bis 7
     - **Knoten 2:** IP `2.0.2.1` -> Universen 8 bis 15
     - **Knoten 3:** IP `2.0.3.1` -> Universen 16 bis 23
     - *(konfigurierbar bis Knoten 16: IP `2.0.16.1` -> Universen 120 bis 127)*

---

## Vollständige Mehrsprachigkeit (8 Sprachen)

Eingebaute Lokalisierung für: Portugiesisch [PT], Englisch [EN], Spanisch [ES], Französisch [FR], Deutsch [DE], Italienisch [IT], Russisch [RU] und Chinesisch [ZH]. Sprachänderungen werden sofort im Webserver und auf dem I2C-OLED-Display übernommen und im internen SPI-Flash gespeichert.

---

## Bare-Metal C SDK Performance-Highlights

1. **Echte Dual-Core-Trennung:**
   - **CORE 0:** Verwaltet TinyUSB-Stack, lwIP TCP/IP, HTTP-Webserver (Port 80), Art-Net UDP-Empfänger (Port 6454), SSD1306 OLED-Rendering und Hardware-Watchdog.
   - **CORE 1:** Dedizierter DMX512-Engine auf maximaler Priorität ohne Unterbrechungen. Erzeugt DMX-Frames völlig frei von Jitter und Latenzen, isoliert vom Netzwerkverkehr.

2. **Hardware-DMX512 über PIO + DMA (8 State Machines):**
   - Nutzt beide PIO-Blöcke (PIO0 und PIO1) über 8 parallele State Machines.
   - Präzise BREAK- (140 µs) und MAB- (16 µs) Impulse in Hardware bei 250 kbaud (8N2).
   - Kontinuierliche Ausgabe von bis zu 8 Universen bei festen 44 Hz.

3. **Atomares Ping-Pong-Doppelpuffer & ArtSync:**
   - Atomarer Zeigertausch verhindert Bildrisse (Tearing) an den Scheinwerfern.
   - Volle Unterstützung für ArtSync-Pakete (OpSync) zur synchronen Aktualisierung aller Universen.

---

## Hardware-Pinbelegung (Pinout)

Kompakte, saubere und robuste Pin-Zuweisung:

| Funktion | GPIO-Pin | Physischer Pin (Pico) | Peripherie / Bus | Anmerkung |
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

*Nicht konfigurierte DMX-Pins werden per Software in den hochohmigen Zustand (High-Z) versetzt.*

---

## RS-485 Transceiver-Verdrahtung (MAX485 / SN75176)

Für jeden DMX-Ausgang (GP0 bis GP7):
- **DI (Data In):** Mit dem jeweiligen RP2040-GPIO-Pin verbinden.
- **DE + RE:** Zusammen an `3V3` legen (dauerhafter Sendemodus).
- **VCC:** An `5V` (USB VBUS) anschließen.
- **GND:** Mit dem gemeinsamen Schaltungs-`GND` verbinden.
- **A (+):** DMX-Plus-Signal (XLR Pin 3).
- **B (-):** DMX-Minus-Signal (XLR Pin 2).
- **XLR Pin 1:** Schirmung / Masse (GND).

---

## Vorkompilierte Binärdatei (.UF2)

Sofort einsatzbereit unter:
- **`binarios_finais/artnet_dmx.uf2`** (oder auf [GitHub Releases](https://github.com/Matheus-devpixel/PicoArtNetDMXNode/releases))

### Installation auf dem Raspberry Pi RP2040:
1. **BOOTSEL**-Taste auf dem Pico gedrückt halten.
2. USB-Kabel einstecken und Taste loslassen. Das Laufwerk `RPI-RP2` wird gemountet.
3. Die Datei `artnet_dmx.uf2` auf `RPI-RP2` kopieren.
4. Das Board startet neu und ist sofort als USB-Netzwerkkarte erreichbar unter `10.10.0.1` oder `2.0.N.1`.

---

## Kompilieren aus dem Quellcode

Um die Firmware aus dem Ordner `src/` neu zu erstellen:
- Ausführen: `python compile_targets.py`
- Das Skript kompiliert mit Ninja, erzeugt das offizielle `.uf2`-Format und aktualisiert den Ordner `binarios_finais/`.

---

## Autor & Entwickler

- **Entwickler:** Matheus Dev
- **GitHub:** [@Matheus-devpixel](https://github.com/Matheus-devpixel)

---

## Übertragungstestprogramm (ArtNet_Test.py)

Das Repository enthält ein präzises Python-Testprogramm ([`ArtNet_Test.py`](../../ArtNet_Test.py)), das bis zu 4.096 DMX-Kanäle gleichzeitig (8 Universen) bei festen 60 FPS ohne Jitter senden kann:

```bash
# Standard-Bühnensimulation (Moving Heads, RGB Wash, Pixelbars, Strobes)
python ArtNet_Test.py

# Ziel-IP und Bildrate anpassen (z. B. 60 FPS an 2.0.1.1)
python ArtNet_Test.py 60 2.0.1.1

# Synchronisierter Strobe-Modus über alle 4.096 Kanäle
python ArtNet_Test.py --ip=2.0.1.1 --mode=strobe

# Weicher RGB-Farbverlauf
python ArtNet_Test.py --ip=2.0.1.1 --mode=rgb
```

---

## Danksagung & Credits

Dieses Projekt basiert auf anerkannten Open-Source-Beiträgen:

1. **Renzo Mischianti ([www.mischiant.org](https://www.mischiant.org)):** Hochauflösende Pinout-Grafiken für Pico und RP2040-Zero.
2. **Sergey Fetisov ([fetisov/lrndis](https://github.com/fetisov/lrndis)):** Schlanke Microsoft RNDIS-Pilha und DHCP/DNS-Server für lwIP (MIT-Lizenz).
3. **Colin O'Flynn (NewAE Technology / LUFA):** Protokolldefinitionen für Microsoft RNDIS / NDIS (BSD-3-Clause).
4. **Ha Thach ([tinyusb.org](https://github.com/hathach/tinyusb)):** TinyUSB Device Stack (MIT-Lizenz).
5. **Raspberry Pi Ltd.:** Pico C/C++ SDK, PIO-Architektur und pioasm-Assembler (BSD-3-Clause).
6. **Adam Dunkels / SICS:** lwIP TCP/IP-Stack für Embedded-Systeme (BSD-Lizenz).

---

## Lizenz

Veröffentlicht unter der **MIT-Lizenz** für die freie private, schulische und kommerzielle Nutzung.
