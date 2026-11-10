<div align="center">

# Pico ArtNet DMX Node (Raspberry Pi Pico RP2040)

**High-performance bare-metal firmware for professional DMX512 stage lighting control over USB Ethernet (RNDIS / CDC-ECM)**

[![Português](https://img.shields.io/badge/Idioma-Portugu%C3%AAs-blue?style=for-the-badge)](../../README.md)
[![English](https://img.shields.io/badge/Language-English-green?style=for-the-badge)](#)
[![Español](https://img.shields.io/badge/Idioma-Espa%C3%B1ol-yellow?style=for-the-badge)](README_ES.md)
[![Français](https://img.shields.io/badge/Langue-Fran%C3%A7ais-red?style=for-the-badge)](README_FR.md)
[![Deutsch](https://img.shields.io/badge/Sprache-Deutsch-orange?style=for-the-badge)](README_DE.md)
[![Italiano](https://img.shields.io/badge/Lingua-Italiano-teal?style=for-the-badge)](README_IT.md)
[![Русский](https://img.shields.io/badge/%D0%AF%D0%B7%D1%8B%D0%BA-%D0%A0%D1%83%D1%81%D1%81%D0%BA%D0%B8%D0%B9-purple?style=for-the-badge)](README_RU.md)
[![中文](https://img.shields.io/badge/%E8%AF%AD%E8%A8%80-%E4%B8%AD%E6%96%87-lightgrey?style=for-the-badge)](README_ZH.md)

</div>

---

Native, high-performance firmware for **Raspberry Pi RP2040** microcontrollers (**Raspberry Pi Pico, Pico W, and Waveshare RP2040-Zero**) featuring a **100% dedicated USB cable architecture (Ethernet over USB: RNDIS / CDC-ECM)**.

Designed specifically for professional DMX512 stage lighting transmission over Art-Net UDP (Port 6454), processing from **1 to 8 concurrent DMX universes** (up to 4,096 channels) in parallel at a solid 44 Hz refresh rate without jitter.

---

## USB Ethernet Network Architecture (Plug-and-Play)

When connected to any computer via USB cable, the RP2040 acts directly as a **high-speed virtual Ethernet network adapter**:
- **Windows:** Native Remote NDIS (**RNDIS**) driver via Microsoft OS 1.0 Descriptors, instantly recognized without third-party drivers or software installation.
- **macOS / Linux:** Native **CDC-ECM** driver.
- **Built-in DHCP Server:** The Pico automatically assigns the host computer's IP address on the same subnet, eliminating any manual network configuration.

### Operational Modes and Safe IP Ranges

To prevent IP conflicts with Wi-Fi or corporate networks (`192.168.x.x`), the system operates on dedicated subnets:

1. **Initial Provisioning Mode (Setup Wizard):**
   - **IP:** `10.10.0.1` (Host receives `10.10.0.2`).
   - Automatically triggered on first boot or after a factory reset.
   - The node exclusively serves the configuration portal to select Node ID (Universe Range), Number of Active Outputs (1 to 8), Friendly Name, Language, UI Theme, and DMX Frame Rate.

2. **Multi-Node Operational Mode (Art-Net Active):**
   - **IP:** `2.0.N.1` (where `N` is the Node ID from 1 to 16).
   - Native support for up to **16 simultaneous nodes** connected to the same computer or USB hub (128 universes / 65,536 DMX channels):
     - **Node 1:** IP `2.0.1.1` -> Universes 0 to 7
     - **Node 2:** IP `2.0.2.1` -> Universes 8 to 15
     - **Node 3:** IP `2.0.3.1` -> Universes 16 to 23
     - *(configurable up to Node 16: IP `2.0.16.1` -> Universes 120 to 127)*

---

## Comprehensive Multilingual Support (8 Languages)

The firmware includes built-in localization for:
- **Português (Brasil)** [PT]
- **English** [EN]
- **Español** [ES]
- **Français** [FR]
- **Deutsch** [DE]
- **Italiano** [IT]
- **Русский (Russian)** [RU]
- **中文 (Simplified Chinese)** [ZH]

Language selection updates all web pages and the I2C OLED display in real time, permanently stored in onboard SPI Flash memory.

---

## Bare-Metal C SDK Performance Highlights

1. **True Dual-Core Separation:**
   - **CORE 0:** Handles TinyUSB stack, lwIP TCP/IP stack, HTTP Web Server (Port 80), Art-Net UDP Receiver (Port 6454), SSD1306 OLED rendering, and hardware Watchdog timer.
   - **CORE 1:** Dedicated, uninterrupted DMX512 engine running at maximum priority. Generates DMX frames with zero jitter or latency, completely isolated from network traffic.

2. **Hardware DMX512 Engine (8 PIO State Machines + DMA):**
   - Utilizes both RP2040 PIO blocks (PIO0 and PIO1) across 8 parallel State Machines.
   - Precise 140 µs BREAK and 16 µs MAB pulses generated directly in hardware at 250 kbaud (8N2).
   - Continuous simultaneous transmission of up to 8 universes at locked 44 Hz.

3. **Atomic Ping-Pong Double-Buffering & ArtSync:**
   - Atomic pointer swapping prevents visual tearing across fixtures.
   - Full support for ArtSync packets (OpSync) for perfectly synchronized universe updates.

---

## Hardware Pinout

Contiguous, clean, and stable pin mapping:

| Function | GPIO Pin | Physical Pin (Pico) | Peripheral / Bus | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **DMX Output 1** | **GP0** | Pin 1 | PIO0 - SM 0 | Start Universe + 0 |
| **DMX Output 2** | **GP1** | Pin 2 | PIO0 - SM 1 | Start Universe + 1 |
| **DMX Output 3** | **GP2** | Pin 4 | PIO0 - SM 2 | Start Universe + 2 |
| **DMX Output 4** | **GP3** | Pin 5 | PIO0 - SM 3 | Start Universe + 3 |
| **DMX Output 5** | **GP4** | Pin 6 | PIO1 - SM 0 | Start Universe + 4 |
| **DMX Output 6** | **GP5** | Pin 7 | PIO1 - SM 1 | Start Universe + 5 |
| **DMX Output 7** | **GP6** | Pin 9 | PIO1 - SM 2 | Start Universe + 6 |
| **DMX Output 8** | **GP7** | Pin 10 | PIO1 - SM 3 | Start Universe + 7 |
| **OLED SDA** | **GP8** | Pin 11 | I2C0 SDA (400 kHz) | SSD1306 128x64 Display |
| **OLED SCL** | **GP9** | Pin 12 | I2C0 SCL (400 kHz) | SSD1306 128x64 Display |
| **DMX LED Ext** | **GP10** | Pin 14 | Digital Output GPIO | Dedicated external DMX activity LED |
| **Onboard LED** | **GP25** | Onboard LED | Digital Output GPIO | Built-in Pico LED (mirrors GP10) |

*Unconfigured DMX pins are placed in high-impedance mode by software for electrical protection.*

---

## RS-485 Transceiver Wiring (MAX485 / SN75176)

For each DMX output (GP0 to GP7):
- **DI (Data In):** Connect to the respective RP2040 GPIO pin.
- **DE (Driver Enable) + RE (Receiver Enable):** Tie together to `3V3` (maintains transmitter mode).
- **VCC:** Connect to `5V` (USB VBUS).
- **GND:** Connect to common circuit `GND`.
- **A (+):** DMX positive signal (XLR Pin 3).
- **B (-):** DMX negative signal (XLR Pin 2).
- **XLR Pin 1:** Shield / Ground (GND).

---

## Precompiled Binary (.UF2)

Ready-to-flash binary available in:
- **`binarios_finais/artnet_dmx.uf2`** (or via [GitHub Releases](https://github.com/Matheus-devpixel/PicoArtNetDMXNode/releases))

### How to Flash to Raspberry Pi RP2040:
1. Press and hold the **BOOTSEL** button on the Pico.
2. Plug the USB cable into your computer and release the button. A drive named `RPI-RP2` will appear.
3. Drag and drop `artnet_dmx.uf2` onto `RPI-RP2`.
4. The board will reboot instantly and be recognized as a USB Ethernet network adapter on IP `10.10.0.1` (Setup) or `2.0.N.1` (Operational).

---

## Building from Source

To compile the firmware from the source code in `src/`:
- Run: `python compile_targets.py`
- The build script compiles the firmware with Ninja, converts the binary into the official RP2040 `.uf2` format, and updates `binarios_finais/`.

---

## Author & Developer

- **Developer:** Matheus Dev
- **GitHub:** [@Matheus-devpixel](https://github.com/Matheus-devpixel)

---

## Transmission Test Utility (ArtNet_Test.py)

The repository includes a precision Python test and stress generator ([`ArtNet_Test.py`](../../ArtNet_Test.py)) capable of transmitting up to 4,096 simultaneous DMX channels (8 universes of 512 channels) at locked 60 FPS without jitter:

```bash
# Default stage simulation (moving heads, RGB wash, pixel bars, strobes)
python ArtNet_Test.py

# Custom target IP and frame rate (e.g. 60 FPS on IP 2.0.1.1)
python ArtNet_Test.py 60 2.0.1.1

# Synchronized strobe mode across all 4,096 channels
python ArtNet_Test.py --ip=2.0.1.1 --mode=strobe

# Smooth RGB color wheel gradient
python ArtNet_Test.py --ip=2.0.1.1 --mode=rgb
```

---

## Credits & Acknowledgements

This project builds upon outstanding open-source contributions from the embedded systems community:

1. **Renzo Mischianti ([www.mischiant.org](https://www.mischiant.org)):**
   - High-resolution pinout diagrams for Raspberry Pi Pico / Pico W and Waveshare RP2040-Zero used in web dashboard modal viewers.
2. **Sergey Fetisov ([fetisov/lrndis](https://github.com/fetisov/lrndis)):**
   - Lightweight Microsoft RNDIS implementation and embedded DHCP/DNS servers for lwIP (MIT License).
3. **Colin O'Flynn (NewAE Technology / LUFA USB Library):**
   - Original protocol definitions and header files for Microsoft RNDIS / NDIS (BSD-3-Clause License).
4. **Ha Thach ([tinyusb.org](https://github.com/hathach/tinyusb)):**
   - TinyUSB Device Stack (MIT License).
5. **Raspberry Pi Ltd.:**
   - Raspberry Pi Pico C/C++ SDK, PIO architecture, and pioasm compiler (BSD-3-Clause License).
6. **Adam Dunkels / Swedish Institute of Computer Science (SICS):**
   - lwIP (Lightweight TCP/IP Stack) for embedded systems (BSD License).

---

## License

This firmware is distributed under the **MIT License**, permitting free personal, educational, and commercial use while preserving original third-party copyright notices.
