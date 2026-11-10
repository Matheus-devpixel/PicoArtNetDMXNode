/**
 * ============================================================================
 * PROJETO: PICO ARTNET DMX NODE (RP2040)
 * ARQUIVO: tusb_config.h
 * DESCRIÇÃO: Configurações da pilha de dispositivo USB TinyUSB para o RP2040.
 *            Habilita o modo de adaptador de rede virtual USB Ethernet
 *            compatível com Microsoft RNDIS (Windows) e CDC-ECM (Linux/macOS).
 * ============================================================================
 */

#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "globais.h"
#include "lwipopts.h"

/** Porta RootHub do controlador USB do RP2040 (Porta 0). */
#ifndef BOARD_TUD_RHPORT
  #define BOARD_TUD_RHPORT 0
#endif

/** Modo do controlador USB: Dispositivo Full Speed (12 Mbps padrão USB 2.0). */
#ifndef CFG_TUSB_RHPORT0_MODE
  #define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#endif

#ifndef BOARD_TUD_MAX_SPEED
  #define BOARD_TUD_MAX_SPEED OPT_MODE_DEFAULT_SPEED
#endif

/** Microcontrolador alvo: Raspberry Pi RP2040 Dual ARM Cortex-M0+. */
#ifndef CFG_TUSB_MCU
  #define CFG_TUSB_MCU OPT_MCU_RP2040
#endif

/** Sistema operacional: SDK Bare-metal Raspberry Pi Pico. */
#ifndef CFG_TUSB_OS
  #define CFG_TUSB_OS OPT_OS_PICO
#endif

#ifndef CFG_TUSB_DEBUG
  #define CFG_TUSB_DEBUG 0
#endif

/** Habilita a pilha TinyUSB Device. */
#define CFG_TUD_ENABLED 1
#define CFG_TUD_MAX_SPEED BOARD_TUD_MAX_SPEED

#ifndef CFG_TUSB_MEM_SECTION
  #define CFG_TUSB_MEM_SECTION
#endif

/** Alinhamento em 32 bits (4 bytes) exigido pela controladora USB do RP2040. */
#ifndef CFG_TUSB_MEM_ALIGN
  #define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

/** Tamanho máximo de pacote do Endpoint 0 de controle (64 bytes). */
#ifndef CFG_TUD_ENDPOINT0_SIZE
  #define CFG_TUD_ENDPOINT0_SIZE 64
#endif

/**
 * Seleção da classe de rede USB:
 *   - CFG_TUD_ECM_RNDIS = 1: Habilita adaptador híbrido Microsoft RNDIS (para Windows)
 *     e CDC-ECM (para Linux, macOS e Raspberry Pi OS).
 *   - Plug & Play: O Windows instala o driver de rede NDIS automaticamente.
 */
#define CFG_TUD_CDC           0
#define CFG_TUD_ECM_RNDIS     1
#define CFG_TUD_NCM           0

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */
