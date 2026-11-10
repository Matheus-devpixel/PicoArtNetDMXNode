/**
 * ============================================================================
 * PROJETO: PICO ARTNET DMX NODE (RP2040)
 * ARQUIVO: dnserver.h
 * DESCRIÇÃO: Servidor DNS IPv4 embarcado ultraleve para Captive Portal.
 *            Intercepta requisições de nomes na porta UDP 53 e redireciona
 *            o navegador para o endereço IP local do nó no Modo Setup.
 * ============================================================================
 */

#ifndef DNSERVER
#define DNSERVER

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "lwip/def.h"
#include "lwip/err.h"
#include "lwip/udp.h"
#include "netif/etharp.h"

/**
 * @brief Ponteiro de função de callback para resolução de nomes DNS.
 * 
 * @param name Nome consultado pelo cliente (ex: "setup.local", "pico.artnet").
 * @param addr Endereço IPv4 preenchido pelo callback se o nome for reconhecido.
 * @return true se o nome foi reconhecido e deve ser respondido; false para ignorar.
 */
typedef bool (*dns_query_proc_t)(const char *name, ip4_addr_t *addr);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa o servidor DNS na porta UDP especificada (padrão: 53).
 * 
 * @param bind Endereço IP local de escuta.
 * @param port Porta UDP (53).
 * @param query_proc Função de callback para tratamento de cada consulta DNS.
 * @return err_t ERR_OK em sucesso.
 */
err_t dnserv_init(const ip_addr_t *bind, uint16_t port, dns_query_proc_t query_proc);

/**
 * @brief Encerra e desaloca a PCB UDP do servidor DNS.
 */
void  dnserv_free(void);

#ifdef __cplusplus
}
#endif

#endif // DNSERVER
