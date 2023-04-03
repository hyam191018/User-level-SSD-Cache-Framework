#ifndef SPDK_H
#define SPDK_H

/*
 *  Author: Hyam
 *  Date: 2023/03/28
 *  Description: 使用SPDK存取 cache device
 */

#include "config.h"

/*
 * Description: get device info, after init_spdk
                    block_size(512, 4096)、device_size is number of lba
 * Return:  No return value
 */
void get_device_info(unsigned* block_size, unsigned long* device_size);

/*
 * Description: init spdk, find controller >create namespace > create qpair
 * Return:  Retrun 0, if success
 *          Retrun 1, if fail
 */
int init_spdk(void);

/*
 * Description: cleanup
 * Return:  No return value
 */
void exit_spdk(void);

/*
 * Description: allocate dma buffer for spdk io
 * Return:  Return address, if success
 *          Return NULL, if no free space
 */
void* alloc_dma_buffer(unsigned len);

/*
 * Description: free dma buffer
 * Return:  No return value
 */
void free_dma_buffer(void* dma_buf);

/*
 * Description: Read data from SPDK, using buffer allocated from alloc_amd_buffer
 * Return:  Return 0, if success
 *          Return non-zero, if fail
 */
int read_spdk(void* buf, unsigned long offset_block, unsigned num_block);

/*
 * Description: Write data to SPDK, using buffer allocated from alloc_amd_buffer
 * Return:  Return 0, if success
 *          Return non-zero, if fail
 */
int write_spdk(void* buf, unsigned long offset_block, unsigned num_block);

/*
 * Description: Perform data trimming and mark the block as invalid
 * Return:  Return 0, if success
 *          Return non-zero, if fail
 */
int trim_spdk(unsigned long offset_block, unsigned num_block);

#endif