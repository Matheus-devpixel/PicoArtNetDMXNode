<div align="center">

# Pico ArtNet DMX Node (Raspberry Pi Pico RP2040)

**Высокопроизводительная bare-metal прошивка для профессионального управления сценическим освещением DMX512 через USB Ethernet (RNDIS / CDC-ECM)**

[![Português](https://img.shields.io/badge/Idioma-Portugu%C3%AAs-blue?style=for-the-badge)](../../README.md)
[![English](https://img.shields.io/badge/Language-English-green?style=for-the-badge)](README_EN.md)
[![Español](https://img.shields.io/badge/Idioma-Espa%C3%B1ol-yellow?style=for-the-badge)](README_ES.md)
[![Français](https://img.shields.io/badge/Langue-Fran%C3%A7ais-red?style=for-the-badge)](README_FR.md)
[![Deutsch](https://img.shields.io/badge/Sprache-Deutsch-orange?style=for-the-badge)](README_DE.md)
[![Italiano](https://img.shields.io/badge/Lingua-Italiano-teal?style=for-the-badge)](README_IT.md)
[![Русский](https://img.shields.io/badge/%D0%AF%D0%B7%D1%8B%D0%BA-%D0%A0%D1%83%D1%81%D1%81%D0%BA%D0%B8%D0%B9-purple?style=for-the-badge)](#)
[![中文](https://img.shields.io/badge/%E8%AF%AD%E8%A8%80-%E4%B8%AD%E6%96%87-lightgrey?style=for-the-badge)](README_ZH.md)

</div>

---

Оригинальная прошивка для микроконтроллеров **Raspberry Pi RP2040 (Raspberry Pi Pico, Pico W и Waveshare RP2040-Zero)** с архитектурой **100% прямого подключения через USB-кабель (Ethernet over USB: RNDIS / CDC-ECM)**.

Предназначена для передачи сценического протокола DMX512 по сети Art-Net UDP (Порт 6454), обрабатывая от **1 до 8 одновременных потоков (юниверсов) DMX** (до 4096 каналов) с фиксированной частотой 44 Гц без джиттера.

---

## Сетевая архитектура USB Ethernet (Plug-and-Play)

При подключении к компьютеру по USB, RP2040 определяется как **высокоскоростной виртуальный сетевой адаптер Ethernet**:
- **Windows:** Встроенный драйвер Remote NDIS (**RNDIS**) через дескрипторы Microsoft OS 1.0 — установка сторонних программ не требуется.
- **macOS / Linux:** Нативный драйвер **CDC-ECM**.
- **Встроенный сервер DHCP:** Автоматическая выдача IP-адреса компьютеру в той же подсети.

### Режимы работы и диапазоны IP

1. **Режим первоначальной настройки (Setup Wizard):**
   - **IP:** `10.10.0.1` (компьютер получает `10.10.0.2`).
   - Запускается при первом старте или сбросе к заводским настройкам для выбора номера узла, числа выходов (1-8), языка и параметров DMX.

2. **Рабочий режим (Art-Net активен):**
   - **IP:** `2.0.N.1` (где `N` — номер узла от 1 до 16).
   - Поддержка до **16 узлов одновременно** (128 юниверсов / 65 536 каналов DMX).

---

## Многоязычный интерфейс (8 языков)

Встроенная поддержка: Португальский [PT], Английский [EN], Испанский [ES], Французский [FR], Немецкий [DE], Итальянский [IT], Русский [RU] и Китайский [ZH].

---

## Назначение выводов (Pinout)

| Функция | Вывод GPIO | Номер вывода (Pico) | Интерфейс | Описание |
| :--- | :--- | :--- | :--- | :--- |
| **DMX Выход 1** | **GP0** | Вывод 1 | PIO0 - SM 0 | Начальный юниверс + 0 |
| **DMX Выход 2** | **GP1** | Вывод 2 | PIO0 - SM 1 | Начальный юниверс + 1 |
| **DMX Выход 3** | **GP2** | Вывод 4 | PIO0 - SM 2 | Начальный юниверс + 2 |
| **DMX Выход 4** | **GP3** | Вывод 5 | PIO0 - SM 3 | Начальный юниверс + 3 |
| **DMX Выход 5** | **GP4** | Вывод 6 | PIO1 - SM 0 | Начальный юниверс + 4 |
| **DMX Выход 6** | **GP5** | Вывод 7 | PIO1 - SM 1 | Начальный юниверс + 5 |
| **DMX Выход 7** | **GP6** | Вывод 9 | PIO1 - SM 2 | Начальный юниверс + 6 |
| **DMX Выход 8** | **GP7** | Вывод 10 | PIO1 - SM 3 | Начальный юниверс + 7 |
| **OLED SDA** | **GP8** | Вывод 11 | I2C0 SDA (400 кГц) | Дисплей SSD1306 128x64 |
| **OLED SCL** | **GP9** | Вывод 12 | I2C0 SCL (400 кГц) | Дисплей SSD1306 128x64 |
| **LED DMX Ext** | **GP10** | Вывод 14 | Цифровой выход | Внешний индикатор DMX |
| **Встроенный LED**| **GP25** | LED на плате | Цифровой выход | Дублирует GP10 |

---

## Готовый бинарный файл (.UF2)

Доступен в папке [`binarios_finais/artnet_dmx.uf2`](../../binarios_finais/artnet_dmx.uf2) или в разделе [GitHub Releases](https://github.com/Matheus-devpixel/Pico-ArtNet-DMX-Node/releases).
Для прошивки зажмите кнопку **BOOTSEL**, подключите USB-кабель и скопируйте `.uf2` файл на появившийся диск `RPI-RP2`.

---

## Автор и Лицензия

- **Разработчик:** Matheus Dev ([@Matheus-devpixel](https://github.com/Matheus-devpixel))
- **Лицензия:** **MIT License**
