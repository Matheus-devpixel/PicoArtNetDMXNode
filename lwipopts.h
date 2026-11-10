/**
 * ============================================================================
 * PROJETO: PICO ARTNET DMX NODE (RP2040)
 * ARQUIVO: lwipopts.h
 * DESCRIÇÃO: Opções de compilação da pilha TCP/IP lwIP otimizadas para alta
 *            performance e baixa latência no microcontrolador RP2040.
 *
 * ESTRATÉGIA DE MEMÓRIA E LATÊNCIA:
 *   - Execução bare-metal sem RTOS (NO_SYS = 1).
 *   - Pool de memória de 64 KB para absorver picos de tráfego Art-Net de alta taxa.
 *   - TCP MSS = 1460 (tamanho máximo de segmento Ethernet MTU 1500).
 *   - Janela TCP aumentada para permitir streaming contínuo DMX (/stream) sem engasgos.
 *   - Filtragem rápida UDP em nível de hardware/driver para as portas 67 (DHCP) e 6454 (Art-Net).
 * ============================================================================
 */

#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

/** Execução direta bare-metal no RP2040 sem dependência de RTOS. */
#define NO_SYS                          1

/** Alinhamento de memória em 32 bits (4 bytes) exigido pelo ARM Cortex-M0+. */
#define MEM_ALIGNMENT                   4

/** Tamanho do heap estático do lwIP: 64 KB (65.536 bytes) de RAM dedicada. */
#define MEM_SIZE                        65536

/** Número máximo de blocos de controle de conexão TCP (PCB) simultâneos. */
#define MEMP_NUM_TCP_PCB                16

/** Número de PCBs TCP em estado de escuta (LISTEN). */
#define MEMP_NUM_TCP_PCB_LISTEN         8

/** Quantidade de segmentos TCP pré-alocados no pool de memória. */
#define MEMP_NUM_TCP_SEG                160

/** Capacidade da fila de pacotes ARP pendentes de resolução. */
#define MEMP_NUM_ARP_QUEUE              16

/** Quantidade de buffers pbuf alocados no pool de recepção de alta velocidade. */
#define PBUF_POOL_SIZE                  48

/** Habilita protocolo de resolução de endereços ARP (Address Resolution Protocol). */
#define LWIP_ARP                        1

/** Habilita suporte a frames de camada de enlace Ethernet (802.3). */
#define LWIP_ETHERNET                   1

/** Habilita protocolo ICMP (permite que o nó responda a comandos PING do Windows/Linux). */
#define LWIP_ICMP                       1

#define LWIP_RAW                        0
#define LWIP_NETCONN                    0
#define LWIP_SOCKET                     0

/** Tamanho máximo de segmento TCP: 1460 bytes (Ethernet padrão 1500 MTU - 40 cabeçalhos). */
#define TCP_MSS                         1460

/** Janela de recepção TCP: 20 * 1460 = 29.200 bytes para fluxo ininterrupto de dados. */
#define TCP_WND                         (20 * TCP_MSS)

/** Buffer de envio TCP: 40 * 1460 = 58.400 bytes para entrega ultra-rápida de WebP e HTML. */
#define TCP_SND_BUF                     (40 * TCP_MSS)

/** Comprimento da fila de transmissão TCP. */
#define TCP_SND_QUEUELEN                ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))

#define LWIP_NETIF_STATUS_CALLBACK      1
#define LWIP_NETIF_LINK_CALLBACK        1
#define LWIP_NETIF_HOSTNAME             1
#define LWIP_IPV4                       1
#define LWIP_IPV6                       0
#define LWIP_TCP                        1
#define LWIP_UDP                        1
#define LWIP_DNS                        1
#define LWIP_DHCP                       0

#define ETH_PAD_SIZE                    0

/**
 * Macro de aceitação rápida de portas UDP: descarta na camada mais baixa qualquer
 * pacote que não seja DHCP (porta 67) ou Art-Net (porta 6454), poupando tempo de CPU.
 */
#define LWIP_IP_ACCEPT_UDP_PORT(p)      (((p) == PP_NTOHS(67)) || ((p) == PP_NTOHS(6454)))

#define CYW43_NETIF_DEBUG               0
#define LWIP_DEBUG                      0

#endif // _LWIPOPTS_H
