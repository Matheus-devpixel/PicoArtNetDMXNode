<div align="center">

# Pico ArtNet DMX Node (Raspberry Pi Pico RP2040)

**Firmware nativo bare-metal de altíssimo desempenho para controle profissional de iluminação cênica DMX512 via USB Ethernet (RNDIS / CDC-ECM)**

[![Português](https://img.shields.io/badge/Idioma-Portugu%C3%AAs-blue?style=for-the-badge)](#)
[![English](https://img.shields.io/badge/Language-English-green?style=for-the-badge)](docs/translations/README_EN.md)
[![Español](https://img.shields.io/badge/Idioma-Espa%C3%B1ol-yellow?style=for-the-badge)](docs/translations/README_ES.md)
[![Français](https://img.shields.io/badge/Langue-Fran%C3%A7ais-red?style=for-the-badge)](docs/translations/README_FR.md)
[![Deutsch](https://img.shields.io/badge/Sprache-Deutsch-orange?style=for-the-badge)](docs/translations/README_DE.md)
[![Italiano](https://img.shields.io/badge/Lingua-Italiano-teal?style=for-the-badge)](docs/translations/README_IT.md)
[![Русский](https://img.shields.io/badge/%D0%AF%D0%B7%D1%8B%D0%BA-%D0%A0%D1%83%D1%81%D1%81%D0%BA%D0%B8%D0%B9-purple?style=for-the-badge)](docs/translations/README_RU.md)
[![中文](https://img.shields.io/badge/%E8%AF%AD%E8%A8%80-%E4%B8%AD%E6%96%87-lightgrey?style=for-the-badge)](docs/translations/README_ZH.md)

</div>

---

> [!NOTE]
> **Desempenho e Baixa Latência:** Graças à execução bare-metal em C SDK e à conexão direta via cabo USB (RNDIS / CDC-ECM), o nó atinge **latência ultrabaixa determinística (< 1 ms)** com **zero jitter**, superando nós convencionais baseados em Wi-Fi e eliminando qualquer atraso perceptível na iluminação de palco.

## Visão Geral do Projeto

Firmware nativo de altíssimo desempenho para microcontroladores **Raspberry Pi RP2040 (Raspberry Pi Pico, Pico W e Waveshare RP2040-Zero)** com arquitetura **100% dedicada via cabo USB (Ethernet over USB: RNDIS / CDC-ECM)**.

Projetado especificamente para transmissão profissional de iluminação cênica DMX512 via Art-Net UDP (Porta 6454), processando de **1 a 8 universos DMX simultâneos** (até 4.096 canais) em paralelo a 44 Hz cravados sem jitter.

---

## Arquitetura de Rede USB Ethernet (Plug-and-Play)

Conectado a qualquer computador via cabo USB, o RP2040 atua diretamente como uma **placa de rede virtual Ethernet de alta velocidade**:
- **Windows:** Driver nativo Remote NDIS (**RNDIS**) via Microsoft OS 1.0 Descriptors, reconhecido instantaneamente sem instalação de drivers externos ou programas adicionais.
- **macOS / Linux:** Driver nativo **CDC-ECM**.
- **Servidor DHCP Integrado:** O Pico atribui automaticamente o IP do host (computador) na mesma sub-rede, eliminando qualquer necessidade de configuração manual de rede.

### Modos de Operação e Faixa de IPs Segura

Para evitar conflitos com redes Wi-Fi ou corporativas (faixas `192.168.x.x`), o sistema adota faixas dedicadas:

1. **Modo Inicial de Configuração (Setup Wizard):**
   - **IP:** `10.10.0.1` (Host recebe `10.10.0.2`).
   - Ativado automaticamente no primeiro boot ou após restauração de fábrica.
   - O nó abre exclusivamente o portal de configuração para escolha do Nó (Faixa de Universos), Quantidade de Saídas Ativas (1 a 8), Nome Amigável, Idioma, Tema e Taxa DMX.
   
2. **Modo Operacional Multi-Nó (Art-Net Ativo):**
   - **IP:** `2.0.N.1` (onde `N` é o ID do nó de 1 a 16).
   - Suporte nativo a até **16 nós simultâneos** conectados ao mesmo computador ou hub USB (128 universos / 65.536 canais DMX):
     - **Nó 1:** IP `2.0.1.1` -> Universos 0 a 7
     - **Nó 2:** IP `2.0.2.1` -> Universos 8 a 15
     - **Nó 3:** IP `2.0.3.1` -> Universos 16 a 23
     - *(configurável até o Nó 16: IP `2.0.16.1` -> Universos 120 a 127)*

---

## Suporte Multilíngue Completo (8 Idiomas)

O firmware possui suporte integrado aos idiomas:
- **Português (Brasil)** [PT]
- **English** [EN]
- **Español** [ES]
- **Français** [FR]
- **Deutsch** [DE]
- **Italiano** [IT]
- **Русский (Russo)** [RU]
- **中文 (Chinês Simplificado)** [ZH]

A troca de idioma é refletida instantaneamente em todas as páginas web e no display OLED I2C, ficando gravada de forma permanente na memória Flash interna.

---

## Destaques de Performance Bare-Metal C SDK

1. **Separação Real de Cores (Dual-Core):**
   - **CORE 0:** Gerenciamento da pilha USB TinyUSB, pilha TCP/IP lwIP, servidor HTTP (Porta 80), receptor UDP Art-Net (Porta 6454), atualização de tela OLED SSD1306 e monitoramento do Watchdog de hardware.
   - **CORE 1:** Motor DMX512 dedicado, ininterrupto e de prioridade máxima. Gera frames DMX sem latência, jitter ou concorrência com o tráfego de rede.

2. **DMX512 por Hardware (8 State Machines PIO + DMA):**
   - O RP2040 utiliza os 2 blocos PIO (PIO0 e PIO1) com 8 State Machines operando em paralelo.
   - Pulso BREAK de 140 µs e MAB de 16 µs gerados com precisão de hardware a 250 kbaud (8N2).
   - Saída simultânea contínua de até 8 universos a 44 Hz cravados.

3. **Double-Buffering Ping-Pong e ArtSync:**
   - Troca atômica de buffers que impede qualquer efeito de tearing visual nos refletores.
   - Suporte a pacotes ArtSync (OpSync) para disparo sincronizado de todos os universos.

4. **Baixa Latência Determinística em Tempo Real (< 1 ms):**
   - **Comunicação USB Direta:** Elimina o jitter, a latência variável e a perda de pacotes típicas de redes sem fio (Wi-Fi), garantindo tempo de resposta constante.
   - **Execução Bare-Metal Sem OS:** Sem sistema operacional ou trocas de contexto que possam introduzir atrasos; a interrupção de rede processa o pacote Art-Net imediatamente na memória RAM.
   - **Despacho Direto por Hardware (DMA):** O buffer é encaminhado via Direct Memory Access diretamente para as máquinas de estado PIO, permitindo resposta instantânea e sincronismo perfeito com mesas e softwares de luz (GrandMA, Onyx, Resolume, QLC+).

---

## Pinagem de Hardware Padronizada (Pinout)

Pinagem contígua, limpa e estável:

| Função | Pino GPIO | Pino Físico (Pico) | Barramento / Periférico | Observação |
| :--- | :--- | :--- | :--- | :--- |
| **DMX Saída 1** | **GP0** | Pino 1 | PIO0 - SM 0 | Universo Inicial + 0 |
| **DMX Saída 2** | **GP1** | Pino 2 | PIO0 - SM 1 | Universo Inicial + 1 |
| **DMX Saída 3** | **GP2** | Pino 4 | PIO0 - SM 2 | Universo Inicial + 2 |
| **DMX Saída 4** | **GP3** | Pino 5 | PIO0 - SM 3 | Universo Inicial + 3 |
| **DMX Saída 5** | **GP4** | Pino 6 | PIO1 - SM 0 | Universo Inicial + 4 |
| **DMX Saída 6** | **GP5** | Pino 7 | PIO1 - SM 1 | Universo Inicial + 5 |
| **DMX Saída 7** | **GP6** | Pino 9 | PIO1 - SM 2 | Universo Inicial + 6 |
| **DMX Saída 8** | **GP7** | Pino 10 | PIO1 - SM 3 | Universo Inicial + 7 |
| **OLED SDA** | **GP8** | Pino 11 | I2C0 SDA (400 kHz) | Display SSD1306 128x64 |
| **OLED SCL** | **GP9** | Pino 12 | I2C0 SCL (400 kHz) | Display SSD1306 128x64 |
| **LED DMX Ext** | **GP10** | Pino 14 | GPIO Saída Digital | LED externo dedicado de atividade DMX |
| **LED Onboard** | **GP25** | LED da Placa | GPIO Saída Digital | LED integrado do Pico padrão (em paralelo ao GP10) |

*Pinos de saída DMX não configurados são desativados por software em alta impedância (alta proteção elétrica).*

---

## Conexão dos Módulos Transmissores RS-485 (MAX485 / SN75176)

Para cada saída DMX (GP0 a GP7):
- **DI (Data In):** Conectar ao respectivo pino GPIO do RP2040.
- **DE (Driver Enable) + RE (Receiver Enable):** Conectar juntos ao `3V3` (mantém o transceiver em transmissão contínua).
- **VCC:** Conectar ao `5V` (VBUS do cabo USB).
- **GND:** Conectar ao `GND` comum do circuito.
- **A (+):** Linha positiva DMX (Pino 3 do conector XLR).
- **B (-):** Linha negativa DMX (Pino 2 do conector XLR).
- **XLR Pino 1:** Malha / Blindagem (GND).

---

## Binário Final Compilado (.UF2)

O binário final pronto para uso está disponível em:
- **`binarios_finais/artnet_dmx.uf2`** (ou na aba de [Releases Oficiais](https://github.com/Matheus-devpixel/PicoArtNetDMXNode/releases))

### Como Gravar no Raspberry Pi RP2040:
1. Pressione e segure o botão **BOOTSEL** na placa do Pico.
2. Conecte o cabo USB ao computador e solte o botão. Uma unidade chamada `RPI-RP2` será montada no Windows Explorer.
3. Copie ou arraste o arquivo `artnet_dmx.uf2` para dentro de `RPI-RP2`.
4. A placa reiniciará imediatamente pronta e reconhecida como adaptador Ethernet USB no IP `10.10.0.1` (Setup) ou `2.0.N.1` (Operacional)!

---

## Como Recompilar por Linha de Comando

Para recompilar o firmware a qualquer momento a partir do código-fonte em `src/`:
- Execute: `python compile_targets.py`
- O script automatizado compila o firmware com Ninja, converte o binário para o formato `.uf2` oficial do RP2040 e atualiza a pasta `binarios_finais/`.

---

## Autor e Desenvolvedor

- **Desenvolvedor:** Matheus Dev
- **GitHub:** [@Matheus-devpixel](https://github.com/Matheus-devpixel)

---

## Ferramenta de Teste de Transmissão (ArtNet_Test.py)

O repositório inclui um gerador e testador de estresse em Python de alta precisão ([`ArtNet_Test.py`](ArtNet_Test.py)) capaz de transmitir até 4.096 canais DMX simultâneos (8 universos de 512 canais) a 60 FPS cravados sem jitter:

```bash
# Execução padrão (simulação completa de palco com moving heads, RGB wash, pixels e strobes)
python ArtNet_Test.py

# Especificando IP do nó e taxa de quadros (ex: 60 FPS no IP 2.0.1.1)
python ArtNet_Test.py 60 2.0.1.1

# Modo Strobe sincronizado em todos os 4.096 canais
python ArtNet_Test.py --ip=2.0.1.1 --mode=strobe

# Modo Gradiente de cor RGB contínuo
python ArtNet_Test.py --ip=2.0.1.1 --mode=rgb
```

---

## Créditos e Agradecimentos (Credits & Acknowledgements)

Este projeto foi construído sobre trabalhos de código aberto notáveis da comunidade de sistemas embarcados:

1. **Renzo Mischianti ([www.mischiant.org](https://www.mischiant.org)):**
   - Criação e disponibilização dos diagramas gráficos de pinagem de alta resolução do Raspberry Pi Pico / Pico W e Waveshare RP2040-Zero utilizados nas abas de documentação e no visualizador em tela cheia da dashboard.
2. **Sergey Fetisov ([fetisov/lrndis](https://github.com/fetisov/lrndis)):**
   - Implementação de referência leve da pilha Microsoft RNDIS para microcontroladores e dos servidores embarcados DHCP e DNS sobre lwIP (Licença MIT).
3. **Colin O'Flynn (NewAE Technology / LUFA USB Library):**
   - Definições originais e cabeçalhos de constantes do protocolo Microsoft RNDIS / NDIS (Licença BSD-3-Clause).
4. **Ha Thach ([tinyusb.org](https://github.com/hathach/tinyusb)):**
   - Criação e manutenção da biblioteca TinyUSB Device Stack (Licença MIT).
5. **Raspberry Pi Ltd.:**
   - Raspberry Pi Pico C/C++ SDK, arquitetura de E/S programável (PIO) e ferramentas de compilação pioasm (Licença BSD-3-Clause).
6. **Adam Dunkels / Swedish Institute of Computer Science (SICS):**
   - Pilha de protocolos TCP/IP lwIP (Lightweight IP) para sistemas embarcados (Licença BSD).

---

## Licença (License)

Este firmware é distribuído sob a licença **MIT**, permitindo uso livre para fins pessoais, educacionais e comerciais, mantendo os avisos de direitos autorais originais dos componentes de terceiros listados acima.
