/**
 * ============================================================================
 * PROJETO: PICO ARTNET DMX NODE (RP2040)
 * ARQUIVO: globais.h
 * AUTOR: Matheus Dev (@Matheus-devpixel)
 * DESCRIÇÃO: Definições globais de hardware, mapeamento de GPIOs dos microcontroladores
 *            (Raspberry Pi Pico, Pico W e Waveshare RP2040-Zero), macros de logging
 *            e rotinas inline para controle de sinalização por LEDs de status.
 *
 * MAPA DE PINOS PADRÃO (HARDWARE ESTÁVEL E CONTÍGUO):
 *   - GP0 a GP7 : Saídas seriais DMX-512 diferenciais/RS485 (Universos 1 a 8).
 *                 Geradas de forma autônoma pelos módulos PIO0/PIO1 + DMA.
 *   - GP8 (SDA) : Linha de dados do barramento I2C0 para o Display OLED SSD1306.
 *   - GP9 (SCL) : Linha de clock do barramento I2C0 para o Display OLED SSD1306.
 *   - GP10      : LED externo de atividade DMX (Pino físico 14 no Pico / GP10 no Zero).
 *   - GP25      : LED integrado da placa (Pico padrão).
 * ============================================================================
 */

#ifndef GLOBAIS_H
#define GLOBAIS_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"

// ============================================================================
// DEFINIÇÃO DOS PINOS DE HARDWARE
// ============================================================================

/** Pino do LED onboard (utilizado no Raspberry Pi Pico clássico). */
#define LED_PIN     25

/**
 * Pino do LED externo de atividade DMX dedicado (GP10, pino físico 14).
 * Funciona universalmente em Pico clássico, Pico W e Waveshare RP2040-Zero.
 */
#define LED_EXT_PIN 10

/** Instância do barramento I2C utilizada pelo display OLED SSD1306 (I2C0). */
#define OLED_I2C_INST i2c0

/** Pino GPIO conectado à linha SDA (Dados) do display OLED SSD1306. */
#define OLED_SDA_PIN  8

/** Pino GPIO conectado à linha SCL (Clock) do display OLED SSD1306. */
#define OLED_SCL_PIN  9

/** Macro de conveniência para depuração e log no terminal serial USB UART/CDC. */
#define LOG(fmt, ...) printf("[ARTNET] " fmt, ##__VA_ARGS__)

// ============================================================================
// FUNÇÕES INLINE DE CONTROLE DO HARDWARE DE SINALIZAÇÃO
// ============================================================================

/**
 * @brief Inicializa os pinos de GPIO dos LEDs como saídas digitais em nível LOW.
 * 
 * Configura simultaneamente o LED onboard (GP25) e o LED de atividade dedicado (GP10).
 * Ambos iniciam desligados até a validação do boot pelo sistema.
 */
static inline void board_led_init(void) {
    // LED onboard (GP25 no Pico padrão)
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // LED de atividade dedicado GP10 (Pino Físico 14)
    gpio_init(LED_EXT_PIN);
    gpio_set_dir(LED_EXT_PIN, GPIO_OUT);
    gpio_put(LED_EXT_PIN, 0);
}

/**
 * @brief Aciona ou desliga simultaneamente os LEDs de status do dispositivo.
 * 
 * @param on true para ligar os LEDs (nível lógico HIGH); false para desligar (nível LOW).
 * 
 * Durante o ciclo de transmissão DMX, o estado do LED é comutado ritmicamente
 * para fornecer diagnóstico visual imediato de que a porta física DMX está transmitindo.
 */
static inline void board_led_put(bool on) {
    gpio_put(LED_PIN, on ? 1 : 0);
    gpio_put(LED_EXT_PIN, on ? 1 : 0);
}

#endif // GLOBAIS_H
