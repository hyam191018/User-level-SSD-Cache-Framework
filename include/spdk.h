#ifndef SPDK_H
#define SPDK_H

/**
 *  Author: Hyam
 *  Date: 2023/03/28
 *  Description: 使用SPDK存取 cache device
 */

#include "config.h"

/* spdk setting */
#define NVME_ADDR "0000:04:00.0"
#define IODEPTH 32

typedef enum { IO_QUEUE, MG_QUEUE } queue_type;

/**
 * Description: Init spdk, find controller > create namespace > create qpair
 * Return:  Retrun 0, if success
 *          Retrun 1, if fail
 */
int init_spdk(void);

/**
 * Description: Cleanup
 * Return:  No return value
 */
void exit_spdk(void);

/**
 * Description: Get device info, must init spdk first
 * number of lba Return:  No return value
 */
void get_device_info(unsigned* block_size /*(512, 4096)*/,
                     unsigned long* device_size /* number of lba*/);

/**
 * Description: Allocate dma buffer for spdk io
 * Return:  address, if success
 *          NULL, if no free space
 */
void* alloc_dma_buffer(unsigned len);

/**
 * Description: Free dma buffer
 * Return:  No return value
 */
void free_dma_buffer(void* dma_buf);

/**
 * Description: Read from SPDK, using buffer allocated from alloc_amd_buffer
 * Return:  0, if success
 *          non-zero, if fail
 */
int read_spdk(void* buf, unsigned long offset_block, unsigned num_block, queue_type type);

/**
 * Description: Write to SPDK, using buffer allocated from alloc_amd_buffer
 * Return:  0, if success
 *          non-zero, if fail
 */
int write_spdk(void* buf, unsigned long offset_block, unsigned num_block, queue_type type);

/**
 * Description: Perform data trimming and mark the block as invalid
 * Return:  0, if success
 *          non-zero, if fail
 */
int trim_spdk(unsigned long offset_block, unsigned num_block);

#endif