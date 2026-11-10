/**
 * ============================================================================
 * PROJETO: PICO ARTNET DMX NODE (RP2040)
 * ARQUIVO: artnet_dmx.c
 * AUTOR: Matheus Dev (@Matheus-devpixel)
 * DESCRIÇÃO: Ponto de entrada principal do sistema (main) e coordenação multicore
 *            no microcontrolador Raspberry Pi RP2040.
 *
 * ARQUITETURA DUAL-CORE EXCLUSIVA RP2040:
 *   - CORE 0:
 *     * Pilha de rede USB Ethernet (Microsoft RNDIS / CDC-ECM via TinyUSB).
 *     * Pilha TCP/IP lwIP (gerenciamento de buffers de rede e conexões).
 *     * Servidor DHCP integrado (atribuição automática de IP ao computador).
 *     * Servidor DNS para Captive Portal em modo de configuração.
 *     * Receptor de pacotes Art-Net UDP (Porta 6454) para ArtPoll, ArtPollReply,
 *       ArtDmx e ArtSync.
 *     * Servidor Web HTTP na porta 80 com API REST, SSE e monitoramento em tempo real.
 *     * Atualização do display gráfico OLED SSD1306 e gerenciador de Watchdog.
 *
 *   - CORE 1:
 *     * Motor de transmissão DMX-512 isolado em hardware.
 *     * Geração com precisão de microssegundos dos pulsos BREAK (140 us) e MAB (16 us).
 *     * Transmissão paralela multicanal através das máquinas de estado PIO0/PIO1
 *       e canais de DMA de alta velocidade com zero sobrecarga de CPU.
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "hardware/gpio.h"

#include "globais.h"
#include "storage.h"
#include "lang.h"
#include "dmx.h"
#include "ssd1306.h"
#include "rede.h"
#include "tusb.h"

/**
 * @brief Interceptador de falha grave de hardware (Cortex-M0+ HardFault Handler).
 * 
 * Em caso de ponteiro nulo, acesso a endereço de memória desalinhado ou violação
 * de barramento, grava a assinatura 0xDEAD0001 no registrador de hardware scratch[4]
 * do Watchdog e retém a CPU em loop fechado para permitir que o Watchdog acione
 * o reset automático e seguro do microcontrolador.
 */
void isr_hardfault(void) {
    watchdog_hw->scratch[4] = 0xDEAD0001;
    while (true) {
        tight_loop_contents();
    }
}

/**
 * @brief Função principal e ponto de entrada da aplicação (executada no CORE 0).
 * 
 * Executa as 10 etapas de inicialização ordenada do sistema:
 *   1. Inicialização de E/S padrão (STDIO) e verificação da causa do último reset.
 *   2. Leitura e validação dos dados de configuração persistidos na Flash (storage_init).
 *   3. Geração do endereço MAC de hardware e inicialização do dispositivo TinyUSB.
 *   4. Inicialização dos pinos de LED de diagnóstico da placa.
 *   5. Janela inicial para enumeração USB pelo sistema operacional do host.
 *   6. Detecção e inicialização do display OLED SSD1306 no barramento I2C0.
 *   7. Inicialização da pilha de rede lwIP, sockets UDP Art-Net e servidor HTTP.
 *   8. Configuração das máquinas de estado PIO0/PIO1 e canais DMA para as saídas DMX.
 *   9. Lançamento do motor de transmissão DMX bare-metal no CORE 1.
 *  10. Ativação do hardware Watchdog com timeout de 8 segundos.
 * 
 * Em seguida, entra no loop infinito de eventos do CORE 0, gerenciando a rede,
 * o servidor web, cálculos de telemetria FPS/PPS e a atualização do OLED.
 * 
 * @return int Retorno padrão (a execução nunca atinge este ponto).
 */
int main(void) {
    // 1. Inicializacao imediata de E/S, Watchdog tick e Flash Storage
    stdio_init_all();
    watchdog_start_tick(12);

    if (watchdog_caused_reboot()) {
        printf("[WATCHDOG] Reiniciado pelo Watchdog! Ultimo estagio scratch[4]: 0x%08lX\n",
               (unsigned long)watchdog_hw->scratch[4]);
    } else {
        printf("[SISTEMA] Inicializacao a frio limpa (Power-On Reset ou Reset manual)\n");
    }

    storage_init();

    // 2. Geracao de MAC unico por hardware (diferenciado entre Setup e Operacional)
    rede_init_mac();
    tud_init(BOARD_TUD_RHPORT);

    // 3. Inicializacao do hardware de LED da placa (Pico W e Pico comum)
    board_led_init();
    board_led_put(true); // Acende no boot para confirmar que o LED funciona

    // 4. Janela inicial para processamento de enumeracao USB pelo computador
    for (int i = 0; i < 50; i++) {
        tud_task();
        sleep_ms(5);
    }
    board_led_put(false); // Apaga apos a enumeracao inicial

    // 5. Inicializacao do Display OLED SSD1306 e aplicacao da orientacao salva
    ssd1306_init();
    ssd1306_set_rotation(g_config.rotate180 != 0);
    if (ssd1306_is_detected()) {
        if (!g_config.configured) {
            ssd1306_show_setup_mode("10.10.0.1");
        } else {
            const oled_lang_strings_t *l = lang_get_oled(g_config.language);
            ssd1306_show_message("PICO ARTNET", l->starting_msg);
        }
    }

    // 6. Inicializacao da Rede USB Ethernet no CORE 0
    printf("Inicializando rede Ethernet over USB e pilha Art-Net no CORE 0...\n");
    rede_init();

    // 7. Inicializacao dos PIOs e saídas DMX (até 8 universos dinamicos)
    dmx_init(g_config.def_universos);

    // 8. DISPARA O MOTOR DMX NO CORE 1 (100% isolado por hardware PIO+DMA)
    printf("Lancando transmissor DMX multicore no CORE 1...\n");
    multicore_launch_core1(dmx_core1_entry);
    storage_set_core1_active(true);

    // 9. Ativa Watchdog com timeout de 8 segundos apenas com o sistema operacional
    watchdog_enable(8000, 1);

    uint32_t last_display_update = 0;
    bool status_icon_state = true;

    // LOOP PRINCIPAL (CORE 0: Gerencia Rede, WebServer e Interface)
    static bool s_oled_is_dimmed = false;
    static bool s_blackout_applied = false;
    static bool s_last_manual_test_state = false;

    while (true) {
        watchdog_hw->scratch[4] = 0x01; // Marcador: inicio do ciclo
        watchdog_update();

        watchdog_hw->scratch[4] = 0x02; // Marcador: rede_poll
        rede_poll();

        uint32_t now = to_ms_since_boot(get_absolute_time());

        // ====================================================================
        // CASO A: MODO DE CONFIGURACAO INICIAL (IP 10.10.0.1)
        // O nó aguarda o usuário acessar http://10.10.0.1 para configurar
        // identidade, universos e idioma.
        // ====================================================================
        if (!g_config.configured) {
            if (now - last_display_update >= 1000) {
                last_display_update = now;
                status_icon_state = !status_icon_state;
                board_led_put(status_icon_state);
                if (ssd1306_is_detected()) {
                    watchdog_hw->scratch[4] = 0x04; // Marcador: oled_setup
                    ssd1306_show_setup_mode(rede_get_ip_str());
                }
            }
        }
        // ====================================================================
        // CASO B: MODO OPERACIONAL ATIVO (IP 2.0.N.1 - NÓS 1 A 16)
        // Nó em produção executando recepção Art-Net e envio DMX em tempo real.
        // ====================================================================
        else {
            // Critério de atividade estrito: Apenas declara "SEM DADOS" / ocioso após EXATOS 5 SEGUNDOS (5.000 ms)
            // consecutivos sem receber pacotes Art-Net.
            bool has_recent_traffic = (g_artnet_packets_total > 0) && (now - g_artnet_last_packet_time < 5000);

            // 1. Transição imediata de brilho da tela OLED:
            // Se chegaram dados Art-Net e a tela estava esmaecida, restaura o brilho total (0xFF)
            if (has_recent_traffic && s_oled_is_dimmed) {
                ssd1306_set_dim(false);
                s_oled_is_dimmed = false;
                if (ssd1306_is_detected()) {
                    watchdog_hw->scratch[4] = 0x05; // Marcador: oled_restore_bright
                    ssd1306_show_status(
                        g_config.node_name,
                        rede_get_ip_str(),
                        g_config.def_universos,
                        "USB",
                        status_icon_state,
                        g_artnet_packets_total,
                        g_artnet_fps > 0 ? g_artnet_fps : 1,
                        g_artnet_last_universe,
                        g_artnet_ch1,
                        g_artnet_ch2,
                        g_artnet_ch3
                    );
                }
            }
            // Se passar 5 segundos inteiros sem receber dados Art-Net, esmaece a tela (brilho suave anti burn-in)
            else if (!has_recent_traffic && !s_oled_is_dimmed) {
                ssd1306_set_dim(true);
                s_oled_is_dimmed = true;
                if (ssd1306_is_detected()) {
                    watchdog_hw->scratch[4] = 0x06; // Marcador: oled_dim
                    ssd1306_show_status(
                        g_config.node_name,
                        rede_get_ip_str(),
                        g_config.def_universos,
                        "USB",
                        status_icon_state,
                        g_artnet_packets_total,
                        0, // Sem FPS quando ocioso (mostra "ArtNet: SEM DADOS" após 5s)
                        g_artnet_last_universe,
                        g_artnet_ch1,
                        g_artnet_ch2,
                        g_artnet_ch3
                    );
                }
            }

            // 2. Política de Perda de Sinal DMX (Signal Loss Policy):
            // - Modo 0 (Hold Last Look): Mantém o último estado transmitido.
            // - Modo 1 (Blackout): Se ficar sem pacotes por >= 5000ms, comuta todas as saídas para zero.
            if (g_config.signal_loss_mode == 1) {
                if (now - g_artnet_last_packet_time >= 5000) {
                    if (!s_blackout_applied) {
                        dmx_blackout_all();
                        s_blackout_applied = true;
                    }
                    g_dmx_blackout_active = true;
                } else {
                    s_blackout_applied = false;
                    g_dmx_blackout_active = false;
                }
            } else {
                s_blackout_applied = false;
                g_dmx_blackout_active = false;
            }

            // Atualização imediata do OLED ao alternar entrada/saída do modo de teste manual
            if (g_manual_test_active != s_last_manual_test_state) {
                s_last_manual_test_state = g_manual_test_active;
                last_display_update = 0; // Força atualização imediata
            }

            // 3. Atualização periódica da telemetria e OLED:
            // Taxa: 1 segundo durante tráfego ativo (para FPS em tempo real) ou 3 segundos quando ocioso (economia I2C).
            uint32_t oled_refresh_interval = has_recent_traffic ? 1000 : 3000;
            if (now - last_display_update >= oled_refresh_interval) {
                last_display_update = now;

                // Calcula taxa real de quadros por segundo (FPS) por universo DMX ativo
                uint32_t active_count = 0;
                for (int i = 0; i < 8; i++) {
                    if (g_artnet_active_universes_mask & (1 << i)) active_count++;
                }
                if (active_count == 0) active_count = g_config.def_universos > 0 ? g_config.def_universos : 1;
                g_artnet_active_universes_mask = 0;

                g_artnet_pps = g_artnet_packets_sec;
                uint32_t current_sec_fps = g_artnet_packets_sec / active_count;
                if (current_sec_fps == 0 && g_artnet_packets_sec > 0) {
                    current_sec_fps = 1;
                }
                g_artnet_packets_sec = 0;

                static uint32_t s_last_valid_fps = 0;
                if (current_sec_fps > 0) {
                    s_last_valid_fps = current_sec_fps;
                    g_artnet_fps = current_sec_fps;
                }

                // Critério de 5 segundos para estado "SEM DADOS":
                // Enquanto estiver dentro da janela de 5 segundos, o status permanece ATIVO.
                // Apenas após 5 segundos inteiros sem pacotes o status é declarado inativo (SEM DADOS).
                if (has_recent_traffic) {
                    g_artnet_active = true;
                    if (g_artnet_fps == 0) {
                        g_artnet_fps = s_last_valid_fps > 0 ? s_last_valid_fps : 1;
                    }
                } else {
                    g_artnet_active = false;
                    g_artnet_fps = 0;
                    g_artnet_pps = 0;
                    s_last_valid_fps = 0;
                }

                if (!rede_is_connected()) {
                    status_icon_state = !status_icon_state;
                } else {
                    status_icon_state = true;
                }

                if (ssd1306_is_detected()) {
                    watchdog_hw->scratch[4] = 0x07; // Marcador: oled_periodic
                    ssd1306_show_status(
                        g_config.node_name,
                        rede_get_ip_str(),
                        g_config.def_universos,
                        "USB",
                        status_icon_state,
                        g_artnet_packets_total,
                        g_artnet_fps, // > 0 durante os 5s (mostra "ArtNet: XX FPS"); 0 após 5s ("ArtNet: SEM DADOS")
                        g_artnet_last_universe,
                        g_artnet_ch1,
                        g_artnet_ch2,
                        g_artnet_ch3
                    );
                }
            }
        }
    }

    return 0;
}
