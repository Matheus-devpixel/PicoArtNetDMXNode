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

> [!NOTE]
> **极致性能与超低延迟：** 得益于纯 C 语言 Bare-Metal 原生架构与 USB 有线直连传输 (RNDIS / CDC-ECM)，本节点实现了 **确定性超低延迟（< 1 ms）** 与 **零抖动 (Zero Jitter)**，彻底杜绝 Wi-Fi 常见丢包与延迟，满足专业舞台灯光无延时控制需求。

## 项目概述

专为 **Raspberry Pi RP2040 (Raspberry Pi Pico, Pico W 以及 Waveshare RP2040-Zero)** 微控制器设计的专业原生固件，具备 **100% 专用的 USB 虚拟以太网架构 (Ethernet over USB: RNDIS / CDC-ECM)**。

专为舞台灯光专业传输打造，基于 Art-Net UDP (端口 6454)，以无抖动 44 Hz 刷新率并行处理 **1 至 8 个 DMX 域 (Universe)**（最多 4,096 个通道）。

---

## USB 以太网网络架构 (即插即用)

通过 USB 线缆连接电脑后，RP2040 会被直接识别为**高速虚拟以太网网卡**：
- **Windows：** 原生 Remote NDIS (**RNDIS**) 驱动程序，基于 Microsoft OS 1.0 描述符，无需安装任何外部软件即可即插即用。
- **macOS / Linux：** 原生 **CDC-ECM** 驱动。
- **内置 DHCP 服务器：** 自动向计算机分配同网段 IP，无需手动配置网络。

### 工作模式与安全 IP 段

为了避免与 Wi-Fi 或局域网 (`192.168.x.x`) 冲突，系统采用独立网段：

1. **初始配置向导模式 (Setup Wizard)：**
   - **IP：** `10.10.0.1`（电脑自动获取 `10.10.0.2`）。
   - 首次启动或恢复出厂设置时自动启用，仅开放 Web 配置面板，用于选择节点号、输出通道数 (1-8)、名称、语言和刷新率。

2. **多节点运行模式 (Art-Net 激活)：**
   - **IP：** `2.0.N.1`（其中 `N` 为 1 至 16 的节点编号）。
   - 原生支持同一台电脑或 USB 集线器接入最多 **16 个节点**（128 个 DMX 域 / 65,536 个通道）：
     - **节点 1：** IP `2.0.1.1` -> DMX 域 0 至 7
     - **节点 2：** IP `2.0.2.1` -> DMX 域 8 至 15
     - **节点 3：** IP `2.0.3.1` -> DMX 域 16 至 23
     - *(最高可配置至节点 16：IP `2.0.16.1` -> DMX 域 120 至 127)*

---

## 完整多语言支持 (8 种语言)

系统内置支持：葡萄牙语 [PT]、英语 [EN]、西班牙语 [ES]、法语 [FR]、德语 [DE]、意大利语 [IT]、俄语 [RU] 和简体中文 [ZH]。语言切换会实时同步更新所有网页与 I2C OLED 显示屏，并永久保存在内部 SPI Flash 中。

---

## Bare-Metal 原生 C SDK 性能优势

1. **双核真正硬件隔离 (Dual-Core)：**
   - **CORE 0：** 运行 TinyUSB、lwIP TCP/IP 协议栈、HTTP Web 服务器（80端口）、Art-Net 接收器（6454端口）、OLED 绘制及看门狗保护。
   - **CORE 1：** 独占高优先级 DMX512 发送引擎，不受网络流量任何干扰，提供真正零抖动、零延迟的 DMX 输出。

2. **硬件 PIO + DMA 引擎 (8 个状态机并行)：**
   - 充分利用 RP2040 的两个 PIO 模块（PIO0 和 PIO1）同时驱动 8 个 State Machine。
   - 硬件精准生成 BREAK（140 µs）与 MAB（16 µs）时序信号（250 kbaud, 8N2）。
   - 8 个 Universe 满载以锁定 44 Hz 连续稳定输出。

3. **原子级 Ping-Pong 双缓冲与 ArtSync：**
   - 指针原子交换技术彻底杜绝画面撕裂（Tearing）。
   - 全面支持 ArtSync（OpSync）同步包，实现跨 Universe 完美同步刷新。

4. **确定性实时超低延迟（< 1 ms）：**
   - **USB 直连传输：** 彻底消除无线 Wi-Fi 网络的时延波动、无线电干扰与丢包。
   - **无操作系统（Bare-Metal）极致响应：** 无任何操作系统任务调度与上下文切换开销，网络中断直接在内存中处理 Art-Net 数据包。
   - **硬件 DMA 直通转发：** 帧缓冲数据通过直接内存访问（DMA）直达 PIO 状态机，瞬间响应频闪、渐变与灯光音乐同步。

---

## 硬件引脚分配 (Pinout)

连续、规整、电气稳定的引脚规划：

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

*未启用的 DMX 引脚会在软件中被置为高阻抗模式（High-Z），提供最大程度的电路保护。*

---

## RS-485 收发器接线说明 (MAX485 / SN75176)

针对每一个 DMX 输出端口 (GP0 至 GP7)：
- **DI (Data In)：** 连接至对应的 RP2040 GPIO 引脚。
- **DE + RE (使能端)：** 短接并连接至 `3V3`（保持常发模式）。
- **VCC：** 连接至 `5V`（USB VBUS）。
- **GND：** 连接至共地 `GND`。
- **A (+)：** DMX 正信号线（XLR 3 号脚）。
- **B (-)：** DMX 负信号线（XLR 2 号脚）。
- **XLR 1 号脚：** 屏蔽网 / 地线 (GND)。

---

## 预编译固件文件 (.UF2)

可直接使用的固件文件位于：
- **`binarios_finais/artnet_dmx.uf2`**（或前往 [GitHub Releases](https://github.com/Matheus-devpixel/PicoArtNetDMXNode/releases) 下载）

### 如何烧录固件到 Raspberry Pi RP2040：
1. 按住 Pico 板上的 **BOOTSEL** 按钮。
2. 插入 USB 数据线并松开按钮，电脑将识别出名为 `RPI-RP2` 的磁盘。
3. 将 `artnet_dmx.uf2` 拖入 `RPI-RP2` 磁盘。
4. 开发板将立即自动重启并被识别为以太网卡，在 `10.10.0.1`（设置模式）或 `2.0.N.1`（运行模式）提供服务。

---

## 从源码编译

如需使用 `src/` 中的源码重新编译：
- 运行：`python compile_targets.py`
- 脚本将调用 Ninja 进行高速交叉编译，生成标准 `.uf2` 文件并刷新 `binarios_finais/` 目录。

---

## 作者与开发信息

- **开发者：** Matheus Dev
- **GitHub：** [@Matheus-devpixel](https://github.com/Matheus-devpixel)

---

## 传输压力测试工具 (ArtNet_Test.py)

本仓库附带了高精度 Python 测试工具 ([`ArtNet_Test.py`](../../ArtNet_Test.py))，可向目标发送最高 4,096 个 DMX 通道（8 个 Universe），锁定 60 FPS 无抖动稳定传输：

```bash
# 默认全功能舞台灯光测试（包含摇头灯、RGB染色、像素灯条与频闪）
python ArtNet_Test.py

# 指定目标 IP 与帧率（例如以 60 FPS 发送至 2.0.1.1）
python ArtNet_Test.py 60 2.0.1.1

# 全通道 4096 路同步高频爆闪测试
python ArtNet_Test.py --ip=2.0.1.1 --mode=strobe

# 全通道 RGB 色轮渐变测试
python ArtNet_Test.py --ip=2.0.1.1 --mode=rgb
```

---

## 致谢与开源项目引用

1. **Renzo Mischianti ([www.mischiant.org](https://www.mischiant.org))：**
   - 制作并分享的高分辨率 Raspberry Pi Pico 与 RP2040-Zero 引脚图。
2. **Sergey Fetisov ([fetisov/lrndis](https://github.com/fetisov/lrndis))：**
   - 轻量级 Microsoft RNDIS 协议栈及适用于 lwIP 的 DHCP/DNS 服务实现（MIT 协议）。
3. **Colin O'Flynn (NewAE Technology / LUFA 库)：**
   - Microsoft RNDIS / NDIS 头文件与常量定义（BSD-3-Clause 协议）。
4. **Ha Thach ([tinyusb.org](https://github.com/hathach/tinyusb))：**
   - TinyUSB Device Stack（MIT 协议）。
5. **Raspberry Pi Ltd.：**
   - Raspberry Pi Pico C/C++ SDK，PIO 架构与 pioasm 汇编器（BSD-3-Clause 协议）。
6. **Adam Dunkels / 瑞典计算机科学研究所 (SICS)：**
   - 用于嵌入式系统的 lwIP TCP/IP 协议栈（BSD 协议）。

---

## 开源协议

本项目基于 **MIT 协议** 授权，允许个人、教育及商业用途自由使用，请保留上述第三方软件的原始版权声明。
