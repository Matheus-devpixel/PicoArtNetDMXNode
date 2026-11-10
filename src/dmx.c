/**
 * ============================================================================
 * PROJETO: PICO ARTNET DMX NODE (RP2040)
 * ARQUIVO: dmx.c
 * AUTOR: Matheus Dev (@Matheus-devpixel)
 * DESCRIÇÃO: Implementação bare-metal do motor de transmissão DMX-512 utilizando
 *            as máquinas de estado PIO0/PIO1 e canais DMA do RP2040.
 *
 * DETALHES DE TEMPORIZAÇÃO E PROTOCOLO FÍSICO DMX512-A:
 *   - Taxa de transmissão: 250.000 baud (4.0 us por bit).
 *   - Formato de cada slot: 1 Start bit (LOW), 8 bits de dados (LSB primeiro), 2 Stop bits (HIGH) = 11 bits (44 us).
 *   - Pulso BREAK: Nível LOW forçado por software por 140 us (exigência do padrão: mínimo 92 us).
 *   - Pulso MAB (Mark After Break): Nível HIGH forçado por 16 us (exigência do padrão: mínimo 12 us).
 *   - Byte 0 (Start Code): Sempre 0x00 para dados dimmer padrão de iluminação.
 *   - Bytes 1 a 512: Intensidades dos canais DMX (0 a 255).
 *   - Duração física total do frame: 140 us (Break) + 16 us (MAB) + (513 * 44 us) = 22.728 ms (~44.0 Hz).
 * ============================================================================
 */

#include "dmx.h"
#include "dmx.pio.h"
#include "storage.h"
#include "globais.h"
#include "rede.h"
#include "pico/multicore.h"
#include <string.h>
#include <stdio.h>

/** Mapeamento contíguo dos pinos GPIO utilizados para as saídas DMX 1 a 8. */
static const uint8_t UNIVERSE_PINS[MAX_UNIVERSES] = {0, 1, 2, 3, 4, 5, 6, 7};

/** Estruturas de controle de cada um dos 8 universos DMX. */
dmx_universe_t g_universes[MAX_UNIVERSES];

/** Flag que sinaliza estado de blackout forçado pelo timeout de sinal Art-Net. */
volatile bool g_dmx_blackout_active = false;

/** Quantidade de saídas DMX atualmente configuradas e ativas (1 a 8). */
static int g_num_universos = 1;

/** Offset de memória de instrução do programa DMX carregado no bloco PIO0. */
static uint g_pio0_offset = 0;

/** Offset de memória de instrução do programa DMX carregado no bloco PIO1. */
static uint g_pio1_offset = 0;

/** Flag indicando se o microcódigo DMX já foi carregado no PIO0. */
static bool g_pio0_loaded = false;

/** Flag indicando se o microcódigo DMX já foi carregado no PIO1. */
static bool g_pio1_loaded = false;

/**
 * @brief Inicializa os blocos PIO, State Machines, canais DMA e buffers DMX.
 * 
 * @param num_universos Quantidade de universos ativos a habilitar (1 a 8).
 * 
 * Distribui as State Machines entre PIO0 (até 4) e PIO1 (restantes), garantindo
 * alocação sem contenção de recursos. Para cada universo ativo, aloca e configura
 * um canal DMA de 8 bits associado ao DREQ de transmissão do PIO correspondente.
 * As portas GPIO excedentes são configuradas em alta impedância (GPIO_IN sem resistores).
 */
void dmx_init(int num_universos) {
    if (num_universos < 1) num_universos = 1;
    if (num_universos > MAX_UNIVERSES) num_universos = MAX_UNIVERSES;
    g_num_universos = num_universos;

    // Carrega o programa PIO DMX nos blocos PIO apenas se houver espaco
    if (!g_pio0_loaded && pio_can_add_program(pio0, &dmx_out_program)) {
        g_pio0_offset = pio_add_program(pio0, &dmx_out_program);
        g_pio0_loaded = true;
    }
    if (!g_pio1_loaded && pio_can_add_program(pio1, &dmx_out_program)) {
        g_pio1_offset = pio_add_program(pio1, &dmx_out_program);
        g_pio1_loaded = true;
    }

    for (int i = 0; i < MAX_UNIVERSES; i++) {
        dmx_universe_t *u = &g_universes[i];
        memset(u, 0, sizeof(dmx_universe_t));
        u->pin = UNIVERSE_PINS[i];
        u->tx_buffers[0][0] = 0x00; // Start Code DMX (sempre 0)
        u->tx_buffers[1][0] = 0x00; // Start Code DMX (sempre 0)
        u->active_buf = 0;
        u->write_buf = 1;
        u->has_new_frame = false;
        u->active = false;
        u->new_data = false;

        if (i < g_num_universos) {
            PIO pio_target = NULL;
            uint offset = 0;
            int sm = -1;

            // Distribuição: tenta primeiro pio0 se for universo 0..3, ou pio1 se for 4..7
            if (i < 4 && g_pio0_loaded) {
                sm = pio_claim_unused_sm(pio0, false);
                if (sm >= 0) {
                    pio_target = pio0;
                    offset = g_pio0_offset;
                }
            }
            if (sm < 0 && g_pio1_loaded) {
                sm = pio_claim_unused_sm(pio1, false);
                if (sm >= 0) {
                    pio_target = pio1;
                    offset = g_pio1_offset;
                }
            }
            if (sm < 0 && g_pio0_loaded) {
                sm = pio_claim_unused_sm(pio0, false);
                if (sm >= 0) {
                    pio_target = pio0;
                    offset = g_pio0_offset;
                }
            }

            if (sm < 0 || pio_target == NULL) {
                printf("[DMX] Aviso: Nao foi possivel alocar PIO SM para universo %d\n", i + 1);
                u->active = false;
                continue;
            }

            u->active = true;
            u->pio = pio_target;
            u->sm = (uint)sm;
            dmx_out_program_init(u->pio, u->sm, offset, u->pin);

            // Aloca canal de DMA dedicado para transferências autônomas de memória
            int dma_ch = dma_claim_unused_channel(false);
            if (dma_ch < 0) {
                printf("[DMX] Aviso: Sem canal DMA para universo %d\n", i + 1);
                pio_sm_unclaim(u->pio, u->sm);
                u->active = false;
                continue;
            }

            u->dma_chan = (uint)dma_ch;
            dma_channel_config c = dma_channel_get_default_config(u->dma_chan);
            channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
            channel_config_set_read_increment(&c, true);
            channel_config_set_write_increment(&c, false);
            channel_config_set_dreq(&c, pio_get_dreq(u->pio, u->sm, true));

            dma_channel_configure(
                u->dma_chan,
                &c,
                &u->pio->txf[u->sm],    // Destino: FIFO de transmissão da State Machine do PIO
                u->tx_buffers[0],       // Origem: Buffer 0 com Start Code + 512 canais
                DMX_FRAME_SIZE,         // 513 bytes
                false                   // Não inicia imediatamente (disparo sincronizado no Core 1)
            );
        } else {
            // Desativa completamente o pino GPIO não utilizado (alta impedância / entrada sem resistores)
            gpio_init(u->pin);
            gpio_set_dir(u->pin, GPIO_IN);
            gpio_disable_pulls(u->pin);
            u->active = false;
        }
    }
}

/**
 * @brief Atualiza os valores dos canais de um universo DMX com dados de um pacote ArtDmx.
 * 
 * @param universe Índice local do universo (0 a g_num_universos - 1).
 * @param data Array de bytes com os valores DMX recebidos pela rede.
 * @param length Quantidade de canais recebidos (até 512).
 * @param is_sync Se true, os dados são retidos no buffer temporário (staging) até a chegada do OpSync.
 */
void dmx_set_universe_data(uint16_t universe, const uint8_t *data, uint16_t length, bool is_sync) {
    if (universe >= (uint16_t)g_num_universos || universe >= MAX_UNIVERSES) {
        return; // Ignora qualquer universo fora da quantidade ativa configurada
    }

    dmx_universe_t *u = &g_universes[universe];
    if (length > UNIVERSE_CHANNELS) length = UNIVERSE_CHANNELS;

    if (is_sync) {
        // ArtSync ativo: grava no staging buffer, aguardando OpSync para comitar simultaneamente
        memcpy(u->incoming_staging, data, length);
        if (length < UNIVERSE_CHANNELS) {
            memset(&u->incoming_staging[length], 0, UNIVERSE_CHANNELS - length);
        }
    } else {
        // Modo imediato (sem ArtSync): grava no buffer de escrita (write_buf)
        uint8_t wb = u->write_buf;
        memcpy(&u->tx_buffers[wb][1], data, length);
        if (length < UNIVERSE_CHANNELS) {
            memset(&u->tx_buffers[wb][1 + length], 0, UNIVERSE_CHANNELS - length);
        }
        u->has_new_frame = true;
    }
    u->new_data = true;
}

/**
 * @brief Transfere todos os dados do staging buffer para os buffers de saída ping-pong.
 * 
 * Chamado pelo manipulador Art-Net ao receber um pacote OpSync. Todos os universos
 * que acumularam dados síncronos recebem flag de swap imediato para o próximo ciclo DMX.
 */
void dmx_commit_artsync(void) {
    for (int i = 0; i < g_num_universos; i++) {
        dmx_universe_t *u = &g_universes[i];
        if (u->active) {
            uint8_t wb = u->write_buf;
            memcpy(&u->tx_buffers[wb][1], u->incoming_staging, UNIVERSE_CHANNELS);
            u->has_new_frame = true;
            u->new_data = true;
        }
    }
}

/**
 * @brief Altera diretamente o valor de um único canal DMX (para o DMX Channel Tester).
 * 
 * @param universe Índice do universo (0 a 7).
 * @param channel Número do canal (1 a 512).
 * @param value Valor do canal (0 a 255).
 */
void dmx_set_channel(uint16_t universe, uint16_t channel, uint8_t value) {
    if (universe >= (uint16_t)g_num_universos || universe >= MAX_UNIVERSES) return;
    if (channel < 1 || channel > UNIVERSE_CHANNELS) return;
    dmx_universe_t *u = &g_universes[universe];
    u->tx_buffers[0][channel] = value;
    u->tx_buffers[1][channel] = value;
    u->incoming_staging[channel - 1] = value;
    u->has_new_frame = true;
    u->new_data = true;
}

/**
 * @brief Ajusta todos os 512 canais de um universo para um mesmo valor (ex: teste rápido / fader geral).
 * 
 * @param universe Índice do universo (0 a 7).
 * @param value Valor DMX aplicado a todos os canais (0 a 255).
 */
void dmx_set_all_channels(uint16_t universe, uint8_t value) {
    if (universe >= (uint16_t)g_num_universos || universe >= MAX_UNIVERSES) return;
    dmx_universe_t *u = &g_universes[universe];
    memset(&u->tx_buffers[0][1], value, UNIVERSE_CHANNELS);
    memset(&u->tx_buffers[1][1], value, UNIVERSE_CHANNELS);
    memset(u->incoming_staging, value, UNIVERSE_CHANNELS);
    u->has_new_frame = true;
    u->new_data = true;
}

/**
 * @brief Zera imediatamente todos os buffers de todos os universos ativos (Blackout total).
 */
void dmx_blackout_all(void) {
    for (int i = 0; i < g_num_universos; i++) {
        dmx_universe_t *u = &g_universes[i];
        memset(&u->tx_buffers[0][1], 0, UNIVERSE_CHANNELS);
        memset(&u->tx_buffers[1][1], 0, UNIVERSE_CHANNELS);
        memset(u->incoming_staging, 0, UNIVERSE_CHANNELS);
        u->has_new_frame = true;
    }
}

/**
 * @brief Obtém acesso ao buffer de transmissão atual de um universo.
 * 
 * @param universe Índice do universo (0 a 7).
 * @return const uint8_t* Ponteiro para o buffer ativo de 513 bytes.
 */
const uint8_t *dmx_get_active_buffer(uint16_t universe) {
    if (universe >= MAX_UNIVERSES) return NULL;
    dmx_universe_t *u = &g_universes[universe];
    return u->tx_buffers[u->active_buf];
}

/**
 * @brief Orquestra o ciclo completo de transmissão DMX em paralelo para todos os universos ativos.
 * 
 * Etapas rigorosas do protocolo DMX512-A executadas a cada ciclo:
 *   1. Sincronização: Aguarda o término da transferência DMA anterior e o esvaziamento das FIFOs do PIO.
 *   2. Temporização: Aplica tempo de repouso configurado (taxa 44Hz, 33Hz ou 25Hz).
 *   3. Troca de buffer: Swap atômico ping-pong nos universos que receberam novos frames ArtDmx.
 *   4. Pulso BREAK: Comuta pinos GPIO para modo software (SIO) e mantém nível LOW por 140 us.
 *   5. Pulso MAB: Eleva pinos para nível HIGH por 16 us (Mark After Break).
 *   6. Disparo PIO/DMA: Reconecta pinos aos PIOs e dispara canais de DMA com 513 bytes a 250 kbaud.
 *   7. Sinalização visual: Atualiza o ritmo de piscada do LED de atividade GP10/GP25.
 */
void dmx_transmit_all_parallel(void) {
    // 1. Aguarda todas as transferencias anteriores do DMA terminarem E a FIFO da PIO esvaziar
    for (int i = 0; i < g_num_universos; i++) {
        if (g_universes[i].active) {
            dma_channel_wait_for_finish_blocking(g_universes[i].dma_chan);
            while (!pio_sm_is_tx_fifo_empty(g_universes[i].pio, g_universes[i].sm)) {
                tight_loop_contents();
            }
        }
    }
    // Aguarda o ultimo byte sair completamente pelo shift register da PIO (44 us por byte a 250 kbaud)
    sleep_us(50);

    // Controle de taxa de atualizacao DMX (Refresh Rate)
    // 0 = 44 Hz (padrao, maxima velocidade fisica)
    // 1 = 33 Hz (~7.5 ms de espera adicional)
    // 2 = 25 Hz (~17.3 ms de espera adicional)
    if (g_config.dmx_rate == 1) {
        sleep_ms(7);
        sleep_us(500);
    } else if (g_config.dmx_rate == 2) {
        sleep_ms(17);
        sleep_us(300);
    }

    // 2. Realiza o chaveamento atomico de double-buffer (ping-pong) para os universos com novo frame
    for (int i = 0; i < g_num_universos; i++) {
        dmx_universe_t *u = &g_universes[i];
        if (u->active && u->has_new_frame) {
            uint8_t next_active = u->write_buf;
            u->write_buf = u->active_buf;
            u->active_buf = next_active;
            u->has_new_frame = false;
        }
    }

    // 3. Gera o sinal BREAK simultaneo em todos os universos (140 us em LOW - padrao DMX >= 92us)
    for (int i = 0; i < g_num_universos; i++) {
        if (g_universes[i].active) {
            gpio_set_function(g_universes[i].pin, GPIO_FUNC_SIO);
            gpio_set_dir(g_universes[i].pin, GPIO_OUT);
            gpio_put(g_universes[i].pin, 0);
        }
    }
    sleep_us(140);

    // 4. Gera o sinal MAB (Mark After Break) simultaneo (16 us em HIGH - padrao DMX >= 12us)
    for (int i = 0; i < g_num_universos; i++) {
        if (g_universes[i].active) {
            gpio_put(g_universes[i].pin, 1);
        }
    }
    sleep_us(16);

    // 5. Reconecta os pinos aos blocos PIO, reinicia o SM sincronizado e dispara os canais de DMA
    for (int i = 0; i < g_num_universos; i++) {
        if (g_universes[i].active) {
            pio_sm_restart(g_universes[i].pio, g_universes[i].sm);
            pio_sm_clear_fifos(g_universes[i].pio, g_universes[i].sm);
            pio_sm_exec(g_universes[i].pio, g_universes[i].sm, pio_encode_jmp(g_universes[i].pio == pio0 ? g_pio0_offset : g_pio1_offset));
            pio_gpio_init(g_universes[i].pio, g_universes[i].pin);
            dma_channel_transfer_from_buffer_now(g_universes[i].dma_chan, g_universes[i].tx_buffers[g_universes[i].active_buf], DMX_FRAME_SIZE);
        }
    }

    // 6. Indicador visual de ciclo no LED integrado (Pico W / Pico comum - GP10 / LED_PIN):
    // Indica que a saída física DMX está ligada e transmitindo.
    // - Enquanto houver sinal Art-Net ou teste manual: pisca a cada frame DMX.
    // - Sem dados de rede (Hold Last Look): continua piscando ritmicamente indicando saída DMX ativa.
    // - Se Blackout estiver selecionado na perda de sinal: para de piscar (apaga) após os 5 segundos.
    static bool s_led_state = false;
    static uint8_t s_led_div = 0;
    if (g_dmx_blackout_active && !g_manual_test_active) {
        if (s_led_state) {
            s_led_state = false;
            board_led_put(false);
        }
    } else {
        s_led_div++;
        if (g_artnet_active || g_manual_test_active) {
            s_led_state = !s_led_state;
            board_led_put(s_led_state);
        } else {
            // Em Hold Last Look (sem novos pacotes, mas DMX transmitindo continuamente):
            // Pisca a cada 6 ciclos (~136 ms ligado / 136 ms desligado)
            if (s_led_div >= 6) {
                s_led_div = 0;
                s_led_state = !s_led_state;
                board_led_put(s_led_state);
            }
        }
    }
}

/**
 * @brief Função principal e loop infinito executado no CORE 1 do RP2040.
 * 
 * O Core 1 é 100% dedicado a esta tarefa. Não sofre interrupções de USB ou Ethernet,
 * garantindo estabilidade e jitter de temporização zero na transmissão física DMX.
 */
void dmx_core1_entry(void) {
    // Permite que o Core 0 pause o Core 1 de forma segura durante operacoes de gravacao na Flash
    multicore_lockout_victim_init();

    // Loop de transmissao continua de alta precisao rodando 100% no CORE 1
    // A transmissao paralela de 513 bytes a 250 kbaud leva exatamente 22.57 ms.
    // Com Break (140 us) e MAB (16 us), cada ciclo dura ~22.73 ms = taxa fisica exata de 44.0 Hz!
    while (true) {
        dmx_transmit_all_parallel();
    }
}
