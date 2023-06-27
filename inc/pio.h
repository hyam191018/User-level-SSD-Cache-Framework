#ifndef PIO_H
#define PIO_H

/**
 *  @author Hyam
 *  @date 2023/03/17
 *  @brief 對 page io的建立與操作，以便後續使用submit_pio向udm-cache提交
 */

#include "config.h"
#include "stdinc.h"

/* PIO是一個Linked list，必須小於或等於cache，page大小(32KB)，且必須對齊在一個cache page以內 */
struct pio {
    char* full_path_name;
    int fd;               // for fio
    unsigned page_index;  // file offset = page_index * page_size
    operate operation;    // READ, WRITE, DISCARD
    void* buffer;         // user memory space
    unsigned pio_cnt;     // pio nodes count
    struct pio* next;     // point to next pio
};

/**
 * @brief Create a pio head
 * @param full_path_name - File name
 * @param fd - File descriptor, for fio test
 * @param page_index - Offset of file, size is 4KB
 * @param operation - READ, WRITE, DISCARD, FLUSH
 * @param buffer - User buffer, must allocated by alloc_dma_buf
 * @param pio_cnt - Number of pio node
 * @return pio address, if success
 *         NULL, if fail
 */
struct pio* create_pio(char* full_path_name, int fd, unsigned page_index, operate operation,
                       void* buffer, unsigned pio_cnt);

/**
 * @brief Append pio node
 * @param head - Address of pio
 * @param buffer - User buffer, must allocated by alloc_dma_buf
 * @return No return value
 */
void append_pio(struct pio* head, void* buffer);

/**
 * @brief Free all pio nodes
 * @param head - Address of pio
 * @return No return value
 */
void free_pio(struct pio* head);

/**
 * @brief Print pio information
 * @param head - Address of pio
 * @return No return value
 */
void print_pio(struct pio* head);

#endif
