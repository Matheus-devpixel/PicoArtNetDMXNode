/**
 * ============================================================================
 * PROJETO: PICO ARTNET DMX NODE (RP2040)
 * ARQUIVO: rede.h
 * AUTOR: Matheus Dev (@Matheus-devpixel)
 * DESCRIÇÃO: Camada de rede Ethernet over USB (RNDIS / CDC-ECM via TinyUSB),
 *            pilha TCP/IP lwIP, receptor UDP Art-Net (Porta 6454), servidor
 *            DHCP, servidor DNS e servidor Web HTTP (Porta 80).
 *
 * ESPECIFICAÇÕES DO PROTOCOLO ART-NET E REDE:
 *   - Porta padrão Art-Net UDP: 6454 (0x1936).
 *   - Porta do Servidor Web HTTP: 80.
 *   - Modos de Operação de IP:
 *     * Modo Setup Inicial: IP estático 10.10.0.1 (Máscara 255.0.0.0).
 *     * Modo Operacional: IP estático 2.0.N.1 (onde N é o node_id de 1 a 16).
 *   - Pacotes Art-Net processados:
 *     * OpPoll (0x2000): Descoberta de nós na rede local.
 *     * OpPollReply (0x2100): Resposta de identificação completa do nó.
 *     * OpDmx (0x5000): Transmissão de pacotes de dados DMX512.
 *     * OpSync (0x5200): Sincronização simultânea de múltiplos universos (ArtSync).
 * ============================================================================
 */

#ifndef REDE_H
#define REDE_H

#include <stdbool.h>
#include <stdint.h>

/** Porta padrão UDP para recepção e transmissão de pacotes do protocolo Art-Net. */
#define ARTNET_PORT 6454

/** Porta padrão TCP para o servidor Web HTTP embarcado. */
#define HTTP_PORT 80

/** Endereço MAC de hardware de 6 bytes utilizado pelo adaptador de rede USB. */
extern uint8_t tud_network_mac_address[6];

// ============================================================================
// TELEMETRIA E ESTATÍSTICAS EM TEMPO REAL DO PROTOCOLO ART-NET
// ============================================================================

/** Total acumulado de pacotes Art-Net válidos recebidos desde o boot. */
extern volatile uint32_t g_artnet_packets_total;

/** Contador temporário de pacotes recebidos no segundo atual (para cálculo de PPS). */
extern volatile uint32_t g_artnet_packets_sec;

/** Taxa de pacotes por segundo (PPS) calculada a cada intervalo de 1 segundo. */
extern volatile uint32_t g_artnet_pps;

/** Taxa real de quadros por segundo (FPS) por universo calculada a cada segundo. */
extern volatile uint32_t g_artnet_fps;

/** Número do último universo DMX que recebeu dados pela rede. */
extern volatile uint16_t g_artnet_last_universe;

/** Valor atual do canal 1 do último universo recebido (para telemetria/diagnóstico). */
extern volatile uint8_t  g_artnet_ch1;

/** Valor atual do canal 2 do último universo recebido. */
extern volatile uint8_t  g_artnet_ch2;

/** Valor atual do canal 3 do último universo recebido. */
extern volatile uint8_t  g_artnet_ch3;

/** Flag indicando se há tráfego Art-Net ativo dentro da janela de 5 segundos. */
extern volatile bool     g_artnet_active;

/** Timestamp (em milissegundos desde o boot) do último pacote Art-Net recebido. */
extern volatile uint32_t g_artnet_last_packet_time;

/** Contador de pacotes DMX perdidos por salto de sequência (ArtDmx Sequence gap). */
extern volatile uint32_t g_artnet_lost_packets;

/** Flag indicando se o modo de sincronização simultânea ArtSync (OpSync) foi detectado. */
extern volatile bool     g_artsync_active;

/** Flag indicando se o modo de teste manual de canais pela web está ativado. */
extern volatile bool     g_manual_test_active;

/** Máscara de bits dos universos que receberam tráfego no segundo atual. */
extern volatile uint8_t  g_artnet_active_universes_mask;

// ============================================================================
// PROTÓTIPOS DE FUNÇÕES DA CAMADA DE REDE
// ============================================================================

/**
 * @brief Gera um endereço MAC de hardware único para a interface de rede USB.
 * 
 * Utiliza o identificador serial único gravado de fábrica no chip flash NOR do RP2040
 * (`pico_get_unique_board_id`), diferenciando o byte final entre o modo Setup e o
 * modo Operacional para evitar que o Windows reutilize leases de DHCP em cache.
 */
void rede_init_mac(void);

/**
 * @brief Inicializa a pilha de rede lwIP, interface de rede virtual USB e serviços.
 * 
 * Atribui o IP correspondente (10.10.0.1 ou 2.0.N.1), inicializa a netif do lwIP,
 * dispara o servidor DHCP embutido (atribuindo 10.10.0.2 ou 2.0.N.2 ao computador),
 * inicia o servidor DNS (apenas em modo Setup) e ativa os servidores UDP Art-Net e HTTP.
 */
void rede_init(void);

/**
 * @brief Rotina periódica de processamento da rede (deve ser chamada no loop do Core 0).
 * 
 * Executa `tud_task()` para processar descritores e pacotes USB RNDIS/CDC-ECM
 * e chama os timers periódicos do lwIP (`sys_check_timeouts()`).
 */
void rede_poll(void);

/**
 * @brief Reinicia com segurança o microcontrolador via Watchdog por software.
 */
void rede_reboot_system(void);

/**
 * @brief Informa se o computador host estabeleceu enlace de rede USB ativo.
 * @return true se o enlace USB estiver ativo e enumerado; false caso desconectado.
 */
bool rede_is_connected(void);

/**
 * @brief Informa se o sistema está operando em modo USB Ethernet (sempre true no firmware dedicado).
 * @return true.
 */
bool rede_is_usb_mode(void);

/**
 * @brief Retorna uma string com o endereço IP ativo do nó (ex: "10.10.0.1" ou "2.0.1.1").
 * @return const char* Ponteiro para a string formatada em memória estática.
 */
const char *rede_get_ip_str(void);

/**
 * @brief Inicializa a escuta de pacotes UDP na porta 6454 para o protocolo Art-Net.
 * 
 * Associa uma PCB UDP do lwIP à porta 6454 e registra o callback `artnet_udp_recv`.
 */
void artnet_start_listener(void);

/**
 * @brief Inicializa o servidor Web HTTP na porta TCP 80.
 * 
 * Associa uma PCB TCP do lwIP à porta 80 e registra o callback `http_accept`.
 */
void http_server_start(void);

#endif // REDE_H
