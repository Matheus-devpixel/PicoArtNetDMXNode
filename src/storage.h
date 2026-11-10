/**
 * ============================================================================
 * PROJETO: PICO ARTNET DMX NODE (RP2040)
 * ARQUIVO: storage.h
 * AUTOR: Matheus Dev (@Matheus-devpixel)
 * DESCRIÇÃO: Subsistema de persistência de parâmetros não-voláteis na memória
 *            Flash interna do microcontrolador RP2040 (XIP - eXecute In Place).
 *
 * CARACTERÍSTICAS DA PERSISTÊNCIA:
 *   - Localização: Último setor de 4 KB da memória Flash (offset: PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE).
 *   - Não utiliza EEPROM externa nem emulações voláteis: gravação direta nos setores SPI NOR Flash.
 *   - Integridade: Validada por assinatura mágica (CONFIG_MAGIC = 0x444D5832) e Checksum CRC32.
 *   - Segurança Multicore: Durante a programação da Flash, o Core 1 é pausado/resetado
 *     para prevenir acessos simultâneos ao barramento XIP pelo pipeline de instrução.
 * ============================================================================
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "lang.h"

/** Assinatura mágica de identificação do bloco de dados na Flash ("DMX2"). */
#define CONFIG_MAGIC 0x444D5832

/** ID numérico atribuído por padrão aos nós de fábrica. */
#define DEFAULT_NODE_ID 1

/** Nome amigável padrão exibido no software de controle de iluminação. */
#define DEFAULT_NODE_NAME "PICO - 1"

/** Quantidade máxima de nós configuráveis suportados na topologia de rede (1 a 16). */
#define MAX_CONFIGURABLE_NODES 16

/**
 * @brief Estrutura de dados persistente de configuração do nó.
 * 
 * Empacotada (__attribute__((packed))) e alinhada ao setor físico de 4096 bytes da memória Flash.
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;              /**< Assinatura mágica (CONFIG_MAGIC = 0x444D5832). */
    uint8_t  configured;         /**< Estado do nó: 0 = Modo Setup Inicial (10.10.0.1), 1 = Modo Operacional (2.0.N.1). */
    uint8_t  node_id;            /**< Número identificador do nó (1 a 16). Define o terceiro octeto do IP no modo 2.0.N.1. */
    char     node_name[32];      /**< Nome textual amigável do nó (exibido em softwares como QLC+, Resolume, GrandMA, Dot2). */
    uint16_t start_universe;     /**< Universo DMX inicial do nó: calculado como (node_id - 1) * def_universos. */
    int      def_universos;      /**< Quantidade de saídas DMX ativas configuradas no nó (1 a 8 saídas físicas). */
    uint8_t  rotate180;          /**< Orientação da tela OLED: 0 = normal (0°), 1 = invertido (180°). */
    uint8_t  language;           /**< Idioma da interface (app_lang_t: 0=PT, 1=EN, 2=ES, 3=FR, 4=DE, 5=IT, 6=RU, 7=ZH). */
    uint8_t  signal_loss_mode;   /**< Comportamento na perda de sinal Art-Net: 0 = Hold Last Look (manter saída), 1 = Blackout após 5s. */
    uint8_t  dmx_rate;           /**< Taxa física de envio DMX: 0 = 44 Hz (máxima), 1 = 33 Hz, 2 = 25 Hz. */
    uint8_t  theme;              /**< Tema visual da interface web: 0 = Automático (detecta SO), 1 = Escuro, 2 = Claro. */
    uint32_t crc32;              /**< Checksum CRC32 calculado sobre todos os bytes anteriores para verificação de integridade. */
    uint8_t  padding[4096 - (4 + 1 + 1 + 32 + 2 + sizeof(int) + 1 + 1 + 1 + 1 + 1 + 4)]; /**< Preenchimento até 4096 bytes. */
} config_data_t;

/** Instância global das configurações ativas do nó em memória RAM. */
extern config_data_t g_config;

/**
 * @brief Calcula o checksum CRC32 padrão IEEE 802.3 de um bloco de memória.
 * 
 * @param data Ponteiro para os dados a serem avaliados.
 * @param length Comprimento do bloco em bytes.
 * @return uint32_t Valor do CRC32 calculado.
 */
uint32_t storage_calculate_crc32(const void *data, size_t length);

/**
 * @brief Inicializa o subsistema de armazenamento e carrega as configurações da Flash.
 */
void storage_init(void);

/**
 * @brief Lê o setor de Flash e valida a assinatura mágica e o CRC32.
 * 
 * Se a Flash estiver vazia (virgem de fábrica) ou se o checksum CRC32 falhar,
 * aciona automaticamente `storage_factory_reset()` para carregar os valores seguros padrão.
 */
void storage_load(void);

/**
 * @brief Grava a estrutura g_config atual na memória Flash interna do RP2040.
 * 
 * Calcula o novo CRC32, desabilita interrupções globais, pausa o Core 1, apaga o setor
 * de 4KB com `flash_range_erase` e grava com `flash_range_program`. Em seguida, relança o Core 1.
 */
void storage_save(void);

/**
 * @brief Restaura todas as configurações para os padrões de fábrica e persiste na Flash.
 * 
 * Configura o dispositivo de volta para o Modo Setup Inicial (IP 10.10.0.1, node_id 1, 8 universos,
 * idioma Português, tema Auto).
 */
void storage_factory_reset(void);

/**
 * @brief Informa ao subsistema de armazenamento se o CORE 1 está ativo.
 * 
 * @param active true se o Core 1 estiver executando o motor DMX; false caso contrário.
 */
void storage_set_core1_active(bool active);

#endif // STORAGE_H
