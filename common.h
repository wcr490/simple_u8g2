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

struct RleCompressConf {
    uint32_t offset;
    uint32_t each_size;
    uint32_t rle_cnt;
    uint32_t num;
};
struct RleBlock {
    uint8_t* rle_buf;
    uint8_t* res_buf;
    uint32_t res_msize;
    uint8_t rle_0, rle_1;
    uint8_t bit_cursor;
    uint32_t byte_cursor;
    uint32_t rle_bit_cnt;
    uint8_t is_first_time;
    uint8_t last_a;
    uint8_t last_b;
    struct RleCompressConf* conf;
};

struct GraphicsDecoder {
    uint8_t **buffer;
    uint8_t *data_ptr;
    uint8_t rle_0, rle_1;
    uint32_t len, cnt;
    uint32_t byte_cursor;
    uint32_t local_x, local_y;
    uint8_t bit_cursor;
    uint32_t width, height;
    uint32_t  x_start, y_start;
    uint8_t last_bg, last_fg;
};

uint8_t rle_new(struct RleBlock **self, uint32_t result_size);
uint8_t rle_alloc(struct RleBlock **self, uint32_t result_size);
uint8_t rle_init(struct RleBlock *self);
uint8_t rle_free(struct RleBlock *self);


uint8_t rle_write_bits(struct RleBlock* target, uint8_t byte, uint8_t len);
uint8_t rle_encode(struct RleBlock* target, uint8_t a, uint8_t b);
uint8_t rle_prepare_encode(struct RleBlock* target, uint8_t a, uint8_t b);
uint8_t rle_compress_stream(struct RleBlock* target, uint8_t* stream, uint32_t index);
uint8_t rle_compress_load(struct RleBlock *target, uint8_t* stream, uint32_t len);
uint8_t compress_conf_init(struct RleBlock *self);
uint8_t rle_try_best_rle(struct RleBlock *target);
uint8_t rle_get_min_size(struct RleBlock *target, uint32_t *size);
 
#endif