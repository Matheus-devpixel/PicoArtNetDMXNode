<div align="center">

# Pico ArtNet DMX Node (Raspberry Pi Pico RP2040)

**适用于舞台灯光控制的高性能原生固件，通过 USB 以太网传输 DMX512 (RNDIS / CDC-ECM)**

[![Português](https://img.shields.io/badge/Idioma-Portugu%C3%AAs-blue?style=for-the-badge)](../../README.md)
[![English](https://img.shields.io/badge/Language-English-green?style=for-the-badge)](README_EN.md)
[![Español](https://img.shields.io/badge/Idioma-Espa%C3%B1ol-yellow?style=for-the-badge)](README_ES.md)
[![Français](https://img.shields.io/badge/Langue-Fran%C3%A7ais-red?style=for-the-badge)](README_FR.md)
[![Deutsch](https://img.shields.io/badge/Sprache-Deutsch-orange?style=for-the-badge)](README_DE.md)
[![Italiano](https://img.shields.io/badge/Lingua-Italiano-teal?style=for-the-badge)](README_IT.md)
[![Русский](https://img.shields.io/badge/%D0%AF%D0%B7%D1%8B%D0%BA-%D0%A0%D1%83%D1%81%D1%81%D0%BA%D0%B8%D0%B9-purple?style=for-the-badge)](README_RU.md)
[![中文](https://img.shields.io/badge/%E8%AF%AD%E8%A8%80-%E4%B8%AD%E6%96%87-lightgrey?style=for-the-badge)](#)

</div>

---

专为 **Raspberry Pi RP2040 (Raspberry Pi Pico, Pico W 以及 Waveshare RP2040-Zero)** 微控制器设计的专业原生固件，具备 **100% 专用的 USB 虚拟以太网架构 (Ethernet over USB: RNDIS / CDC-ECM)**。

专为舞台灯光专业传输打造，基于 Art-Net UDP (端口 6454)，以无抖动 44 Hz 刷新率并行处理 **1 至 8 个 DMX 域 (Universe)**（最多 4,096 个通道）。

---

## USB 以太网网络架构 (即插即用)

通过 USB 线缆连接电脑后，RP2040 会被直接识别为**高速虚拟以太网网卡**：
- **Windows：** 原生 Remote NDIS (**RNDIS**) 驱动程序，基于 Microsoft OS 1.0 描述符，无需安装任何外部软件即可即插即用。
- **macOS / Linux：** 原生 **CDC-ECM** 驱动。
- **内置 DHCP 服务器：** 自动向计算机分配同网段 IP，无需手动配置网络。

### 工作模式与安全 IP 段

1. **初始配置向导模式 (Setup Wizard)：**
   - **IP：** `10.10.0.1`（电脑自动获取 `10.10.0.2`）。
   - 首次启动或恢复出厂设置时自动启用，仅开放 Web 配置面板，用于选择节点号、输出通道数 (1-8)、名称、语言和刷新率。

2. **多节点运行模式 (Art-Net 激活)：**
   - **IP：** `2.0.N.1`（其中 `N` 为 1 至 16 的节点编号）。
   - 原生支持同一台电脑或 USB 集线器接入最多 **16 个节点**（128 个 DMX 域 / 65,536 个通道）。

---

## 完整多语言支持 (8 种语言)

系统内置支持：葡萄牙语 [PT]、英语 [EN]、西班牙语 [ES]、法语 [FR]、德语 [DE]、意大利语 [IT]、俄语 [RU] 和简体中文 [ZH]。

---

## 硬件引脚分配 (Pinout)

| 功能 | GPIO 引脚 | Pico 物理引脚 | 外设 / 总线 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| **DMX 输出 1** | **GP0** | 引脚 1 | PIO0 - SM 0 | 起始域 + 0 |
| **DMX 输出 2** | **GP1** | 引脚 2 | PIO0 - SM 1 | 起始域 + 1 |
| **DMX 输出 3** | **GP2** | 引脚 4 | PIO0 - SM 2 | 起始域 + 2 |
| **DMX 输出 4** | **GP3** | 引脚 5 | PIO0 - SM 3 | 起始域 + 3 |
| **DMX 输出 5** | **GP4** | 引脚 6 | PIO1 - SM 0 | 起始域 + 4 |
| **DMX 输出 6** | **GP5** | 引脚 7 | PIO1 - SM 1 | 起始域 + 5 |
| **DMX 输出 7** | **GP6** | 引脚 9 | PIO1 - SM 2 | 起始域 + 6 |
| **DMX 输出 8** | **GP7** | 引脚 10 | PIO1 - SM 3 | 起始域 + 7 |
| **OLED SDA** | **GP8** | 引脚 11 | I2C0 SDA (400 kHz) | SSD1306 128x64 屏幕 |
| **OLED SCL** | **GP9** | 引脚 12 | I2C0 SCL (400 kHz) | SSD1306 128x64 屏幕 |
| **DMX 外接 LED** | **GP10** | 引脚 14 | 数字输出 GPIO | 外部 DMX 状态指示灯 |
| **板载 LED** | **GP25** | 板载 LED | 数字输出 GPIO | Pico 原生 LED (同步 GP10) |

---

## 编译好的固件 (.UF2)

可直接下载 [`binarios_finais/artnet_dmx.uf2`](../../binarios_finais/artnet_dmx.uf2) 或访问 [GitHub Releases](https://github.com/Matheus-devpixel/Pico-ArtNet-DMX-Node/releases)。
安装时只需按住 **BOOTSEL** 按钮连接 USB 线，将 `.uf2` 文件拖入弹出的 `RPI-RP2` 磁盘即可。

---

## 开发者与开源协议

- **开发者：** Matheus Dev ([@Matheus-devpixel](https://github.com/Matheus-devpixel))
- **开源协议：** **MIT License**
