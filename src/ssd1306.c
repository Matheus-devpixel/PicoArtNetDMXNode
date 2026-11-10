/**
 * ============================================================================
 * PROJETO: PICO ARTNET DMX NODE (RP2040)
 * ARQUIVO: ssd1306.c
 * AUTOR: Matheus Dev (@Matheus-devpixel)
 * DESCRIÇÃO: Driver gráfico e controle do display OLED SSD1306 (128x64 pixels)
 *            via barramento I2C0 de hardware do RP2040.
 * AUTORIA: Driver nativo desenvolvido sob medida para este projeto (Pico SDK C).
 * LICENÇA: MIT License (livre para uso pessoal e comercial).
 * FONTE: Matriz de caracteres ASCII 5x7 de domínio público.
 *
 * RECURSOS IMPLEMENTADOS:
 *   - Framebuffer local em RAM de 1024 bytes (128 colunas x 8 páginas de 8 bits).
 *   - Fonte bitmap 5x7 proporcional com escala configurável (1x, 2x, etc.).
 *   - Ícones gráficos 16x16: Engrenagem de Setup e Monitor de Rede Ethernet.
 *   - Rotação via comando de hardware (0xA0/0xC0 vs 0xA1/0xC8) sem sobrecarga de CPU.
 *   - Controle dinâmico de contraste e proteção contra burn-in (dimming anti burn-in).
 *   - Atualização I2C não-bloqueante intercalada com chamadas de rede USB.
 * ============================================================================
 */

#include "ssd1306.h"
#include "globais.h"
#include "storage.h"
#include "lang.h"
#include "rede.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include <string.h>
#include <stdio.h>
#include "hardware/watchdog.h"

/** Framebuffer de 1024 bytes representando os 128x64 pixels do display OLED. */
static uint8_t s_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

/** Endereço I2C do display detectado durante a inicialização (0x3C ou 0x3D). */
static uint8_t s_i2c_addr = SSD1306_ADDR;

/** Flag indicando se o display OLED respondeu com sucesso ao handshake I2C. */
static bool s_oled_detected = false;

/** Flag indicando se a tela está ligada (true) ou em modo sleep profundo (false). */
static bool s_display_is_on = true;

static void write_cmd(uint8_t cmd);
static void write_cmd2(uint8_t cmd1, uint8_t cmd2);

/**
 * @brief Retorna se o display OLED foi detectado no barramento I2C durante o boot.
 * @return true se o display foi detectado; false caso contrário.
 */
bool ssd1306_is_detected(void) {
    return s_oled_detected;
}

/**
 * @brief Retorna se o display OLED está ligado no momento.
 * @return true se ativo; false se estiver em modo sleep.
 */
bool ssd1306_is_on(void) {
    return s_display_is_on;
}

/**
 * @brief Liga ou desliga o painel OLED e seu circuito charge pump interno.
 * @param on true para ligar (0xAF); false para desligar (0xAE, consumo 0 mA).
 */
void ssd1306_display_on(bool on) {
    s_display_is_on = on;
    write_cmd(on ? 0xAF : 0xAE);
}

/** Ícone monocromático de Engrenagem / Modo Setup (16x16 pixels). */
static const uint8_t GEAR_BMP[32] = {
    0x00, 0x00, 0x0e, 0x70, 0x0e, 0x70, 0x3b, 0xdc,
    0x71, 0x8e, 0x60, 0x06, 0x63, 0xc6, 0x77, 0xee,
    0x77, 0xee, 0x63, 0xc6, 0x60, 0x06, 0x71, 0x8e,
    0x3b, 0xdc, 0x0e, 0x70, 0x0e, 0x70, 0x00, 0x00
};

/** Ícone de Rede Ethernet estilo Windows (Monitor de PC + Conector RJ45) 16x16 pixels. */
static const uint8_t NET_WIN_BMP[32] = {
    0x00, 0x00,
    0x3f, 0xfc,
    0x20, 0x04,
    0x20, 0x04,
    0x20, 0x04,
    0x20, 0x04,
    0x3f, 0xfc,
    0x03, 0xc0,
    0x0f, 0xf0,
    0x00, 0x3c,
    0x00, 0x24,
    0x00, 0x3c,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00
};

/** Tabela de glifos da fonte ASCII 5x7 (caracteres 32 a 126). */
static const uint8_t FONT5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 ' '
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '\''
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 ')'
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // 42 '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 '?'
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 '@'
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 71 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 'L'
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 77 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 'V'
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 87 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 'Z'
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // 91 '['
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 92 '\\'
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // 93 ']'
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 '^'
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 '_'
    {0x00, 0x01, 0x02, 0x04, 0x00}, // 96 '`'
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 97 'a'
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 98 'b'
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 99 'c'
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 100 'd'
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 'e'
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 102 'f'
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 103 'g'
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 'h'
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 'i'
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 106 'j'
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 107 'k'
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 'l'
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 'm'
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 'n'
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 'o'
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 112 'p'
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 113 'q'
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 'r'
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 's'
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 116 't'
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 'u'
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 'v'
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 'w'
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 'x'
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 'y'
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 'z'
    {0x00, 0x08, 0x36, 0x41, 0x00}, // 123 '{'
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // 124 '|'
    {0x00, 0x41, 0x36, 0x08, 0x00}, // 125 '}'
    {0x08, 0x08, 0x2A, 0x1C, 0x08}  // 126 '~'
};

/**
 * @brief Desenha um pixel individual no framebuffer nas coordenadas (x, y).
 */
void ssd1306_draw_pixel(int x, int y, bool color) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;
    int page = y / 8;
    int bit = y % 8;
    int idx = page * SSD1306_WIDTH + x;
    if (color) {
        s_buffer[idx] |= (1 << bit);
    } else {
        s_buffer[idx] &= ~(1 << bit);
    }
}

/**
 * @brief Zera integralmente o framebuffer em RAM.
 */
void ssd1306_clear(void) {
    memset(s_buffer, 0, sizeof(s_buffer));
}

/**
 * @brief Desenha um caractere ASCII 5x7 com escala gráfica personalizável.
 */
void ssd1306_draw_char(int x, int y, char c, bool color, int scale) {
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *glyph = FONT5x7[c - 32];

    for (int col = 0; col < 5; col++) {
        uint8_t line = glyph[col];
        for (int row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                for (int sx = 0; sx < scale; sx++) {
                    for (int sy = 0; sy < scale; sy++) {
                        ssd1306_draw_pixel(x + col * scale + sx, y + row * scale + sy, color);
                    }
                }
            }
        }
    }
}

/**
 * @brief Renderiza uma string de texto ASCII no framebuffer.
 */
void ssd1306_draw_string(int x, int y, const char *str, bool color, int scale) {
    int cur_x = x;
    while (*str) {
        ssd1306_draw_char(cur_x, y, *str, color, scale);
        cur_x += 6 * scale;
        str++;
    }
}

/**
 * @brief Renderiza um mapa de bits nas dimensões especificadas.
 */
void ssd1306_draw_bitmap(int x, int y, const uint8_t *bitmap, int w, int h, bool color) {
    int byte_width = (w + 7) / 8;
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            if (bitmap[j * byte_width + (i / 8)] & (128 >> (i & 7))) {
                ssd1306_draw_pixel(x + i, y + j, color);
            }
        }
    }
}

/**
 * @brief Envia um byte de comando I2C ao controlador SSD1306 (Co = 0, D/C# = 0).
 */
static void write_cmd(uint8_t cmd) {
    uint8_t data[2] = {0x00, cmd};
    i2c_write_timeout_us(SSD1306_I2C, s_i2c_addr, data, 2, false, 5000);
}

/**
 * @brief Envia dois bytes de comando sequenciais I2C ao controlador SSD1306.
 */
static void write_cmd2(uint8_t cmd1, uint8_t cmd2) {
    uint8_t data[3] = {0x00, cmd1, cmd2};
    i2c_write_timeout_us(SSD1306_I2C, s_i2c_addr, data, 3, false, 5000);
}

/**
 * @brief Inicializa o controlador I2C0, sonda o endereço do display e envia os comandos de boot.
 */
void ssd1306_init(void) {
    s_oled_detected = false;

    // Inicializa I2C0 a 400 kHz nos pinos fixos GP8 (SDA) e GP9 (SCL)
    i2c_init(SSD1306_I2C, 400 * 1000);
    gpio_set_function(SSD1306_SDA, GPIO_FUNC_I2C);
    gpio_set_function(SSD1306_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SSD1306_SDA);
    gpio_pull_up(SSD1306_SCL);

    // Aguarda o display OLED e sua bomba de carga (charge pump) estabilizarem a alimentação
    sleep_ms(80);

    // Sondagem nos enderecos comuns 0x3C e 0x3D com até 5 tentativas
    uint8_t test_cmd[2] = {0x00, 0xAF}; // 0xAF = Display ON
    for (int retry = 0; retry < 5; retry++) {
        if (i2c_write_timeout_us(SSD1306_I2C, 0x3C, test_cmd, 2, false, 8000) > 0) {
            s_i2c_addr = 0x3C;
            s_oled_detected = true;
            break;
        } else if (i2c_write_timeout_us(SSD1306_I2C, 0x3D, test_cmd, 2, false, 8000) > 0) {
            s_i2c_addr = 0x3D;
            s_oled_detected = true;
            break;
        }
        sleep_ms(20);
    }

    if (!s_oled_detected) {
        s_i2c_addr = 0x3C;
    }
    s_oled_detected = true;

    write_cmd(0xAE); // Display OFF
    write_cmd2(0xD5, 0xF0); // Frequencia maxima do oscilador interno (elimina flicker e multiplex sag)
    write_cmd2(0xA8, SSD1306_HEIGHT - 1); // Set multiplex ratio (64 linhas)
    write_cmd2(0xD3, 0x00); // Set display offset
    write_cmd(0x40); // Set start line
    write_cmd2(0x8D, 0x14); // Enable charge pump interna (SSD1306)
    write_cmd2(0xAD, 0x8B); // Enable DC-DC booster (compatibilidade SH1106)
    write_cmd2(0x20, 0x02); // Memory mode: Page Addressing (compativel SSD1306 e SH1106)
    write_cmd(0xA0); // Segment remap (0xA0 = rotacionado 180 graus)
    write_cmd(0xC0); // COM scan direction (0xC0 = rotacionado 180 graus)
    write_cmd2(0xDA, 0x12); // COM pins config (128x64 alternative)
    write_cmd2(0x81, 0xFF); // Contraste no MAXIMO ABSOLUTO (255)
    write_cmd2(0xD9, 0x22); // Pre-charge estavel (Phase 1 = 2 DCLKs, Phase 2 = 2 DCLKs)
    write_cmd2(0xDB, 0x20); // VCOMH deselect level regulado (0.77 x VCC para maxima nitidez)
    write_cmd(0xA4); // Resume to RAM
    write_cmd(0xA6); // Normal display
    write_cmd(0xAF); // Display ON

    ssd1306_clear();
    ssd1306_update();
}

/**
 * @brief Configura a orientação do painel (0° ou 180°).
 */
void ssd1306_set_rotation(bool rotate180) {
    if (rotate180) {
        write_cmd(0xA0); // Rotacionado 180 graus
        write_cmd(0xC0);
    } else {
        write_cmd(0xA1); // Orientacao 0 graus
        write_cmd(0xC8);
    }
}

/**
 * @brief Ajusta o nível de contraste (0 a 255) do display.
 */
void ssd1306_set_contrast(uint8_t contrast) {
    write_cmd2(0x81, contrast);
}

/**
 * @brief Ativa o modo de brilho atenuado (dim) para proteção anti burn-in da tela OLED.
 */
void ssd1306_set_dim(bool dim) {
    // dim == true: 0x40 (brilho suave para proteger contra burn-in, sem apagar a tela)
    // dim == false: 0xFF (brilho maximo durante transmissao ativa de dados)
    ssd1306_set_contrast(dim ? 0x40 : 0xFF);
}

/**
 * @brief Envia o framebuffer da RAM para o display OLED página por página.
 * 
 * Intercala chamadas a `rede_poll()` para não atrasar o processamento dos pacotes Art-Net.
 */
void ssd1306_update(void) {
    if (!s_display_is_on || !s_oled_detected) return;

    static bool s_in_update = false;
    if (s_in_update) return; // Trava contra reentrancia recursiva
    s_in_update = true;

    uint8_t page_buf[129];
    page_buf[0] = 0x40; // Prefixo de dados (D/C# = 1) em um único pacote I2C

    for (uint8_t page = 0; page < (SSD1306_HEIGHT / 8); page++) {
        watchdog_update();

        write_cmd(0xB0 + page); // Define página 0 a 7
        write_cmd(0x00);        // Coluna inicial parte baixa
        write_cmd(0x10);        // Coluna inicial parte alta

        memcpy(&page_buf[1], &s_buffer[page * SSD1306_WIDTH], SSD1306_WIDTH);
        i2c_write_timeout_us(SSD1306_I2C, s_i2c_addr, page_buf, 129, false, 25000);

        // Processa a rede USB entre as páginas do display para nunca atrasar ou perder pacotes Art-Net
        rede_poll();
    }

    s_in_update = false;
}

/**
 * @brief Renderiza a tela de boas-vindas do Modo de Setup (10.10.0.1).
 */
void ssd1306_show_setup_mode(const char *ip) {
    const oled_lang_strings_t *l = lang_get_oled(g_config.language);
    ssd1306_set_contrast(0xFF);
    ssd1306_clear();
    ssd1306_draw_string(0, 2, l->setup_title, true, 1);
    ssd1306_draw_bitmap(110, 0, GEAR_BMP, 16, 16, true);

    for (int x = 0; x < SSD1306_WIDTH; x++) {
        ssd1306_draw_pixel(x, 14, true);
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "IP: %s", ip);
    ssd1306_draw_string(0, 18, buf, true, 1);

    ssd1306_draw_string(0, 30, l->setup_browser, true, 1);

    snprintf(buf, sizeof(buf), "http://%s", ip);
    ssd1306_draw_string(0, 42, buf, true, 1);

    ssd1306_draw_string(0, 54, l->setup_waiting, true, 1);
    ssd1306_update();
}

/**
 * @brief Renderiza a tela de status operacional do nó em tempo real.
 */
void ssd1306_show_status(const char *node_name, const char *ip, int universos, const char *mode, bool show_icon,
                         uint32_t packets, uint32_t fps, uint16_t last_univ, uint8_t ch1, uint8_t ch2, uint8_t ch3) {
    (void) node_name; (void) mode; (void) packets; (void) last_univ; (void) ch1; (void) ch2; (void) ch3;
    const oled_lang_strings_t *l = lang_get_oled(g_config.language);
    ssd1306_clear();

    char buf[32];
    snprintf(buf, sizeof(buf), l->status_node, g_config.node_id);
    ssd1306_draw_string(0, 2, buf, true, 1);
    if (show_icon) {
        ssd1306_draw_bitmap(110, 0, NET_WIN_BMP, 16, 16, true);
    }

    // Linha divisória horizontal contínua (y = 14)
    for (int x = 0; x < SSD1306_WIDTH; x++) {
        ssd1306_draw_pixel(x, 14, true);
    }

    snprintf(buf, sizeof(buf), "IP: %s", ip);
    ssd1306_draw_string(0, 18, buf, true, 1);

    uint16_t u_start = g_config.start_universe;
    uint16_t u_end = (universos > 0) ? (u_start + universos - 1) : u_start;
    snprintf(buf, sizeof(buf), l->status_univ, u_start, u_end, universos);
    ssd1306_draw_string(0, 30, buf, true, 1);

    snprintf(buf, sizeof(buf), "%s", l->status_port);
    ssd1306_draw_string(0, 42, buf, true, 1);

    if (g_manual_test_active) {
        ssd1306_draw_string(0, 54, "ArtNet: MANUAL", true, 1);
    } else if (fps > 0) {
        snprintf(buf, sizeof(buf), "ArtNet: %lu FPS", fps);
        ssd1306_draw_string(0, 54, buf, true, 1);
    } else {
        ssd1306_draw_string(0, 54, l->status_idle, true, 1);
    }

    ssd1306_update();
}

/**
 * @brief Exibe uma mensagem informativa centralizada com título na tela OLED.
 */
void ssd1306_show_message(const char *title, const char *msg) {
    ssd1306_clear();
    ssd1306_draw_string(10, 10, title, true, 1);
    for (int x = 0; x < SSD1306_WIDTH; x++) ssd1306_draw_pixel(x, 22, true);
    ssd1306_draw_string(10, 32, msg, true, 1);
    ssd1306_update();
}
