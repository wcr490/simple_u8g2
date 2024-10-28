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
 * Time: 2004.10.26
 *
 * Modifications made in [2024] by: Rm
 */
#include "common.h"

#include "string.h"
#include "assert.h"
#include "stdlib.h"
#include "stdio.h"
uint8_t rle_decoder_setup(struct GraphicsDecoder *self) {
    self->last_bg = 0, self->last_fg = 0;
    self->byte_cursor = 0;
    self->bit_cursor = 0;
    self->local_x = 0;
    self->local_y = 0;
    for (int i = 0; i < self->y_size; i++)
    {
        memset(self->buffer[i], 0, self->x_size);
    }
    return 0;
}
uint8_t rle_decoder_init(struct GraphicsDecoder *self, uint8_t *data_ptr, uint8_t rle_0, uint8_t rle_1,
                         uint8_t width, uint8_t height)
{
    if (data_ptr == NULL)
    {
        return 1;
    }
    if (width == 0 || height == 0)
    {
        return 2;
    }
    if (rle_0 == 0 || rle_1 == 0)
    {
        return 3;
    }
    self->data_ptr = data_ptr;
    self->rle_0 = rle_0;
    self->rle_1 = rle_1;
    self->width = width;
    self->height = height;
    self->x_start = 0;
    self->y_start = 0;
    self->local_x = 0;
    self->local_y = 0;
    self->byte_cursor = 0;
    self->bit_cursor = 0;
    return 0;
}
uint8_t rle_decoder_alloc(struct GraphicsDecoder **self, uint32_t x_size, uint32_t y_size)
{
    struct GraphicsDecoder *new = malloc(sizeof(struct GraphicsDecoder));
    if (new == NULL)
        return 1;
    new->buffer = malloc(y_size * sizeof(uint8_t *));
    for (int i = 0; i < y_size; i++)
    {
        new->buffer[i] = malloc(x_size * sizeof(uint8_t));
    }
    new->x_size = x_size, new->y_size = y_size;
    *self = new;
    return 0;
}
uint8_t rle_decoder_new(struct GraphicsDecoder **self, uint32_t x_size, uint32_t y_size, uint8_t *data_ptr,
                        uint8_t rle_0, uint8_t rle_1, uint8_t width, uint8_t height)
{
    struct GraphicsDecoder *new;
    if (rle_decoder_alloc(&new, x_size, y_size))
        return 1;
    if (rle_decoder_init(new, data_ptr, rle_0, rle_1, width, height))
        return 2;
    *self = new;
    return 0;
}
uint8_t rle_decoder_free(struct GraphicsDecoder *self) {
    if (self != NULL) {
        for (int i = 0; i < self->y_size; i++)
        {
            free(self->buffer[i]);
        }
        free(self->buffer);
    }
    free(self);
    self = NULL;
    return 0;
}
uint8_t update_graphics_rle_0_1(struct GraphicsDecoder *decoder)
{
    decoder->rle_0 = *(uint8_t *)(decoder->data_ptr + HEADER_RLE_0);
    decoder->rle_1 = *(uint8_t *)(decoder->data_ptr + HEADER_RLE_1);
    return 0;
}

uint8_t __unsafe_read_graphics_info(struct GraphicsDecoder *decoder, void *target, uint8_t info_index, uint8_t bit_cnt)
{
    assert(decoder->byte_cursor + info_index + (bit_cnt / 8) < *(uint32_t *)(decoder->data_ptr + HEADER_LEN));
    if (bit_cnt <= 8)
    {
        memcpy(target, (uint8_t *)(decoder->byte_cursor + info_index), sizeof(uint8_t));
    }
    else if (bit_cnt <= 16)
    {
        memcpy(target, (uint8_t *)(decoder->byte_cursor + info_index), sizeof(uint16_t));
    }
    else if (bit_cnt <= 32)
    {
        memcpy(target, (uint8_t *)(decoder->byte_cursor + info_index), sizeof(uint32_t));
    }
    else
    {
        return 1;
    }
    return 0;
}
uint8_t __unsafe_read_graphics_bits(struct GraphicsDecoder *decoder, uint8_t *target, uint8_t cnt)
{
    // TODO: Add more checkings here
    assert(cnt <= 8);

    uint8_t len = (decoder->bit_cursor + cnt >= 8) ? 8 - decoder->bit_cursor : cnt;
    uint8_t data = (decoder->data_ptr[decoder->byte_cursor] >> decoder->bit_cursor) & ((1 << len) - 1);
    decoder->bit_cursor += len;
    // Jump to the next byte if the bit_cursor >= 8
    if (decoder->bit_cursor >= 8)
    {
        decoder->byte_cursor++;
        decoder->bit_cursor = 0;
        // Continue to read if the data in the last byte is not enough
        if (cnt > len)
        {
            uint8_t remaining_bits = cnt - len;
            uint8_t next_data = decoder->data_ptr[decoder->byte_cursor] & ((1 << remaining_bits) - 1);
            data |= (next_data << len);
            decoder->bit_cursor += remaining_bits;
        }
    }
    *target = data;
    return 0;
}

uint8_t __draw_line(struct GraphicsDecoder *decoder, uint32_t x, uint32_t y, uint32_t cnt, uint8_t is_background)
{
    // Write
    // printf("write: (%d, %d) cnt = %d is_background = %d\n", x, y, cnt, is_background);
    for (int i = 0; i < cnt; i++)
    {
        decoder->buffer[y][x + i] = !is_background;
    }
    return 0;
}

uint8_t __unsafe_write_buffer(struct GraphicsDecoder *decoder, uint8_t is_background, uint32_t cnt)
{
    uint32_t _cnt = cnt;
    uint32_t cur_remain, total_remain, current_cnt;
    uint32_t local_x = decoder->local_x, local_y = decoder->local_y, global_x, global_y;
    // Global start position
    uint32_t x_start = decoder->x_start, y_start = decoder->y_start;
    while (cnt > 0)
    {
        // printf("width = %d, local_x = %d\n", decoder->width, local_x);
        cur_remain = decoder->width - local_x;
        
        // Jump or not
        current_cnt = _cnt > cur_remain ? cur_remain : _cnt;
        // Get global x, y
        global_x = x_start + local_x;
        global_y = y_start + local_y;

        // printf("cur_remain: %d, current_cnt: %d, global_x: %d, global_y: %d, local_x: %d, local_y: %d, cnt: %d\n", cur_remain, current_cnt, global_x, global_y, local_x, local_y, _cnt);
        if (current_cnt > 0)
            if (__draw_line(decoder, global_x, global_y, current_cnt, is_background))
                return 1;
        if (_cnt <= cur_remain) {
            break;
        }
        _cnt -= cur_remain;
        local_x = 0;
        local_y++;
    }
    decoder->local_x = local_x + _cnt;
    decoder->local_y = local_y;
    return 0;
}

uint8_t rle_decode(struct GraphicsDecoder *decoder)
{
    uint8_t is_first_time = 1;
    if (decoder->width <= 0 || decoder->height <= 0)
    {
        return 1;
    }
    // TODO: Think it more versatile
    decoder->local_x = 0, decoder->local_y = 0;
    // Assume that the size of Graphics are always same
    // TODO: Add more macros here
    /*
    decoder->x_start += decoder->width;
    decoder->y_start -= decoder->height;
    */
    // TODO: Spare the space for the next Graphics
    // New Mode / Repeatition
    uint8_t flag;
    while (1)
    {
        // WRONG: CHECK IT CAREFULLY !!!
        if (decoder->local_y >= decoder->height || ((decoder->local_y == decoder->height - 1) && (decoder->local_x == decoder->width)))
            break;
        // printf("local_x = %d local_y = %d\n", decoder->local_x, decoder->local_y);
        if (is_first_time)
        {
            // printf("first time\n");
            flag = 0;
            is_first_time = 0;
        }
        else
            if (__unsafe_read_graphics_bits(decoder, &flag, 1))
                return 1;
        // Repeatition
        if (flag)
        {
            // printf("Repeatition\n");
            if (__unsafe_write_buffer(decoder, 1, decoder->last_bg))
                return 2; 
            if (__unsafe_write_buffer(decoder, 0, decoder->last_fg))
                return 3;
            // printf("last_bg = %d, last_fg = %d\n", decoder->last_bg, decoder->last_fg);
        }
        // New Mode
        else
        {
            // printf("New Mode\n");
            // Background first, Frontground second.
            for (uint32_t i = 0; i < 2; i++)
            {
                uint8_t *ground = i ? &decoder->last_fg : &decoder->last_bg;
                if (__unsafe_read_graphics_bits(decoder, ground, i ? decoder->rle_1 : decoder->rle_0))
                    return 4;
                if (__unsafe_write_buffer(decoder, !i, *ground))
                    return 5;
            }
        }
        // printf("------------------------------\n");
    }

    return 0;
}
/*
TODO: A more high level function to wrap all the operations
uint8_t prepare_graphics_decode(struct GraphicsDecoder *decoder)
{
    while (1)
    {
    }

    return 0;
}
*/

