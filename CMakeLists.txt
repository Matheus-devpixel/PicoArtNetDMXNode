# ============================================================================
# PROJETO: PICO ARTNET DMX NODE (RP2040)
# ARQUIVO: CMakeLists.txt
# AUTOR: Matheus Dev (@Matheus-devpixel)
# DESCRIÇÃO: Script de compilação CMake para o Raspberry Pi Pico SDK.
#            Gera o firmware dedicado de alta performance para USB Ethernet
#            (RNDIS / CDC-ECM) com compilação cruzada ARM Cortex-M0+.
# ============================================================================

cmake_minimum_required(VERSION 3.13)

# Padrões modernos C11 e C++17
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Pré-define o compilador de microcódigo PIOASM do SDK
if (NOT TARGET pioasm)
    add_executable(pioasm IMPORTED GLOBAL)
    set_property(TARGET pioasm PROPERTY IMPORTED_LOCATION "F:/tools/pioasm.exe")
    set(pioasm_FOUND 1 CACHE INTERNAL "")
endif()

set(PICO_NO_PICOTOOL 1 CACHE BOOL "" FORCE)

# Alvo de hardware: Microcontrolador Raspberry Pi Pico RP2040
set(PICO_BOARD pico CACHE STRING "Board type" FORCE)

# Configura o TinyUSB para execução integrada com o Pico SDK
add_compile_definitions(CFG_TUSB_OS=OPT_OS_PICO)

# Importa a infraestrutura do Pico SDK
include(pico_sdk_import.cmake)

project(artnet_dmx C CXX ASM)

# Inicializa as bibliotecas e subsistemas do Pico SDK
pico_sdk_init()

# Lista de fontes C que compõem o firmware universal
set(COMMON_SOURCES
    artnet_dmx.c        # Ponto de entrada (main), controle multicore e rotina watchdog
    lang.c              # Dicionários de internacionalização em 8 idiomas (i18n)
    dmx.c               # Motor de transmissão DMX-512 via PIO e DMA no Core 1
    storage.c           # Persistência na memória Flash interna SPI do RP2040
    ssd1306.c           # Driver gráfico e controle do display OLED I2C 128x64
    rede.c              # Pilha de rede USB RNDIS/CDC-ECM, lwIP, Art-Net e HTTP
    usb_descriptors.c   # Descritores USB padrão e classes de rede TinyUSB
    dhserver.c          # Servidor DHCP IPv4 embarcado
    dnserver.c          # Servidor DNS captive portal para modo setup
    rndis_reports.c     # Processamento do protocolo Microsoft RNDIS para Windows
    img_pinouts.c       # Dados binários WebP dos diagramas de pinagem
)

# Cria o executável do firmware
add_executable(artnet_dmx
    ${COMMON_SOURCES}
)

# Vincula as bibliotecas necessárias de hardware e rede do RP2040
target_link_libraries(artnet_dmx 
    pico_stdlib         # Biblioteca padrão de E/S, delays e temporizadores
    pico_multicore      # Controle do segundo núcleo de processamento (Core 1)
    pico_lwip_nosys     # Pilha TCP/IP lwIP bare-metal (sem sistema operacional)
    tinyusb_device      # Pilha de dispositivo USB TinyUSB
    hardware_i2c        # Controlador mestre de hardware I2C para o OLED
    hardware_dma        # Controlador de canais DMA de alta velocidade para o DMX
    hardware_pio        # Controlador dos blocos de E/S programável PIO0 e PIO1
    hardware_flash      # Funções de apagamento e gravação na Flash NOR SPI interna
    hardware_sync       # Primitivas de sincronização e controle de interrupções
    hardware_watchdog   # Timer de watchdog de hardware contra travamentos
    hardware_clocks     # Medição e configuração dos clocks do sistema
)

# Configuração de saída padrão STDIO (UART física ativada para depuração)
pico_enable_stdio_uart(artnet_dmx 1)
pico_enable_stdio_usb(artnet_dmx 0)

# Metadados do programa
pico_set_program_name(artnet_dmx "pico_artnet_dmx")
pico_set_program_version(artnet_dmx "2.0")

# Compila o arquivo dmx.pio em cabeçalho C (dmx.pio.h) contendo o microcódigo assembly
pico_generate_pio_header(artnet_dmx ${CMAKE_CURRENT_LIST_DIR}/dmx.pio)

# Diretórios de inclusão de cabeçalhos
target_include_directories(artnet_dmx PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${PICO_SDK_PATH}/lib/tinyusb/lib/networking
)

# Gera os artefatos de saída: .bin, .hex, .dis e arquivo de mapa de memória .map
pico_add_extra_outputs(artnet_dmx)
