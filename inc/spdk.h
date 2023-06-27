#ifndef SPDK_H
#define SPDK_H

/**
 *  @author Hyam
 *  @date 2023/03/28
 *  @brief 使用SPDK存取 cache device
 */

/* spdk setting */
#include "stdinc.h"
#define NVME_ADDR "0000:04:00.0"
#define IODEPTH 1
#define QPAIR_COUNT 2
typedef enum { IO_QUEUE, MG_QUEUE } queue_type;

/**
 * @brief Init spdk, find controller > create namespace > create qpairs
 * @return 0, if success
 *         non-zero, if fail
 */
int init_spdk(void);

/**
 * @brief Cleanup
 * @return No return value
 */
void exit_spdk(void);

/**
 * @brief Get device info, after init_spdk
 * @param block_size - 512, 4096 bytes
 * @param device_size - Number of lba
 * @return No return value
 */
void get_device_info(unsigned* block_size, unsigned long* device_size);

/**
 * @brief Allocate dma buffer for spdk io
 * @param len - The size in bytes of the DMA buffer to allocate
 * @return address, if success
 *         NULL, if no free space
 */
void* alloc_dma_buffer(unsigned len);

/**
 * @brief Free dma buffer.
 * @param dma_buf - Allocated by alloc_dma_buf
 * @return No return value.
 */
void free_dma_buffer(void* dma_buf);

/**
 * @brief Read from SPDK
 * @param dma_buf - Allocated by alloc_dma_buf
 * @param offset_block - Start lba to perform io
 * @param num_block - length of block to read
 * @param type - queue for different threads
 * @return 0, if success
 *         non-zero, if fail
 */
int read_spdk(void* dma_buf, unsigned long offset_block, unsigned num_block, queue_type type);

/**
 * @brief Write to SPDK
 * @param dma_buf - Allocated by alloc_dma_buf
 * @param offset_block - Start lba to perform io
 * @param num_block - length of block to write
 * @param type - queue for different threads
 * @return 0, if success
 *         non-zero, if fail
 */
int write_spdk(void* dma_buf, unsigned long offset_block, unsigned num_block, queue_type type);

/**
 * @brief Perform data trimming and mark the block as invalid
 * @param offset_block - Start lba to perform io
 * @param num_block - length of block to read
 * @param type - queue for different threads
 * @return 0, if success
 *         non-zero, if fail
 */
int trim_spdk(unsigned long offset_block, unsigned num_block, queue_type type);

/**
 * @brief Read from SPDK with iovector
 * @param iov - iovector with PAGE_SIZE
 * @param iov_cnt - Number of iovector
 * @param offset_block - Start lba to perform io
 * @param num_block - length of block to read
 * @param type - queue for different threads
 * @return 0, if success
 *         non-zero, if fail
 */
int readv_spdk(struct iovec* iov, int iov_cnt, unsigned long offset_block, unsigned num_block,
               queue_type type);

/**
 * @brief Write to SPDK with iovector
 * @param iov - iovector with PAGE_SIZE
 * @param iov_cnt - Number of iovector
 * @param offset_block - Start lba to perform io
 * @param num_block - length of block to write
 * @param type - queue for different threads
 * @return 0, if success
 *         non-zero, if fail
 */
int writev_spdk(struct iovec* iov, int iov_cnt, unsigned long offset_block, unsigned num_block,
                queue_type type);

#endif