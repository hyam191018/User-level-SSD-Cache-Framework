#ifndef PIO_H
#define PIO_H

/*
 *  Author: Hyam
 *  Date: 2023/03/17
 *  Description: 對 page io的建立與操作，以便後續使用submit_pio向udm-cache提交
 */

#include "config.h"
#include "stdinc.h"

/* PIO是一個Linked list，必須小於或等於cache
            page大小(32KB)，且必須對齊在一個cache page以內 */
struct pio {
    char* full_path_name;
    unsigned page_index;  // file offset = page_index * page_size
    operate operation;    // READ, WRITE, DISCARD
    void* buffer;         // user memory space
    unsigned pio_cnt;     // pio nodes count
    struct pio* next;     // point to next pio
};

/*
 * Description: create a pio head
 * Return:  pio address, if success
 *          NULL, if fail
 */
struct pio* create_pio(char* full_path_name, unsigned page_index, operate operation, void* buffer,
                       unsigned pio_cnt);

/*
 * Description: append pio to tail
 * Return:  No return value
 */
void append_pio(struct pio* head, void* buffer);

/*
 * Description: free all pio nodes
 * Return:  No return value
 */
void free_pio(struct pio* head);

/*
 * Description: print pio info
 * Return:  No return value
 */
void print_pio(struct pio* head);

#endif
