/**
 * ============================================================================
 * PROJETO: PICO ARTNET DMX NODE (RP2040)
 * ARQUIVO: ssd1306.h
 * AUTOR: Matheus Dev (@Matheus-devpixel)
 * DESCRIÇÃO: Driver gráfico e interface de controle para o display OLED SSD1306
 *            (128x64 pixels monocromático) via barramento de hardware I2C0.
 * AUTORIA: Driver nativo desenvolvido especificamente para este projeto (Pico SDK C).
 * LICENÇA: MIT License (livre para uso pessoal e comercial).
 * FONTE: Matriz de caracteres ASCII 5x7 de domínio público.
 *
 * ESPECIFICAÇÕES DO HARDWARE OLED:
 *   - Resolução: 128 colunas x 64 linhas (8 páginas de 8 pixels verticais).
 *   - Barramento: I2C0 a 400 kHz (SDA: GP8, SCL: GP9).
 *   - Endereço I2C padrão: 0x3C (ou 0x3D selecionável por jumper físico).
 *   - Framebuffer local: 1024 bytes (128 * 64 / 8 bits).
 *   - Recursos avançados: Dimming anti burn-in, rotação de 180°, telas multilingues.
 * ============================================================================
 */

#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

/** Instância de hardware do barramento I2C utilizada (i2c0). */
#define SSD1306_I2C i2c0

/** Pino GPIO conectado à linha SDA (GP8). */
#define SSD1306_SDA 8

/** Pino GPIO conectado à linha SCL (GP9). */
#define SSD1306_SCL 9

/** Endereço padrão I2C de 7 bits do controlador SSD1306. */
#define SSD1306_ADDR 0x3C

/** Largura física da tela em pixels. */
#define SSD1306_WIDTH 128

/** Altura física da tela em pixels. */
#define SSD1306_HEIGHT 64

/**
 * @brief Inicializa o barramento I2C0 a 400 kHz e envia a sequência de comandos de inicialização do SSD1306.
 * 
 * Configura charge pump interno de 7.5V, endereçamento por páginas, multiplex ratio,
 * oscilador e orientação inicial. Realiza detecção automática do display.
 */
void ssd1306_init(void);

/**
 * @brief Define a orientação física do display (normal ou rotacionado 180°).
 * 
 * @param rotate180 true para inverter a tela em 180° (remap de segmentos e scan inverso);
 *                  false para orientação padrão.
 */
void ssd1306_set_rotation(bool rotate180);

/**
 * @brief Ajusta o nível de contraste (brilho) do painel OLED.
 * 
 * @param contrast Valor de 0x00 (mínimo) a 0xFF (brilho máximo).
 */
void ssd1306_set_contrast(uint8_t contrast);

/**
 * @brief Alterna entre o modo de brilho normal e o modo esmaecido (Dimming).
 * 
 * @param dim true para ativar brilho suave (0x40) anti burn-in quando ocioso;
 *            false para brilho total (0xFF) quando em transmissão ativa.
 */
void ssd1306_set_dim(bool dim);

/**
 * @brief Liga ou desliga o painel OLED (modo sleep profundo).
 * 
 * @param on true para ligar a tela (0xAF); false para desligar charge pump e painel (0xAE, 0 mA).
 */
void ssd1306_display_on(bool on);

/**
 * @brief Consulta se o display está ligado no momento.
 * 
 * @return true se o display estiver ativo; false se estiver em sleep.
 */
bool ssd1306_is_on(void);

/**
 * @brief Informa se o display OLED SSD1306 foi detectado com sucesso no barramento I2C.
 * 
 * @return true se o display respondeu ao handshake I2C no boot; false caso ausente ou desconectado.
 */
bool ssd1306_is_detected(void);

/**
 * @brief Limpa integralmente o framebuffer em RAM (zera os 1024 bytes).
 */
void ssd1306_clear(void);

/**
 * @brief Transfere o framebuffer da RAM para a memória GDDRAM do SSD1306 via I2C0.
 * 
 * A transferência é segmentada página por página (8 páginas de 128 bytes) com chamadas
 * intercaladas a `rede_poll()` para assegurar que o tráfego de rede USB não sofra atrasos.
 */
void ssd1306_update(void);

/**
 * @brief Plota um pixel individual nas coordenadas (x, y) do framebuffer.
 * 
 * @param x Coordenada horizontal (0 a 127).
 * @param y Coordenada vertical (0 a 63).
 * @param color true para acender o pixel; false para apagar.
 */
void ssd1306_draw_pixel(int x, int y, bool color);

/**
 * @brief Renderiza um caractere ASCII 5x7 nas coordenadas especificadas.
 * 
 * @param x Posição horizontal inicial em pixels.
 * @param y Posição vertical inicial em pixels.
 * @param c Caractere ASCII a desenhar (faixa 32 a 126).
 * @param color true para cor direta; false para cor invertida.
 * @param scale Fator multiplicador de escala (1 para 5x7 original, 2 para 10x14, etc.).
 */
void ssd1306_draw_char(int x, int y, char c, bool color, int scale);

/**
 * @brief Renderiza uma string de texto ASCII no framebuffer.
 * 
 * @param x Posição horizontal inicial.
 * @param y Posição vertical inicial.
 * @param str String terminada em nulo a desenhar.
 * @param color Cor direta (true) ou invertida (false).
 * @param scale Escala da fonte (1, 2, etc.).
 */
void ssd1306_draw_string(int x, int y, const char *str, bool color, int scale);

/**
 * @brief Desenha um mapa de bits monocromático (bitmap) nas dimensões w x h.
 * 
 * @param x Posição horizontal inicial.
 * @param y Posição vertical inicial.
 * @param bitmap Ponteiro para o array de bytes com os dados da imagem (bit packing por linha).
 * @param w Largura do bitmap em pixels.
 * @param h Altura do bitmap em pixels.
 * @param color Cor dos pixels ativos.
 */
void ssd1306_draw_bitmap(int x, int y, const uint8_t *bitmap, int w, int h, bool color);

/**
 * @brief Exibe a tela gráfica do Modo de Configuração Inicial (Setup 10.10.0.1).
 * 
 * @param ip String com o endereço IP ativo do nó (ex: "10.10.0.1").
 */
void ssd1306_show_setup_mode(const char *ip);

/**
 * @brief Exibe a tela principal de status operacional do nó Art-Net.
 * 
 * Apresenta em tempo real:
 *   - Cabeçalho: Identidade do nó e ícone de status de conexão de rede.
 *   - Linha IP: Endereço IP operacional do nó (ex: 2.0.1.1).
 *   - Linha Universos: Faixa de universos DMX mapeados (ex: DMX: 0-7 (8 Uni.)).
 *   - Linha Porta: Porta padrão UDP (6454).
 *   - Rodapé Dinâmico: ArtNet: XX FPS (quando em tráfego), ArtNet: MANUAL (em teste), ou ArtNet: SEM DADOS.
 * 
 * @param node_name Nome do nó.
 * @param ip Endereço IP do nó.
 * @param universos Quantidade de universos ativos.
 * @param mode Modo de conexão ("USB").
 * @param show_icon Se true, exibe o ícone de rede Ethernet.
 * @param packets Total de pacotes recebidos.
 * @param fps Taxa de quadros por segundo calculada.
 * @param last_univ Último universo recebido.
 * @param ch1 Valor do canal 1.
 * @param ch2 Valor do canal 2.
 * @param ch3 Valor do canal 3.
 */
void ssd1306_show_status(const char *node_name, const char *ip, int universos, const char *mode, bool show_icon,
                         uint32_t packets, uint32_t fps, uint16_t last_univ, uint8_t ch1, uint8_t ch2, uint8_t ch3);

/**
 * @brief Exibe uma mensagem informativa centralizada com título na tela OLED.
 * 
 * @param title Título superior.
 * @param msg Mensagem descritiva inferior.
 */
void ssd1306_show_message(const char *title, const char *msg);

#endif // SSD1306_H
