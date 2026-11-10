/**
 * ============================================================================
 * PROJETO: PICO ARTNET DMX NODE (RP2040)
 * ARQUIVO: lang.h
 * AUTOR: Matheus Dev (@Matheus-devpixel)
 * DESCRIÇÃO: Subsistema de internacionalização (i18n) e suporte multilíngue
 *            para display OLED SSD1306 e mensagens de status do sistema web.
 *
 * IDIOMAS SUPORTADOS (8 IDIOMAS):
 *   - Português (Brasil)  [LANG_PT]
 *   - English              [LANG_EN]
 *   - Español              [LANG_ES]
 *   - Français             [LANG_FR]
 *   - Deutsch              [LANG_DE]
 *   - Italiano             [LANG_IT]
 *   - Русский (Russo)      [LANG_RU]
 *   - 中文 (Chinês Simpl.) [LANG_ZH]
 * ============================================================================
 */

#ifndef LANG_H
#define LANG_H

#include <stdint.h>

/**
 * @brief Enumeração dos identificadores de idioma do sistema.
 */
typedef enum {
    LANG_PT = 0, /**< Português (Brasil) - Idioma padrão. */
    LANG_EN = 1, /**< English (Inglês). */
    LANG_ES = 2, /**< Español (Espanhol). */
    LANG_FR = 3, /**< Français (Francês). */
    LANG_DE = 4, /**< Deutsch (Alemão). */
    LANG_IT = 5, /**< Italiano (Italiano). */
    LANG_RU = 6, /**< Русский (Russo - transcrição fonética/ASCII na tela OLED). */
    LANG_ZH = 7, /**< 中文 (Chinês - Pinyin/ASCII na tela OLED). */
    LANG_COUNT = 8 /**< Total de idiomas implementados. */
} app_lang_t;

/**
 * @brief Tabela de textos traduzidos exibidos no display OLED SSD1306 (128x64 pixels).
 */
typedef struct {
    const char *setup_title;        /**< Título da tela de configuração inicial. */
    const char *setup_browser;      /**< Instrução de acesso ("Acesse no Navegador:"). */
    const char *setup_waiting;      /**< Mensagem de status ocioso ("AGUARDANDO CONFIG"). */
    const char *status_node;        /**< Formato do nome do nó ("PICO - %d"). */
    const char *status_univ;        /**< Formato da faixa de universos ("DMX: %u-%u (%d Uni.)"). */
    const char *status_port;        /**< Indicação de porta de rede ("Porta: 6454"). */
    const char *status_idle;        /**< Estado sem pacotes recebidos ("ArtNet: SEM DADOS"). */
    const char *reset_title;        /**< Título da tela de confirmação de reset. */
    const char *reset_prompt;       /**< Instrução para confirmação de reset. */
    const char *reset_confirmed_t;  /**< Título de reset de fábrica executado. */
    const char *reset_confirmed_m;  /**< Mensagem de restauração concluída. */
    const char *cancel_title;       /**< Título de cancelamento de ação. */
    const char *cancel_msg;         /**< Mensagem de ação cancelada. */
    const char *starting_msg;       /**< Mensagem de inicialização do sistema ("Iniciando..."). */
} oled_lang_strings_t;

/**
 * @brief Tabela de textos das páginas web de transição (Salvar, Reiniciar, Reset).
 */
typedef struct {
    const char *save_title;         /**< Título da página de salvamento. */
    const char *save_msg;           /**< Mensagem detalhada de gravação na Flash. */
    const char *reboot_wait;        /**< Mensagem de espera de reinicialização. */
    const char *redirecting;        /**< Mensagem de redirecionamento para novo IP. */
    const char *btn_manual;         /**< Texto do botão de acesso manual caso o redirecionamento falhe. */
    const char *reset_title;        /**< Título da tela de reset de fábrica web. */
    const char *reset_msg;          /**< Mensagem de restauração de fábrica. */
    const char *reboot_title;       /**< Título da tela de reinicialização. */
    const char *reboot_msg;         /**< Mensagem de reinicialização do dispositivo. */
} web_reboot_strings_t;

/** Tabela global de strings para OLED para os 8 idiomas. */
extern const oled_lang_strings_t g_oled_lang[LANG_COUNT];

/** Tabela global de strings para telas web para os 8 idiomas. */
extern const web_reboot_strings_t g_web_reboot_lang[LANG_COUNT];

/**
 * @brief Obtém o ponteiro para o dicionário de textos OLED no idioma especificado.
 * 
 * @param lang_id Índice do idioma (0 a LANG_COUNT - 1).
 * @return const oled_lang_strings_t* Ponteiro para a estrutura correspondente (ou LANG_PT se inválido).
 */
static inline const oled_lang_strings_t *lang_get_oled(uint8_t lang_id) {
    if (lang_id >= LANG_COUNT) lang_id = LANG_PT;
    return &g_oled_lang[lang_id];
}

/**
 * @brief Obtém o ponteiro para o dicionário de textos web no idioma especificado.
 * 
 * @param lang_id Índice do idioma (0 a LANG_COUNT - 1).
 * @return const web_reboot_strings_t* Ponteiro para a estrutura correspondente (ou LANG_PT se inválido).
 */
static inline const web_reboot_strings_t *lang_get_web(uint8_t lang_id) {
    if (lang_id >= LANG_COUNT) lang_id = LANG_PT;
    return &g_web_reboot_lang[lang_id];
}

#endif // LANG_H
