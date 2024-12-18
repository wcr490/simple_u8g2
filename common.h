/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright (C) [2024] Rm
 * Author: Rm
 *
 * Modifications made in [2024] by: Rm
 */
#ifndef __COMMMON__H__
#define __COMMMON__H__

#include "stdint.h"

#define HEADER_CNT 0
#define HEADER_LEN (HEADER_CNT + 4)
#define HEADER_RLE_0 (HEADER_LEN + 4)
#define HEADER_RLE_1 (HEADER_RLE_0 + 1)

#define WIDTH_INDEX 0
#define HEIGHT_INDEX (WIDTH_INDEX + 4)

struct LookupTableItem
{
    uint8_t bit_offset;
    uint32_t byte_offset;
    char str[2];
};
struct RleCompressConf
{
    // Size of one block(Byte)
    // It has to be same in that version (Also, I don't think it will have any changes)
    uint32_t each_size;
    // Array contains the count of pairs of Rle code in each block
    uint32_t *rle_cnt;
    // Number of blocks
    uint32_t num;
    // Used in private function
    uint32_t cur_cnt;
    // Total number of pairs
    // Used in private function mostly
    uint32_t total_rle_cnt;
};
struct RleBlock
{
    // Contains Rle code
    uint8_t *rle_buf;
    // Contains result of encoding
    uint8_t *res_buf;
    // The side of the memory of res_buf
    uint32_t res_msize;
    // Rle parameters
    // background and frontground
    uint8_t rle_0, rle_1;
    // Point to the bit that will be written in the byte
    uint8_t bit_cursor;
    // Point to the byte that we are writing or readying to write
    uint32_t byte_cursor;
    // Used for getting Rle code
    // We don't get flag at the very first time
    uint8_t is_first_time;
    // Used for competition writing 
    uint8_t last_a;
    uint8_t last_b;
    // Some configuration
    struct RleCompressConf *conf;
    // Table for looking up the code relates to certain character
    struct LookupTableItem *lookup_table;
};
struct GraphicsDecoder
{
    uint8_t **buffer;
    uint32_t x_size, y_size;
    uint8_t *data_ptr;
    uint8_t rle_0, rle_1;
    uint32_t byte_cursor;
    uint8_t bit_cursor;
    uint32_t width, height;
    uint32_t x_start, y_start;
    uint32_t local_x, local_y;
    uint8_t last_bg, last_fg;
};

/* Encoder Memory Management */
uint8_t rle_block_new(struct RleBlock **self, uint32_t result_size, uint32_t block_num);
uint8_t rle_block_alloc(struct RleBlock **self, uint32_t result_size, uint32_t block_num);
uint8_t rle_block_init(struct RleBlock *self, uint32_t block_num);
uint8_t rle_block_free(struct RleBlock *self);

/* Encoder Func */
uint8_t rle_write_bits(struct RleBlock *target, uint8_t byte, uint8_t len);
uint8_t __rle_encode(struct RleBlock *target, uint8_t a, uint8_t b);
uint8_t rle_prepare_encode(struct RleBlock *target, uint8_t a, uint8_t b);
uint8_t rle_compress_stream(struct RleBlock *target, uint8_t *stream, uint32_t index);
uint8_t rle_compress_load(struct RleBlock *target, uint8_t *stream, uint32_t len);
uint8_t compress_conf_init(struct RleBlock *self);
uint8_t rle_try_best_rle(struct RleBlock *target);
uint8_t rle_get_min_size(struct RleBlock *target, uint32_t *size);
uint8_t rle_encode(struct RleBlock *target);

/* Decoder Memory Management */
uint8_t rle_decoder_init(struct GraphicsDecoder *self, uint8_t *data_ptr, uint8_t rle_0, uint8_t rle_1,
                         uint8_t width, uint8_t height);
uint8_t rle_decoder_alloc(struct GraphicsDecoder **self, uint32_t x_size, uint32_t y_size);
uint8_t rle_decoder_new(struct GraphicsDecoder **self, uint32_t x_size, uint32_t y_size, uint8_t *data_ptr,
                        uint8_t rle_0, uint8_t rle_1, uint8_t width, uint8_t height);
uint8_t rle_decoder_setup(struct GraphicsDecoder *self);
uint8_t rle_decoder_free(struct GraphicsDecoder *self);

/* Decoder Func */
uint8_t rle_decode(struct GraphicsDecoder *decoder);
uint8_t __unsafe_write_buffer(struct GraphicsDecoder *decoder, uint8_t is_background, uint32_t cnt);
uint8_t __draw_line(struct GraphicsDecoder *decoder, uint32_t x, uint32_t y, uint32_t cnt, uint8_t is_background);
uint8_t __unsafe_read_graphics_info(struct GraphicsDecoder *decoder, void *target, uint8_t info_index, uint8_t bit_cnt);
uint8_t __unsafe_read_graphics_bits(struct GraphicsDecoder *decoder, uint8_t *target, uint8_t cnt);
#endif