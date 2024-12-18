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
 * Time: 2004.10.20
 *
 * Modifications made in [2024] by: Rm
 */
#include "common.h"

#include "assert.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include <stdint.h>
#include "time.h"

uint8_t rle_block_new(struct RleBlock **self, uint32_t data_size, uint32_t block_num)
{
    struct RleBlock *new;
    if (rle_block_alloc(&new, data_size, block_num))
        return 1;
    if (rle_block_init(new, block_num))
        return 1;
    *self = new;
    return 0;
}
uint8_t rle_block_alloc(struct RleBlock **self, uint32_t total_data_size, uint32_t block_num)
{
    struct RleBlock *s = malloc(sizeof(struct RleBlock));
    if (s == NULL)
        goto FAIL_SELF;
    // The worst case is that the data completely unrepeated
    // Also, consider the flag bit.
    s->res_buf = malloc((8 + 1) * total_data_size * (sizeof(char)));
    if (s->res_buf == NULL)
        goto FAIL_RES;
    s->conf = malloc(sizeof(struct RleCompressConf));
    if (s->conf == NULL)
        goto FAIL_CONF;
    // TODO: Optimize the size
    s->rle_buf = malloc(8 * total_data_size * (sizeof(char)));
    if (s->rle_buf == NULL)
        goto FAIL_RLE;
    s->conf->rle_cnt = malloc(block_num * (sizeof(uint32_t)));
    if (s->conf->rle_cnt == NULL)
        goto FAIL_RLE_SIZE_ARR;
    s->lookup_table = malloc((block_num + 1) * sizeof(struct LookupTableItem));
    if (s->lookup_table == NULL)
        goto FAIL_LOOKUP_TABLE;
    memset(s->res_buf, 0, (8 + 1) * total_data_size * (sizeof(char)));
    memset(s->rle_buf, 0, 8 * total_data_size * (sizeof(char)));
    s->res_msize = (8 + 1) * total_data_size * (sizeof(char));
    *self = s;
    return 0;

FAIL_LOOKUP_TABLE:
    free(s->conf->rle_cnt);
FAIL_RLE_SIZE_ARR:
    free(s->rle_buf);
FAIL_RLE:
    free(s->conf);
FAIL_CONF:
    free(s->res_buf);
FAIL_RES:
    free(s);
FAIL_SELF:
    return 1;
}
uint8_t rle_block_init(struct RleBlock *self, uint32_t block_num)
{
    memset(self->res_buf, 0, self->res_msize);
    memset(self->lookup_table, 0, block_num * sizeof(struct LookupTableItem));
    self->last_a = 0;
    self->last_b = 0;
    self->bit_cursor = 0;
    self->byte_cursor = 0;
    self->is_first_time = 1;
    self->conf->num = block_num;
    return 0;
}
uint8_t compress_conf_init(struct RleBlock *self)
{
    self->conf->cur_cnt = 0;
    self->conf->total_rle_cnt = 0;
    memset(self->rle_buf, 0, (self->res_msize * 8) / 9);
    memset(self->conf->rle_cnt, 0, self->conf->num * sizeof(uint32_t));
    return 0;
}
uint8_t rle_block_free(struct RleBlock *self)
{
    if (self->res_buf != NULL)
    {
        free(self->res_buf);
        self->res_buf = NULL;
    }
    free(self);
    self = NULL;
    return 0;
}

uint8_t rle_compress_stream(struct RleBlock *target, uint8_t *stream, uint32_t index)
{
    uint32_t last_is_one = 0, cur_len = 0, cnt = target->conf->total_rle_cnt;
    for (int i = 7; i >= 0; i--)
    {
        // 0
        if ((stream[index] & (1 << i)) == 0)
        {
            if (last_is_one)
            {
                target->rle_buf[cnt] = cur_len;
                // printf("write: %d\n", cur_len);
                cur_len = 0;
                last_is_one = 0;
                cnt++;
            }
            cur_len++;
        }
        // 1
        else
        {
            if (!last_is_one)
            {
                target->rle_buf[cnt] = cur_len;
                // printf("write: %d\n", cur_len);
                cur_len = 0;
                last_is_one = 1;
                cnt++;
            }
            cur_len++;
        }
    }
    /* Handle the last element */
    target->rle_buf[cnt] = cur_len;
    cnt++;
    // Cause Problem
    // When the last one is not for frontend, we plus it manually.
    if (cnt & 1 == 1)
    {
        target->rle_buf[cnt] = 0;
        cnt++;
    }
    target->conf->rle_cnt[target->conf->cur_cnt] += cnt - target->conf->total_rle_cnt;
    target->conf->total_rle_cnt = cnt;
    return 0;
}

uint8_t rle_compress_load(struct RleBlock *target, uint8_t *stream, uint32_t len)
{
    assert(len <= target->conf->num);
    target->conf->cur_cnt = 0;
    for (int i = 0; i < len; i++)
    {
        for (uint32_t k = 0; k < target->conf->each_size; k++)
        {
            if (rle_compress_stream(target, stream, i * target->conf->each_size + k))
                return 1;
        }
        target->conf->cur_cnt ++;
    }

    return 0;
}
uint8_t rle_try_best_rle(struct RleBlock *target)
{
    rle_get_min_size(target, NULL);
    return 0;
}
uint8_t rle_get_min_size(struct RleBlock *target, uint32_t *size)
{
    uint32_t min_volume = 0xffffffff;
    uint8_t best_rle_0 = 0, best_rle_1 = 0;
    for (int cur_rle_0 = 1; cur_rle_0 < 8; cur_rle_0++)
    {
        for (int cur_rle_1 = 1; cur_rle_1 < 8; cur_rle_1++)
        {
            target->rle_0 = cur_rle_0;
            target->rle_1 = cur_rle_1;
            for (int t = 0; t < target->conf->num; t++)
            {
                for (int s = 0; s < target->conf->rle_cnt[t]; s += 2)
                {
                    if (rle_prepare_encode(target, target->rle_buf[s], target->rle_buf[s + 1]))
                        return 1;
                }
            }

            if (min_volume > target->byte_cursor + 1)
            {
                min_volume = target->byte_cursor + 1;
                best_rle_0 = cur_rle_0, best_rle_1 = cur_rle_1;
                // printf("best_rle_0: %d, best_rle_1: %d\n", best_rle_0, best_rle_1);
                // printf("min_volume = %d bytes, best_rle_0 = %d, best_rle_1 = %d\n", min_volume, cur_rle_0, cur_rle_1);
            }
            // Optimize it
            rle_block_init(target, target->conf->num);
        }
    }
    target->rle_0 = best_rle_0, target->rle_1 = best_rle_1;
    if (size != NULL)
        *size = min_volume;
    return 0;
}
uint8_t rle_encode(struct RleBlock *target)
{
    target->lookup_table[0].bit_offset = 0;
    target->lookup_table[0].byte_offset = 0;
    uint32_t index = 0;
    for (int t = 0; t < target->conf->num; t++)
    {
        for (int s = 0; s < target->conf->rle_cnt[t]; s += 2)
        {
            if (rle_prepare_encode(target, target->rle_buf[index + s], target->rle_buf[index + s + 1]))
                return 1;
        }
        target->lookup_table[t + 1].byte_offset = target->byte_cursor;
        target->lookup_table[t + 1].bit_offset = target->bit_cursor;
        target->lookup_table[t + 1].str[0] = t + 1;
        index += target->conf->rle_cnt[t];
        printf("%d : update lookup table: %d %d\n", t, target->byte_cursor, target->bit_cursor);
    }
    target->lookup_table[target->conf->num].byte_offset = target->byte_cursor;
    target->lookup_table[target->conf->num].bit_offset = target->bit_cursor;
    target->lookup_table[target->conf->num].str[0] = target->conf->num + 1;
    return 0;
}

uint8_t rle_write_bits(struct RleBlock *target, uint8_t data, uint8_t len)
{
    assert(data < (1 << len));
    assert(target->byte_cursor <= target->res_msize - 1);
    // Write (8 - bit_cursor) bits into buffer here.
    // So, there's maybe (len - (8 - bit_cursor)) bits left.
    target->res_buf[target->byte_cursor] |= (data << target->bit_cursor);
    if (target->bit_cursor + len >= 8)
    {
        target->byte_cursor++;
        // Write those overflow bits.
        target->res_buf[target->byte_cursor] = (data >> (8 - target->bit_cursor));
        // (len - (8 - bit_cursor)) bits written.
        // means that (bit_cursor + len - 8) is the result
        target->bit_cursor += len;
        target->bit_cursor -= 8;
    }
    // It may be luckily enough
    else
        target->bit_cursor += len;
    return 0;
}
uint8_t __rle_encode(struct RleBlock *target, uint8_t a, uint8_t b)
{
    if (target->is_first_time == 0 && target->last_a == a && target->last_b == b)
    {
        if (rle_write_bits(target, 1, 1))
            return 1;
    }
    else
    {
        if (target->is_first_time == 0)
        {
            if (rle_write_bits(target, 0, 1))
                return 1;
        }
        if (rle_write_bits(target, a, target->rle_0))
            return 1;
        if (rle_write_bits(target, b, target->rle_1))
            return 1;
        target->last_a = a;
        target->last_b = b;
        target->is_first_time = 0;
    }
    return 0;
}
uint8_t rle_prepare_encode(struct RleBlock *target, uint8_t a, uint8_t b)
{
    uint8_t rle_0 = target->rle_0;
    uint8_t rle_1 = target->rle_1;
    // Don't use (a >= (1 << rle_0) - 1)
    // Because mostly we think that combining a and b is a better choice to save memory
    while (a > (1 << rle_0) - 1)
    {
        if (__rle_encode(target, (1 << rle_0) - 1, 0))
            return 1;
        a -= (1 << rle_0) - 1;
    }
    while (b > (1 << rle_1) - 1)
    {
        if (__rle_encode(target, a, (1 << rle_1) - 1))
            return 1;
        b -= (1 << rle_1) - 1;
        a = 0;
    }
    if (a != 0 || b != 0)
        if (__rle_encode(target, a, b))
            return 1;
    return 0;
}
