#include "common.h"

#include "assert.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

uint8_t rle_new(struct RleBlock **self, uint8_t result_size)
{
    struct RleBlock *new;
    if (rle_alloc(&new, result_size))
        return 1;
    if (rle_init(new))
        return 1;
    *self = new;
    return 0;
}
uint8_t rle_alloc(struct RleBlock **self, uint8_t result_size)
{
    struct RleBlock *s = malloc(sizeof(struct RleBlock));
    if (s == NULL)
        goto FAIL_SELF;
    s->res_buf = malloc(result_size * (sizeof(char)));
    if (s->res_buf == NULL)
        goto FAIL_RES;
    s->conf = malloc(sizeof(struct RleCompressConf));
    if (s->conf == NULL)
        goto FAIL_CONF;
    s->rle_buf = malloc(result_size * 8);
    if (s->rle_buf == NULL)
        goto FAIL_RLE;
    memset(s->res_buf, 0, result_size);
    memset(s->rle_buf, 0, result_size * 8);
    s->res_msize = result_size;
    *self = s;
    return 0;
FAIL_RLE:
    free(s->conf);
FAIL_CONF:
    free(s->res_buf);
FAIL_RES:
    free(s);
FAIL_SELF:
    return 1;
}
uint8_t rle_init(struct RleBlock *self)
{
    memset(self->res_buf, 0, self->res_msize);
    memset(self->rle_buf, 0, 8 * self->res_msize);
    self->last_a = 0;
    self->last_b = 0;
    self->bit_cursor = 0;
    self->byte_cursor = 0;
    self->is_first_time = 1;
    self->rle_bit_cnt = 0;
    return 0;
}
uint8_t rle_free(struct RleBlock *self)
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
uint8_t rle_compress_stream(struct RleBlock *target, uint8_t *stream, uint8_t index)
{
    uint8_t last_is_one = 0, cur_len = 0, cnt = target->conf->rle_cnt;
    for (int i = 7; i >= 0; i--)
    {
        // 0
        if ((stream[index] & (1 << i)) == 0)
        {
            if (last_is_one)
            {
                target->rle_buf[cnt] = cur_len;
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
    if (cnt & 1 == 1)
    {
        target->rle_buf[cnt] = 0;
        cnt++;
    }
    target->conf->rle_cnt += cnt;
    return 0;
}

uint8_t rle_compress_load(struct RleBlock *target, uint8_t *stream, uint8_t len)
{
    for (int i = 0; i < len; i++)
    {
        for (int k = 0; k < target->conf->each_size; k++)
        {
            printf("index in buf: %d\n", k);
            if (rle_compress_stream(target, stream, i * target->conf->each_size + k))
                return 1;
        }
    }

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
    {
        target->bit_cursor += len;
    }
    return 0;
}
uint8_t rle_encode(struct RleBlock *target, uint8_t a, uint8_t b)
{
    if (target->is_first_time == 0 && target->last_a == a && target->last_b == b)
    {
        if (rle_write_bits(target, 1, 1))
            return 1;
        target->rle_bit_cnt++;
    }
    else
    {
        if (target->is_first_time == 0)
        {
            if (rle_write_bits(target, 0, 1))
                return 1;
            target->rle_bit_cnt++;
        }
        if (rle_write_bits(target, a, target->rle_0))
            return 1;
        if (rle_write_bits(target, b, target->rle_1))
            return 1;
        target->last_a = a;
        target->last_b = b;
        target->is_first_time = 0;
        target->rle_bit_cnt += target->rle_0;
        target->rle_bit_cnt += target->rle_1;
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
        if (rle_encode(target, (1 << rle_0) - 1, 0))
            return 1;
        a -= (1 << rle_0) - 1;
    }
    while (b > (1 << rle_1) - 1)
    {
        if (rle_encode(target, a, (1 << rle_1) - 1))
            return 1;
        b -= (1 << rle_1) - 1;
        a = 0;
    }
    if (a != 0 || b != 0)
        if (rle_encode(target, a, b))
            return 1;
    return 0;
}

int main()
{
    struct RleBlock *test;
    if (rle_new(&test, 100))
    {
        printf("Error: RleBlock malloc failed\n");
        return -1;
    }
    printf("//////////////////////////////////////////////\n");
    /* Test 1 */
    // encode
    // no overflow, no repeatition
    test->rle_0 = 3;
    test->rle_1 = 2;
    rle_prepare_encode(test, 6, 2);
    printf("input: 6  2\n");
    printf("rle_0: %d  rle_1: %d\n", test->rle_0, test->rle_1);
    printf("output: %d  %d  %d\n", test->res_buf[0], test->res_buf[1], test->res_buf[2]);
    printf("//////////////////////////////////////////////\n");
    /* Test 2 */
    // encode
    // overflow, no repeatition
    test->rle_0 = 3;
    test->rle_1 = 2;
    rle_init(test);
    rle_prepare_encode(test, 9, 6);
    printf("input: 9  6\n");
    printf("rle_0: %d  rle_1: %d\n", test->rle_0, test->rle_1);
    printf("output: %d  %d  %d\n", test->res_buf[0], test->res_buf[1], test->res_buf[2]);
    printf("//////////////////////////////////////////////\n");

    /* Test 3 */
    // encode
    // overflow and repeatition
    test->rle_0 = 2;
    test->rle_1 = 2;
    rle_init(test);
    rle_prepare_encode(test, 9, 6);
    printf("input: 9  6\n");
    printf("rle_0: %d  rle_1: %d\n", test->rle_0, test->rle_1);
    printf("output: %d  %d  %d\n", test->res_buf[0], test->res_buf[1], test->res_buf[2]);
    printf("//////////////////////////////////////////////\n");

    /* Test 4 */
    // RLE algorithm
    rle_init(test);
    uint8_t *strm = malloc(5 * sizeof(uint8_t));
    // 9
    // 0000 1 00 1
    // 4    1 2  1
    strm[0] = (1 << 3) | (1);
    test->conf->each_size = 1;
    test->conf->rle_cnt = 0;
    rle_compress_load(test, strm, 1);
    printf("input: 9\n");
    printf("output: %d, %d, %d, %d\n", test->rle_buf[0], test->rle_buf[1], test->rle_buf[2], test->rle_buf[3]);
    printf("//////////////////////////////////////////////\n");

    /* Test 5 */
    // RLE algorithm
    rle_init(test);
    // 9
    // 0000 1 00 1
    // 4    1 2  1
    strm[0] = 9;
    // 6
    // 00000 11 0
    // 5     2  1
    strm[1] = 6;
    test->conf->each_size = 2;
    test->conf->rle_cnt = 0;
    rle_compress_load(test, strm, 1);
    printf("input: 9  6\n");
    printf("output: %d, %d, %d, %d, ", test->rle_buf[0], test->rle_buf[1], test->rle_buf[2], test->rle_buf[3]);
    printf("%d, %d, %d, %d\n", test->rle_buf[4], test->rle_buf[5], test->rle_buf[6], test->rle_buf[7]);
    printf("//////////////////////////////////////////////\n");

    rle_free(test);
    return 0;
}
