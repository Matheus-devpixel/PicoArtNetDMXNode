<div align="center">

# Pico ArtNet DMX Node (Raspberry Pi Pico RP2040)

**Firmware nativo bare-metal de alto rendimiento para control profesional de iluminación DMX512 vía USB Ethernet (RNDIS / CDC-ECM)**

[![Português](https://img.shields.io/badge/Idioma-Portugu%C3%AAs-blue?style=for-the-badge)](../../README.md)
[![English](https://img.shields.io/badge/Language-English-green?style=for-the-badge)](README_EN.md)
[![Español](https://img.shields.io/badge/Idioma-Espa%C3%B1ol-yellow?style=for-the-badge)](#)
[![Français](https://img.shields.io/badge/Langue-Fran%C3%A7ais-red?style=for-the-badge)](README_FR.md)
[![Deutsch](https://img.shields.io/badge/Sprache-Deutsch-orange?style=for-the-badge)](README_DE.md)
[![Italiano](https://img.shields.io/badge/Lingua-Italiano-teal?style=for-the-badge)](README_IT.md)
[![Русский](https://img.shields.io/badge/%D0%AF%D0%B7%D1%8B%D0%BA-%D0%A0%D1%83%D1%81%D1%81%D0%BA%D0%B8%D0%B9-purple?style=for-the-badge)](README_RU.md)
[![中文](https://img.shields.io/badge/%E8%AF%AD%E8%A8%80-%E4%B8%AD%E6%96%87-lightgrey?style=for-the-badge)](README_ZH.md)

</div>

---

Firmware nativo de muy alto rendimiento para microcontroladores **Raspberry Pi RP2040 (Raspberry Pi Pico, Pico W y Waveshare RP2040-Zero)** con arquitectura **100% dedicada mediante cable USB (Ethernet over USB: RNDIS / CDC-ECM)**.

Diseñado específicamente para la transmisión profesional de iluminación escénica DMX512 mediante Art-Net UDP (Puerto 6454), procesando de **1 a 8 universos DMX simultáneos** (hasta 4.096 canales) en paralelo a 44 Hz exactos sin fluctuaciones (jitter).

---

## Arquitectura de Red USB Ethernet (Plug-and-Play)

Conectado a cualquier ordenador mediante cable USB, el RP2040 actúa directamente como una **tarjeta de red virtual Ethernet de alta velocidad**:
- **Windows:** Controlador nativo Remote NDIS (**RNDIS**) mediante descriptores Microsoft OS 1.0, detectado al instante sin software ni controladores externos.
- **macOS / Linux:** Controlador nativo **CDC-ECM**.
- **Servidor DHCP Integrado:** El Pico asigna automáticamente la dirección IP al ordenador en la misma subred, eliminando cualquier configuración manual de red.

### Modos de Operación y Rangos de IP Seguros

Para evitar conflictos de red con routers Wi-Fi o redes corporativas (`192.168.x.x`), el sistema utiliza rangos dedicados:

1. **Modo Inicial de Configuración (Setup Wizard):**
   - **IP:** `10.10.0.1` (el host recibe `10.10.0.2`).
   - Se activa automáticamente en el primer encendido o tras un restablecimiento de fábrica.
   - El nodo ofrece exclusivamente el portal web de configuración para elegir el ID del nodo, cantidad de salidas activas (1 a 8), nombre descriptivo, idioma, tema visual y tasa DMX.

2. **Modo Operativo Multi-Nodo (Art-Net Activo):**
   - **IP:** `2.0.N.1` (donde `N` es el ID del nodo de 1 a 16).
   - Soporte nativo para hasta **16 nodos simultáneos** conectados al mismo ordenador o hub USB (128 universos / 65.536 canales DMX):
     - **Nodo 1:** IP `2.0.1.1` -> Universos 0 a 7
     - **Nodo 2:** IP `2.0.2.1` -> Universos 8 a 15
     - **Nodo 3:** IP `2.0.3.1` -> Universos 16 a 23
     - *(configurable hasta el Nodo 16: IP `2.0.16.1` -> Universos 120 a 127)*

---

## Soporte Multilingüe Completo (8 Idiomas)

El firmware incluye localización interna en:
- **Português (Brasil)** [PT]
- **English** [EN]
- **Español** [ES]
- **Français** [FR]
- **Deutsch** [DE]
- **Italiano** [IT]
- **Русский (Ruso)** [RU]
- **中文 (Chino Simplificado)** [ZH]

El cambio de idioma se actualiza instantáneamente en todas las páginas web y en la pantalla OLED I2C, guardándose de manera persistente en la memoria Flash interna SPI.

---

## Aspectos Destacados de Rendimiento (Bare-Metal C SDK)

1. **Separación Real de Núcleos (Dual-Core):**
   - **CORE 0:** Gestiona la pila TinyUSB, pila TCP/IP lwIP, servidor web HTTP (Puerto 80), receptor UDP Art-Net (Puerto 6454), interfaz OLED SSD1306 y temporizador Watchdog de hardware.
   - **CORE 1:** Motor DMX512 dedicado, ininterrumpido y de máxima prioridad. Genera tramas DMX con cero jitter y latencia nula, aislado del tráfico de red.

2. **Transmisión DMX512 por Hardware (8 State Machines PIO + DMA):**
   - Utiliza los dos bloques PIO (PIO0 y PIO1) con 8 máquinas de estado funcionando en paralelo.
   - Pulso BREAK de 140 µs y MAB de 16 µs generados por hardware a 250 kbaud (8N2).
   - Salida simultánea continua de hasta 8 universos a 44 Hz sincronizados.

3. **Doble Búfer Ping-Pong Atómico y ArtSync:**
   - Intercambio atómico de punteros que elimina cualquier parpadeo o tearing en los proyectores.
   - Soporte para paquetes ArtSync (OpSync) para sincronización exacta entre universos.

---

## Mapeo de Pines de Hardware (Pinout)

Disposición contigua, limpia y robusta:

| Función | Pin GPIO | Pin Físico (Pico) | Periférico / Bus | Observación |
| :--- | :--- | :--- | :--- | :--- |
| **Salida DMX 1** | **GP0** | Pin 1 | PIO0 - SM 0 | Universo Inicial + 0 |
| **Salida DMX 2** | **GP1** | Pin 2 | PIO0 - SM 1 | Universo Inicial + 1 |
| **Salida DMX 3** | **GP2** | Pin 4 | PIO0 - SM 2 | Universo Inicial + 2 |
| **Salida DMX 4** | **GP3** | Pin 5 | PIO0 - SM 3 | Universo Inicial + 3 |
| **Salida DMX 5** | **GP4** | Pin 6 | PIO1 - SM 0 | Universo Inicial + 4 |
| **Salida DMX 6** | **GP5** | Pin 7 | PIO1 - SM 1 | Universo Inicial + 5 |
| **Salida DMX 7** | **GP6** | Pin 9 | PIO1 - SM 2 | Universo Inicial + 6 |
| **Salida DMX 8** | **GP7** | Pin 10 | PIO1 - SM 3 | Universo Inicial + 7 |
| **OLED SDA** | **GP8** | Pin 11 | I2C0 SDA (400 kHz) | Pantalla SSD1306 128x64 |
| **OLED SCL** | **GP9** | Pin 12 | I2C0 SCL (400 kHz) | Pantalla SSD1306 128x64 |
| **LED DMX Ext** | **GP10** | Pin 14 | Salida Digital GPIO | LED externo de actividad DMX |
| **LED Integrado** | **GP25** | LED de Placa | Salida Digital GPIO | LED nativo del Pico (refleja GP10) |

*Los pines DMX deshabilitados se configuran en alta impedancia por software para máxima protección eléctrica.*

---

## Conexión de Módulos RS-485 (MAX485 / SN75176)

Para cada salida DMX (GP0 a GP7):
- **DI (Data In):** Conectar al respectivo pin GPIO del RP2040.
- **DE + RE (Driver/Receiver Enable):** Unir y conectar a `3V3` (modo transmisión continua).
- **VCC:** Conectar a `5V` (VBUS USB).
- **GND:** Conectar al `GND` común del circuito.
- **A (+):** Línea positiva DMX (Pin 3 del conector XLR).
- **B (-):** Línea negativa DMX (Pin 2 del conector XLR).
- **XLR Pin 1:** Malla / Blindaje (GND).

---

## Binario Final Precompilado (.UF2)

El binario listo para flashear está disponible en:
- **`binarios_finais/artnet_dmx.uf2`** (o en [GitHub Releases](https://github.com/Matheus-devpixel/PicoArtNetDMXNode/releases))

### Cómo Grabar en el Raspberry Pi RP2040:
1. Mantenga presionado el botón **BOOTSEL** en la placa del Pico.
2. Conecte el cable USB al ordenador y suelte el botón. Aparecerá la unidad `RPI-RP2`.
3. Arrastre y suelte el archivo `artnet_dmx.uf2` dentro de `RPI-RP2`.
4. La placa se reiniciará inmediatamente lista y reconocida en la red en la IP `10.10.0.1` (Setup) o `2.0.N.1` (Operativo).

---

## Compilación desde el Código Fuente

Para recompilar el firmware a partir del código en `src/`:
- Ejecute: `python compile_targets.py`
- El script compila mediante Ninja, convierte el ejecutable a formato `.uf2` oficial y actualiza la carpeta `binarios_finais/`.

---

## Autor y Desarrollador

- **Desarrollador:** Matheus Dev
- **GitHub:** [@Matheus-devpixel](https://github.com/Matheus-devpixel)

---

## Herramienta de Prueba de Transmisión (ArtNet_Test.py)

El repositorio incluye un script de prueba de alto rendimiento en Python ([`ArtNet_Test.py`](../../ArtNet_Test.py)) capaz de emitir hasta 4.096 canales DMX continuos (8 universos) a 60 FPS sin fluctuaciones:

```bash
# Simulación escénica estándar (cabezas móviles, barras LED, wash RGB, estrobos)
python ArtNet_Test.py

# Especificando IP destino y tasa de cuadros (ej. 60 FPS en 2.0.1.1)
python ArtNet_Test.py 60 2.0.1.1

# Modo estroboscópico sincronizado en los 4.096 canales
python ArtNet_Test.py --ip=2.0.1.1 --mode=strobe

# Modo gradiente de color RGB continuo
python ArtNet_Test.py --ip=2.0.1.1 --mode=rgb
```

---

## Créditos y Agradecimientos

Este proyecto utiliza y reconoce los trabajos de código abierto de la comunidad:

1. **Renzo Mischianti ([www.mischiant.org](https://www.mischiant.org)):** Diagramas de pines de alta resolución para Raspberry Pi Pico y RP2040-Zero.
2. **Sergey Fetisov ([fetisov/lrndis](https://github.com/fetisov/lrndis)):** Pila RNDIS ligera y servidores DHCP/DNS para lwIP (Licencia MIT).
3. **Colin O'Flynn (NewAE Technology / LUFA):** Definiciones de cabecera del protocolo RNDIS / NDIS (Licencia BSD-3-Clause).
4. **Ha Thach ([tinyusb.org](https://github.com/hathach/tinyusb)):** TinyUSB Device Stack (Licencia MIT).
5. **Raspberry Pi Ltd.:** Pico C/C++ SDK, PIO y compilador pioasm (Licencia BSD-3-Clause).
6. **Adam Dunkels / SICS:** Pila TCP/IP lwIP para sistemas embebidos (Licencia BSD).

---

## Licencia

Distribuido bajo la **Licencia MIT**, permitiendo uso libre personal, educativo y comercial.
