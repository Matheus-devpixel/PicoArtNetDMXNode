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

## Panoramica del Progetto

Firmware nativo ad altissime prestazioni per microcontrollori **Raspberry Pi RP2040 (Raspberry Pi Pico, Pico W e Waveshare RP2040-Zero)** con architettura **100% dedicata tramite cavo USB (Ethernet over USB: RNDIS / CDC-ECM)**.

Progettato specificamente per la trasmissione professionale di illuminazione scenica DMX512 tramite Art-Net UDP (Porta 6454), elaborando da **1 a 8 universi DMX simultanei** (fino a 4.096 canali) in parallelo a 44 Hz stabili senza jitter.

---

## Architettura di Rete USB Ethernet (Plug-and-Play)

Collegato a qualsiasi computer tramite cavo USB, l'RP2040 funge direttamente da **scheda di rete Ethernet virtuale ad alta velocità**:
- **Windows:** Driver nativo Remote NDIS (**RNDIS**) tramite descrittori Microsoft OS 1.0, riconosciuto istantaneamente senza software aggiuntivo.
- **macOS / Linux:** Driver nativo **CDC-ECM**.
- **Server DHCP Integrato:** Assegnazione automatica dell'indirizzo IP all'host sulla stessa sottorete, eliminando qualsiasi configurazione manuale.

### Modalità Operative e Indirizzi IP Sicuri

Per evitare conflitti con reti Wi-Fi o aziendali (`192.168.x.x`):

1. **Modalità di Configurazione Iniziale (Setup Wizard):**
   - **IP:** `10.10.0.1` (l'host riceve `10.10.0.2`).
   - Si attiva automaticamente al primo avvio o dopo un ripristino di fabbrica per impostare ID del nodo, numero di uscite (1-8), nome, lingua, tema e frequenza DMX.

2. **Modalità Operativa Multi-Nodo (Art-Net Attivo):**
   - **IP:** `2.0.N.1` (dove `N` è l'ID del nodo da 1 a 16).
   - Supporto nativo fino a **16 nodi simultanei** collegati allo stesso computer o hub USB (128 universi / 65.536 canali DMX):
     - **Nodo 1:** IP `2.0.1.1` -> Universi 0 a 7
     - **Nodo 2:** IP `2.0.2.1` -> Universi 8 a 15
     - **Nodo 3:** IP `2.0.3.1` -> Universi 16 a 23
     - *(configurabile fino al Nodo 16: IP `2.0.16.1` -> Universi 120 a 127)*

---

## Supporto Multilingue Completo (8 Lingue)

Supporto integrato per: Portoghese [PT], Inglese [EN], Spagnolo [ES], Francese [FR], Tedesco [DE], Italiano [IT], Russo [RU] e Cinese [ZH]. Il cambio di lingua è immediato su tutte le pagine web e sullo schermo OLED I2C, rimanendo salvato nella memoria Flash SPI interna.

---

## Caratteristiche di Prestazione Bare-Metal C SDK

1. **Separazione Reale Dual-Core:**
   - **CORE 0:** Gestione stack USB TinyUSB, TCP/IP lwIP, server HTTP (Porta 80), ricevitore UDP Art-Net (Porta 6454), display OLED SSD1306 e Watchdog di sicurezza.
   - **CORE 1:** Motore DMX512 dedicato a priorità massima senza interruzioni. Genera trame DMX senza jitter né latenza, completamente isolato dal traffico di rete.

2. **Generazione DMX512 Hardware (8 State Machines PIO + DMA):**
   - Utilizza i due blocchi PIO (PIO0 e PIO1) su 8 macchine a stati parallele.
   - Generazione hardware precisa di impulsi BREAK (140 µs) e MAB (16 µs) a 250 kbaud (8N2).
   - Uscita continua e simultanea di fino a 8 universi a 44 Hz fissi.

3. **Doppio Buffer Atomico Ping-Pong & ArtSync:**
   - Scambio atomico di puntatori che elimina ogni effetto di tearing visivo sui fari.
   - Pieno supporto ai pacchetti ArtSync (OpSync) per la sincronizzazione simultanea di tutti gli universi.

---

## Mappatura dei Pin di Hardware (Pinout)

Mappatura ordinata, pulita e stabile:

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
| **LED DMX Ext** | **GP10** | Pin 14 | Uscita Digitale GPIO | LED esterno di attività DMX |
| **LED Onboard** | **GP25** | LED integrato | Uscita Digitale GPIO | Riflette GP10 |

*I pin DMX non configurati vengono disattivati via software in alta impedenza per la massima protezione elettrica.*

---

## Cablaggio dei Moduli RS-485 (MAX485 / SN75176)

Per ogni uscita DMX (da GP0 a GP7):
- **DI (Data In):** Collegare al rispettivo pin GPIO dell'RP2040.
- **DE + RE:** Collegare insieme a `3V3` (modalità di trasmissione continua).
- **VCC:** Collegare a `5V` (VBUS USB).
- **GND:** Collegare al `GND` comune del circuito.
- **A (+):** Linea DMX positiva (Pin XLR 3).
- **B (-):** Linea DMX negativa (Pin XLR 2).
- **XLR Pin 1:** Schermatura / Massa (GND).

---

## Binario Compilato Pronto (.UF2)

Disponibile in:
- **`binarios_finais/artnet_dmx.uf2`** (o su [GitHub Releases](https://github.com/Matheus-devpixel/PicoArtNetDMXNode/releases))

### Come Installare su Raspberry Pi RP2040:
1. Tenere premuto il tasto **BOOTSEL** sulla scheda Pico.
2. Collegare il cavo USB al PC e rilasciare il tasto. Comparirà l'unità `RPI-RP2`.
3. Trascinare il file `artnet_dmx.uf2` dentro `RPI-RP2`.
4. La scheda si riavvierà immediatamente e sarà operativa su IP `10.10.0.1` o `2.0.N.1`.

---

## Compilazione dai Codici Sorgente

Per ricompilare il firmware dal codice in `src/`:
- Eseguire: `python compile_targets.py`
- Lo script compila con Ninja, genera il file `.uf2` ufficiale e aggiorna la cartella `binarios_finais/`.

---

## Autore e Sviluppatore

- **Sviluppatore:** Matheus Dev
- **GitHub:** [@Matheus-devpixel](https://github.com/Matheus-devpixel)

---

## Utilità di Test di Trasmissione (ArtNet_Test.py)

Il repository include un programma di test ad alta precisione in Python ([`ArtNet_Test.py`](../../ArtNet_Test.py)) capace di trasmettere fino a 4.096 canali DMX (8 universi) a 60 FPS senza jitter:

```bash
# Simulazione scenica completa (moving heads, wash RGB, barre pixel, strobo)
python ArtNet_Test.py

# Specifica dell'IP di destinazione e framerate (es. 60 FPS su 2.0.1.1)
python ArtNet_Test.py 60 2.0.1.1

# Modalità strobo sincronizzata su tutti i 4.096 canali
python ArtNet_Test.py --ip=2.0.1.1 --mode=strobe

# Modalità gradiente continuo RGB
python ArtNet_Test.py --ip=2.0.1.1 --mode=rgb
```

---

## Crediti e Riconoscimenti

1. **Renzo Mischianti ([www.mischiant.org](https://www.mischiant.org)):** Diagrammi grafici ad alta risoluzione del piedinaggio di Raspberry Pi Pico e Waveshare RP2040-Zero.
2. **Sergey Fetisov ([fetisov/lrndis](https://github.com/fetisov/lrndis)):** Implementazione RNDIS leggera e server DHCP/DNS per lwIP (Licenza MIT).
3. **Colin O'Flynn (NewAE Technology / LUFA):** Definizioni del protocollo RNDIS / NDIS (Licenza BSD-3-Clause).
4. **Ha Thach ([tinyusb.org](https://github.com/hathach/tinyusb)):** TinyUSB Device Stack (Licenza MIT).
5. **Raspberry Pi Ltd.:** Pico C/C++ SDK, architettura PIO e assemblatore pioasm (Licenza BSD-3-Clause).
6. **Adam Dunkels / SICS:** Stack TCP/IP lwIP per sistemi embedded (Licenza BSD).

---

## Licenza

Distribuito con **Licenza MIT**, che consente l'uso libero per scopi personali, educativi e commerciali.
