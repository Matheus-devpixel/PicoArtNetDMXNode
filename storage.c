/**
 * ============================================================================
 * PROJETO: PICO ARTNET DMX NODE (RP2040)
 * ARQUIVO: storage.c
 * AUTOR: Matheus Dev (@Matheus-devpixel)
 * DESCRIÇÃO: Implementação das rotinas de leitura, validação e gravação direta
 *            no setor de Flash NOR SPI interna do Raspberry Pi RP2040.
 *
 * PREVENÇÃO DE CONFLITOS DE BARRAMENTO MULTICORE (XIP SAFETY):
 *   O RP2040 executa código diretamente da Flash através do controlador XIP.
 *   Operações de apagamento (erase) ou escrita (program) tornam a Flash
 *   temporariamente inacessível para leitura de instruções.
 *   Por isso, antes de qualquer operação na Flash:
 *     1. O CORE 1 é paralisado com `multicore_reset_core1()`.
 *     2. As interrupções locais do CORE 0 são desabilitadas com `save_and_disable_interrupts()`.
 *     3. O setor de 4096 bytes é apagado e regravado.
 *     4. As interrupções são restauradas e o CORE 1 é reinicializado com `multicore_launch_core1()`.
 * ============================================================================
 */

#include "storage.h"
#include "dmx.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/multicore.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>

/** Deslocamento em bytes da memória Flash onde o setor de configuração reside (últimos 4096 bytes). */
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

/** Ponteiro constante mapeado no espaço de endereçamento linear XIP (0x10000000 + offset). */
static const uint8_t *flash_target_contents = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);

/** Indicador interno se o motor DMX do CORE 1 está em execução. */
static bool s_core1_active = false;

/** Instância global das configurações ativas do nó. */
config_data_t g_config;

/**
 * @brief Calcula o checksum CRC32 padrão IEEE 802.3 polinômio 0xEDB88320 invertido.
 * 
 * @param data Ponteiro para os dados a serem avaliados.
 * @param length Quantidade de bytes a processar.
 * @return uint32_t Checksum calculado.
 */
uint32_t storage_calculate_crc32(const void *data, size_t length) {
    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= p[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

/**
 * @brief Notifica o subsistema de armazenamento sobre o estado de execução do Core 1.
 * 
 * @param active true se o Core 1 foi lançado; false se estiver inativo.
 */
void storage_set_core1_active(bool active) {
    s_core1_active = active;
}

/**
 * @brief Restaura todos os parâmetros de configuração aos padrões seguros de fábrica.
 * 
 * Define:
 *   - magic = CONFIG_MAGIC (0x444D5832)
 *   - configured = 0 (Modo Setup Inicial no IP 10.10.0.1)
 *   - node_id = 1
 *   - node_name = "PICO - 1"
 *   - def_universos = 8 saídas físicas
 *   - rotate180 = 1 (Display OLED invertido por padrão)
 *   - language = LANG_PT (Português)
 *   - signal_loss_mode = 0 (Hold Last Look)
 *   - dmx_rate = 0 (44 Hz taxa máxima)
 *   - theme = 0 (Tema Automático)
 * 
 * Grava imediatamente na Flash após a redefinição.
 */
void storage_factory_reset(void) {
    memset(&g_config, 0, sizeof(config_data_t));
    g_config.magic = CONFIG_MAGIC;
    g_config.configured = 0;              // 0 = Inicia no Modo Setup Inicial (10.10.0.1)
    g_config.node_id = DEFAULT_NODE_ID;   // 1
    strncpy(g_config.node_name, DEFAULT_NODE_NAME, sizeof(g_config.node_name) - 1);
    g_config.start_universe = 0;
    g_config.def_universos = 8;
    g_config.rotate180 = 1;              // Padrão rotacionado 180 graus
    g_config.language = LANG_PT;         // Padrão: Português (Brasil)
    g_config.signal_loss_mode = 0;       // 0 = Hold Last Look (manter saída)
    g_config.dmx_rate = 0;               // 0 = 44 Hz (máxima velocidade)
    g_config.theme = 0;                  // 0 = Auto (1 = Dark, 2 = Light)
    storage_save();
}

/**
 * @brief Inicializa o módulo de armazenamento carregando os dados da Flash.
 */
void storage_init(void) {
    storage_load();
}

/**
 * @brief Realiza a leitura e validação estrita da Flash de configuração.
 * 
 * Verifica:
 *   1. Assinatura mágica de integridade (CONFIG_MAGIC).
 *   2. Checksum CRC32 recalculado sobre a estrutura de dados.
 *   3. Limites dos campos (node_id 1..16, universos 1..8, idioma válido, taxas válidas).
 * 
 * Se qualquer teste falhar, aciona `storage_factory_reset()`. Se for válido, copia
 * para `g_config` e calcula o universo inicial com base no ID do nó.
 */
void storage_load(void) {
    const config_data_t *stored = (const config_data_t *)flash_target_contents;
    bool valid = true;

    if (stored->magic != CONFIG_MAGIC) {
        valid = false;
    } else {
        // Validação estrita por checksum CRC32 contra falhas de gravação/corte de energia
        uint32_t calc_crc = storage_calculate_crc32(stored, offsetof(config_data_t, crc32));
        if (stored->crc32 != calc_crc) {
            printf("[STORAGE] Checksum CRC32 divergente (calculado: 0x%08lX, lido: 0x%08lX)\n",
                   (unsigned long)calc_crc, (unsigned long)stored->crc32);
            valid = false;
        } else {
            // Validação estrita de limites dos campos
            if (stored->configured > 1) valid = false;
            if (stored->node_id < 1 || stored->node_id > MAX_CONFIGURABLE_NODES) valid = false;
            if (stored->def_universos < 1 || stored->def_universos > 8) valid = false;
            if (stored->language >= LANG_COUNT) valid = false;
            if (stored->rotate180 > 1) valid = false;
            if (stored->signal_loss_mode > 1) valid = false;
            if (stored->dmx_rate > 2) valid = false;
            if (stored->theme > 2) valid = false;
            if (stored->node_name[0] == '\0' || (uint8_t)stored->node_name[0] == 0xFF) valid = false;
        }
    }

    if (valid) {
        memcpy(&g_config, stored, sizeof(config_data_t));
        g_config.node_name[sizeof(g_config.node_name) - 1] = '\0';
        g_config.start_universe = (g_config.node_id - 1) * g_config.def_universos;
        printf("[STORAGE] Configuracao valida (CRC OK): No %d | %d Universos | Modo: %s | DMX: %s\n",
               g_config.node_id, g_config.def_universos,
               g_config.configured ? "Operacional" : "Setup",
               g_config.dmx_rate == 0 ? "44Hz" : (g_config.dmx_rate == 1 ? "33Hz" : "25Hz"));
    } else {
        printf("[STORAGE] Flash vazia ou checksum corrompido. Restaurando padroes de fabrica...\n");
        storage_factory_reset();
    }
}

/**
 * @brief Grava a estrutura g_config atual no setor Flash com isolamento multicore.
 */
void storage_save(void) {
    g_config.magic = CONFIG_MAGIC;
    g_config.crc32 = storage_calculate_crc32(&g_config, offsetof(config_data_t, crc32));

    // Reseta o Core 1 por hardware durante a gravação na Flash para eliminar deadlocks e contenção no barramento XIP
    bool restart_core1 = s_core1_active;
    if (restart_core1) {
        multicore_reset_core1();
    }

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t *)&g_config, sizeof(config_data_t));
    restore_interrupts(ints);

    if (restart_core1) {
        multicore_launch_core1(dmx_core1_entry);
    }

    printf("[STORAGE] Configuracoes gravadas com sucesso na Flash (CRC32: 0x%08lX)!\n", (unsigned long)g_config.crc32);
}
