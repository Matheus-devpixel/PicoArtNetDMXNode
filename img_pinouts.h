/**
 * ============================================================================
 * PROJETO: PICO ARTNET DMX NODE (RP2040)
 * ARQUIVO: img_pinouts.h
 * AUTOR: Matheus Dev (@Matheus-devpixel)
 * DESCRIÇÃO: Declarações dos arrays binários das imagens de pinagem dos boards
 *            Raspberry Pi Pico / Pico W e Waveshare RP2040-Zero.
 *
 * CRÉDITOS E ATRIBUIÇÃO DOS DIAGRAMAS DE PINAGEM:
 * As ilustrações detalhadas e esquemáticos de pinagem contidos nestes binários
 * foram concebidos, desenhados e disponibilizados por Renzo Mischianti.
 * Website Oficial: https://www.mischiant.org
 * Artigos de referência:
 *   - Raspberry Pi Pico / Pico W pinout: https://www.mischiant.org
 *   - Waveshare RP2040-Zero pinout: https://www.mischiant.org
 * Todos os direitos autorais dos diagramas gráficos pertencem a Renzo Mischianti.
 * ============================================================================
 */

#ifndef IMG_PINOUTS_H
#define IMG_PINOUTS_H

#include <stdint.h>
#include <stddef.h>

/**
 * Imagem de pinagem do Waveshare RP2040-Zero (Formato WebP comprimido).
 * Armazenada em memória Flash XIP, alinhada em 4 bytes para leitura direta
 * e streaming em blocos pelo servidor HTTP (rede.c).
 */
extern const size_t g_img_zero_len;
extern const uint8_t g_img_zero_data[];

/**
 * Imagem de pinagem do Raspberry Pi Pico / Pico W (Formato WebP comprimido).
 * Armazenada em memória Flash XIP, alinhada em 4 bytes para leitura direta
 * e streaming em blocos pelo servidor HTTP (rede.c).
 */
extern const size_t g_img_picow_len;
extern const uint8_t g_img_picow_data[];

#endif // IMG_PINOUTS_H
