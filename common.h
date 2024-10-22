#ifndef __COMMMON__H__
#define __COMMMON__H__

#include "stdint.h"

struct RleCompressConf {
    uint8_t offset;
    uint32_t each_size;
    uint32_t rle_cnt;
    uint32_t num;
};
struct RleBlock {
    /**/
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
 

/*
// Online C Compiler - Build, Compile and Run your C programs online in your favorite browser

#include <stdio.h>
#include "stdint.h"
#include "stdlib.h"

uint8_t elr_0, elr_1;
struct ElrBlock {
    uint8_t* list;
    uint8_t rle_0, rle_1;
    uint8_t bit_cursor;
    uint8_t byte_cursor;
    uint8_t rle_bit_cnt;
    // init to 1 
    uint8_t is_first_time;
    uint8_t last_a;
    uint8_t last_b;
};
struct ElrBlock* List;
uint8_t *l;
int write_byte(struct ElrBlock* target, uint8_t data, uint8_t len) {
    target->list[target->byte_cursor] |= data;
    return 0;
}
int encode_elr(struct ElrBlock* target, uint8_t x, uint8_t y) {
    if (target->is_first_time == 0 && target->last_a == a && target->last_b == b) {
        write_byte(target, 1, 1);
        target->rle_bit_cnt ++;
    }
    else {
        if (target->is_first_time == 0) {
            write_byte(target, 0, 1);
        }
        write_byte(target, 0, 0);
        write_byte(target, a, target->rle_0);
        write_byte(target, b, target->rle_1);
        
        target->last_a = a;
        target->last_b = b;
        target->is_first_time = 0;
        target->rle_bit_cnt ++;
    }
    return 0;
}
int ELR(uint8_t a, uint8_t b) {
    while (a > (1 << elr_0) - 1) {
        a -= (1 << elr_0) - 1;
        encode_elr(List, (1 << elr_0) - 1, 0);
    }
    while (b > (1 << elr_1) - 1) {
        b -= (1 << elr_1) - 1;
        encode_elr(List, a, (1 << elr_1) - 1);
        a = 0;
    }
    if (a != 0 || b != 0) {
        encode_elr(List, a, b);
    }
    return 0;
}
int main()
{
    printf("Welcome to Online IDE!! Happy Coding :)");
    l = malloc(100000);
    l[0] = (1 << 7) & (1 << 3);
    return 0;
}


*/
#endif