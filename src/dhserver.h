/**
 * ============================================================================
 * PROJETO: PICO ARTNET DMX NODE (RP2040)
 * ARQUIVO: dhserver.h
 * DESCRIÇÃO: Servidor DHCP IPv4 embarcado ultraleve sobre PCB UDP do lwIP.
 *            Atribui endereço IP automaticamente ao computador conectado via USB.
 * 
 * Portas utilizadas:
 *   - UDP 67: Porta de escuta do servidor DHCP (DHCP Server).
 *   - UDP 68: Porta de destino dos clientes DHCP (DHCP Client).
 * ============================================================================
 */

#ifndef DHSERVER_H
#define DHSERVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "lwip/err.h"
#include "lwip/udp.h"
#include "netif/etharp.h"

/**
 * @brief Registro de concessão (lease) de endereço IP para um cliente DHCP.
 */
typedef struct dhcp_entry
{
	uint8_t    mac[6]; /**< Endereço MAC do computador cliente. */
	ip4_addr_t addr;   /**< Endereço IPv4 concedido ao cliente. */
	uint32_t   lease;  /**< Tempo restante da concessão em segundos. */
} dhcp_entry_t;

/**
 * @brief Estrutura de configuração do servidor DHCP.
 */
typedef struct dhcp_config
{
	ip4_addr_t    router;    /**< Endereço IP do gateway padrão (o próprio nó Pico). */
	uint16_t      port;      /**< Porta UDP do servidor (67). */
	ip4_addr_t    dns;       /**< Endereço IP do servidor DNS anunciado. */
	const char   *domain;    /**< Sufixo DNS anunciado para a conexão (ex: "local"). */
	int           num_entry; /**< Quantidade de endereços disponíveis na tabela de concessão. */
	dhcp_entry_t *entries;   /**< Ponteiro para a tabela de concessões. */
} dhcp_config_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa o servidor DHCP associando o socket UDP na porta 67.
 * 
 * @param c Ponteiro para a configuração do pool DHCP.
 * @return err_t ERR_OK em caso de sucesso.
 */
err_t dhserv_init(const dhcp_config_t *c);

/**
 * @brief Encerra e desaloca a PCB UDP do servidor DHCP.
 */
void dhserv_free(void);

#ifdef __cplusplus
}
#endif

#endif /* DHSERVER_H */
