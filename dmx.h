/**
 * ============================================================================
 * PROJETO: PICO ARTNET DMX NODE (RP2040)
 * ARQUIVO: dmx.h
 * AUTOR: Matheus Dev (@Matheus-devpixel)
 * DESCRIÇÃO: Motor de transmissão DMX-512 via hardware RP2040 (PIO + DMA).
 *            Gerenciamento de buffer duplo (ping-pong) por universo,
 *            sincronização ArtSync (OpSync), controle de taxa de atualização
 *            e execução contínua no CORE 1.
 * ============================================================================
 */

#ifndef DMX_H
#define DMX_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

/** Quantidade máxima de universos DMX físicos suportados pelo hardware (GP0 a GP7). */
#define MAX_UNIVERSES 8

/** Quantidade de slots/canais de intensidade por universo DMX (padrão USITT DMX512-A). */
#define UNIVERSE_CHANNELS 512

/** Tamanho do frame transmitido pelo DMA: 1 byte de Start Code (0x00) + 512 canais DMX = 513 bytes. */
#define DMX_FRAME_SIZE (UNIVERSE_CHANNELS + 1)

/**
 * @brief Estrutura de controle de um universo DMX físico.
 * 
 * Contém o mapeamento de pinos, instâncias de PIO e State Machine, canal DMA,
 * buffers de transmissão ping-pong para evitar tearing visual, e staging buffer
 * para o protocolo ArtSync.
 */
typedef struct {
    uint8_t pin;                                 /**< Pino GPIO físico associado a esta saída (GP0 a GP7). */
    PIO pio;                                     /**< Ponteiro para o bloco PIO alocado (pio0 ou pio1). */
    uint sm;                                     /**< Índice da State Machine alocada dentro do bloco PIO (0 a 3). */
    uint dma_chan;                               /**< Canal DMA alocado para transferências diretas de memória para FIFO. */
    uint8_t tx_buffers[2][DMX_FRAME_SIZE];       /**< Double-buffer ping-pong: Start Code (0x00) + 512 canais. */
    volatile uint8_t active_buf;                 /**< Índice do buffer atualmente sob leitura pelo DMA (0 ou 1). */
    volatile uint8_t write_buf;                  /**< Índice do buffer livre para escrita de novos dados pelo Core 0. */
    volatile bool has_new_frame;                 /**< Flag atômica indicando que um novo quadro completo aguarda swap. */
    uint8_t incoming_staging[UNIVERSE_CHANNELS]; /**< Buffer intermediário retido até o recebimento de OpSync. */
    bool active;                                 /**< Indica se este universo físico está habilitado e configurado. */
    bool new_data;                               /**< Flag indicando chegada recente de dados. */
} dmx_universe_t;

/** Array global com os descritores de todos os 8 universos DMX. */
extern dmx_universe_t g_universes[MAX_UNIVERSES];

/** Flag atômica que sinaliza o estado de Blackout forçado por perda de sinal Art-Net. */
extern volatile bool g_dmx_blackout_active;

/**
 * @brief Inicializa o subsistema de hardware DMX (PIOs, State Machines e Canais DMA).
 * 
 * @param num_universos Quantidade de saídas DMX ativas a configurar (1 a 8).
 * 
 * Carrega o microcódigo `dmx.pio` nos blocos PIO0 e PIO1, reserva as State Machines
 * necessárias, associa os pinos GP0..GP(N-1) e aloca canais de DMA de 8 bits.
 * Pinos não utilizados são configurados em alta impedância (alta proteção elétrica).
 */
void dmx_init(int num_universos);

/**
 * @brief Atualiza o buffer de dados de um determinado universo DMX.
 * 
 * @param universe Índice do universo local (0 a num_universos - 1).
 * @param data Ponteiro para o array de canais DMX recebido no pacote ArtDmx.
 * @param length Quantidade de canais recebidos (1 a 512).
 * @param is_sync true se o pacote estiver sob controle do protocolo ArtSync (grava em staging);
 *                false para atualização imediata no buffer ping-pong livre.
 */
void dmx_set_universe_data(uint16_t universe, const uint8_t *data, uint16_t length, bool is_sync);

/**
 * @brief Efetua o commit simultâneo de todos os universos retidos no staging buffer pelo ArtSync.
 * 
 * Chamada imediatamente ao receber um pacote Art-Net OpSync (porta 6454),
 * garantindo que todos os universos sejam atualizados exatamente no mesmo instante.
 */
void dmx_commit_artsync(void);

/**
 * @brief Configura o valor de um canal específico (utilizado pelo DMX Channel Tester).
 * 
 * @param universe Índice do universo (0 a 7).
 * @param channel Número do canal (1 a 512).
 * @param value Valor DMX de intensidade (0 a 255).
 */
void dmx_set_channel(uint16_t universe, uint16_t channel, uint8_t value);

/**
 * @brief Configura todos os 512 canais de um universo para o mesmo valor.
 * 
 * @param universe Índice do universo (0 a 7).
 * @param value Valor DMX a aplicar em todos os canais (0 a 255).
 */
void dmx_set_all_channels(uint16_t universe, uint8_t value);

/**
 * @brief Zera imediatamente todos os 512 canais de todas as saídas físicas ativas (Blackout).
 */
void dmx_blackout_all(void);

/**
 * @brief Retorna o ponteiro para o buffer de transmissão atualmente ativo de um universo.
 * 
 * @param universe Índice do universo (0 a 7).
 * @return const uint8_t* Ponteiro para os 513 bytes (Start Code + 512 canais) ou NULL se inválido.
 */
const uint8_t *dmx_get_active_buffer(uint16_t universe);

/**
 * @brief Transmite simultaneamente um frame DMX completo em paralelo por hardware em todas as saídas ativas.
 * 
 * Executa o ciclo DMX512 padrão:
 *   1. Aguarda conclusão do frame anterior e esvaziamento das FIFOs do PIO.
 *   2. Aplica controle de atraso configurado (44 Hz, 33 Hz ou 25 Hz).
 *   3. Executa a troca atômica de double-buffer (ping-pong).
 *   4. Gera o pulso BREAK (140 microssegundos em nível LOW).
 *   5. Gera o pulso MAB (16 microssegundos em nível HIGH).
 *   6. Dispara os canais de DMA para transferir 513 bytes por universo para os PIOs a 250 kbaud.
 *   7. Comuta ritmicamente os LEDs de sinalização.
 */
void dmx_transmit_all_parallel(void);

/**
 * @brief Ponto de entrada e loop contínuo do transmissor DMX executado no CORE 1 do RP2040.
 * 
 * Inicializa a proteção de lockout multicore (para gravações Flash seguras no Core 0)
 * e entra em loop chamando `dmx_transmit_all_parallel()`.
 */
void dmx_core1_entry(void);

#endif // DMX_H
