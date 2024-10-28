#include "common.h"
#include "test_bit.h"
#include "string.h"
#include "time.h"
#include "stdint.h"
#include "assert.h"
#include "stdio.h"
#include "stdlib.h"

int main()
{
    /* Encode */
    uint32_t test_buffer_size = 10000000;
    uint8_t *strm = malloc(test_buffer_size * sizeof(uint8_t));

    srand((unsigned)time(NULL));

    struct RleBlock *test;
    if (rle_block_new(&test, test_buffer_size))
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
    rle_block_init(test);
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
    rle_block_init(test);
    rle_prepare_encode(test, 9, 6);
    printf("input: 9  6\n");
    printf("rle_0: %d  rle_1: %d\n", test->rle_0, test->rle_1);
    printf("output: %d  %d  %d\n", test->res_buf[0], test->res_buf[1], test->res_buf[2]);
    printf("//////////////////////////////////////////////\n");

    /* Test 4 */
    // RLE algorithm
    rle_block_init(test);
    // 9
    // 0000 1 00 1
    // 4    1 2  1
    strm[0] = (1 << 3) | (1);
    test->conf->each_size = 1;
    compress_conf_init(test);
    rle_compress_load(test, strm, 1);
    printf("input: 9\n");
    printf("output: %d  %d  %d  %d\n", test->rle_buf[0], test->rle_buf[1], test->rle_buf[2], test->rle_buf[3]);
    printf("//////////////////////////////////////////////\n");

    /* Test 5 */
    // RLE Compress
    rle_block_init(test);
    // 180
    // 1 0 11 0 1 00
    // 1 1 2  1 1 2
    memset(strm, 0, test_buffer_size);
    strm[0] = 180;
    test->conf->each_size = 1;
    compress_conf_init(test);
    rle_compress_load(test, strm, 1);
    printf("input: 180\n");
    printf("cnt: %d\n", test->conf->rle_cnt);
    printf("output: %d  %d  %d  %d  ", test->rle_buf[0], test->rle_buf[1], test->rle_buf[2], test->rle_buf[3]);
    printf("%d  %d  %d  %d\n", test->rle_buf[4], test->rle_buf[5], test->rle_buf[6], test->rle_buf[7]);
    printf("//////////////////////////////////////////////\n");

    /* Test 6 */
    // Compress and Encode
    rle_block_init(test);
    // 9
    // 0000 1 00 1
    // 4    1 2  1
    strm[0] = 9;
    // 6
    // 00000 11 0
    // 5     2  1
    strm[1] = 6;
    test->conf->each_size = 1;
    compress_conf_init(test);
    rle_compress_load(test, strm, 1);
    test->rle_0 = 1;
    test->rle_1 = 1;
    for (int i = 0; i < test->conf->rle_cnt; i += 2)
    {
        rle_prepare_encode(test, test->rle_buf[i], test->rle_buf[i + 1]);
    }
    printf("input: 9  6\n");
    printf("rle_0: %d  rle_1: %d\n", test->rle_0, test->rle_1);
    printf("output: %d  %d  %d  %d  %d\n", test->res_buf[0], test->res_buf[1], test->res_buf[2], test->res_buf[3], test->res_buf[4]);
    printf("//////////////////////////////////////////////\n");

    /* Test 7 */
    // Best Volume Test
    rle_block_init(test);
    // 9
    // 0000 1 00 1
    // 4    1 2  1
    strm[0] = 9;
    // 6
    // 00000 11 0
    // 5     2  1
    strm[1] = 6;
    test->conf->each_size = 2;
    compress_conf_init(test);
    rle_compress_load(test, strm, 1);
    uint32_t min_size;
    rle_get_min_size(test, &min_size);
    printf("input: 9  6\n");
    printf("best_rle_0: %d\nbest_rle_1: %d\nmin_size: %d bytes\n", test->rle_0, test->rle_1, min_size);
    printf("//////////////////////////////////////////////\n");

    /* Test 8 */
    // Ascii Performance Test
    // data: 34200 bytes -> 16020 bytes (46.842105%)
    memset(strm, 0, test_buffer_size * sizeof(uint8_t));
    rle_block_init(test);
    compress_conf_init(test);

    test->conf->each_size = sizeof(ascii);
    for (size_t i = 0; i < test->conf->each_size; i++)
    {
        strm[i] = ascii[i];
    }
    clock_t start = clock();
    rle_compress_load(test, strm, 1);
    rle_get_min_size(test, &min_size);
    for (int i = 0; i < test->conf->rle_cnt; i += 2)
    {
        rle_prepare_encode(test, test->rle_buf[i], test->rle_buf[i + 1]);
    }
    assert(test->byte_cursor + 1 == min_size);
    printf("data: %d bytes -> %d bytes (%llf%%)\n", test->conf->each_size, min_size, ((double)min_size / (double)test->conf->each_size) * 100);
    printf("best_rle_0: %d\nbest_rle_1: %d\n", test->rle_0, test->rle_1);
    printf("It takes %f secs\n", (double)(clock() - start) / CLOCKS_PER_SEC);
    printf("//////////////////////////////////////////////\n");
    /*
        printf("-------------Source------------------\n");
        for (int i = 0; i < test->conf->each_size; i++) {
            printf("%d ", strm[i]);
        }
        printf("\n-------------RLE code----------------\n");
        for (int i = 0; i < test->conf->each_size; i++) {
            printf("%d ", test->rle_buf[i]);
        }
        printf("\n-------------Result code-------------\n");
        for (int i = 0; i < test->conf->each_size; i++) {
            printf("%d ", test->res_buf[i]);
        }
        printf("\n");
    */
    rle_block_free(test);
    free(strm);

    /* Decode */
    struct GraphicsDecoder *gd;
    /* Test 9 */
    // READ
    uint8_t *_test = malloc(5 * sizeof(uint8_t));
    _test[0] = 35;
    rle_decoder_new(&gd, 100, 100, _test, 2, 2, 4, 2);
    rle_decoder_setup(gd);
    uint8_t res;
    for (int i = 0; i < 3; i++)
    {
        __unsafe_read_graphics_bits(gd, &res, 2);
        printf("res = %d\n", res);
    }
    printf("//////////////////////////////////////////////\n");

    /* Test 10 */
    // WRITE
    rle_decoder_setup(gd);
    gd->width = 4;
    printf("%d\n", __unsafe_write_buffer(gd, 0, 7));
    printf("//////////////////////////////////////////////\n");

    /* Test 11 */
    // DECODE
    // 13
    // 0000 1101
    _test[0] = 109;
    _test[1] = 25;
    gd->width = 4;
    gd->height = 2;
    gd->x_start = 0, gd->y_start = 0;
    gd->rle_0 = 1, gd->rle_1 = 1;
    rle_decoder_setup(gd);
    rle_decode(gd);
    for (int i = 0; i < 2; i++)
    {
        for (int k = 0; k < 4; k++)
        {
            printf("%d ", gd->buffer[i][k]);
        }
        printf("\n");
    }
    printf("//////////////////////////////////////////////\n");
    rle_decoder_free(gd);
    free(_test);

    /* Test 12 */
    // ENCODE and DECODE
    struct RleBlock *encoder;
    struct GraphicsDecoder *decoder;
    uint8_t *stream = malloc(10 * sizeof(uint8_t));
    memset(stream, 0, 10 * sizeof(uint8_t));
    stream[0] = 9;
    stream[1] = 6;
    stream[2] = 255;
    rle_block_new(&encoder, 100);
    encoder->conf->each_size = 3;
    compress_conf_init(encoder);
    rle_compress_load(encoder, stream, 1);
    rle_get_min_size(encoder, &min_size);
    for (int i = 0; i < encoder->conf->rle_cnt; i += 2)
    {
        rle_prepare_encode(encoder, encoder->rle_buf[i], encoder->rle_buf[i + 1]);
    }
    rle_decoder_new(&decoder, 10, 10, encoder->res_buf, encoder->rle_0, encoder->rle_1, 4, 6);
    decoder->x_start = 0, decoder->y_start = 0;
    rle_decoder_setup(decoder);
    rle_decode(decoder);
    for (int i = 0; i < 6; i++)
    {
        for (int k = 0; k < 4; k++)
        {
            printf("%d ", decoder->buffer[i][k]);
        }
        printf("\n");
    }
    printf("//////////////////////////////////////////////\n");
}