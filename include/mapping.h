#ifndef MAPPING_H
#define MAPPING_H

/**
 *  @author Hyam
 *  @date 2023/03/19
 *  @brief mapping information
 */

#include "cache.h"

/**
 * @brief Init entrys, clean and dirty queue, hash table
 * @param mapping - Address of mapping table
 * @param block_size - 512, 4096 bytes
 * @param cblock_num - Cache block number
 * @return 0, if success
 *         non-zero, if fail
 */
int init_mapping(mapping *mapping, unsigned block_size, unsigned cblock_num);

/**
 * @brief Information of mapping table
 * @param mapping - Address of mapping table
 * @return 0, if success
 *         non-zero, if fail
 */
void info_mapping(mapping *mapping);

/**
 * @brief Try to promotion > demotion > writeback
 * @param mapping - Address of mapping table
 * @param dma_buf - Allocated by alloc_dma_buf
 * @return true, if done a work
 *         false, if donothing
 */
bool do_migration_work(mapping *mapping, void *dma_buf);

/**
 * @deprecated 功能問題，先不使用
 * @brief Try to writeback
 * @param mapping - Address of mapping table
 * @param dma_buf - Allocated by alloc_dma_buf
 * @return true, if done a work
 *         false, if donothing
 */
// bool do_writeback_work(mapping *mapping, void *dma_buf);

/**
 * @brief Search hash table
 * @param mapping - Address of mapping table
 * @param full_path_name - Key
 * @param page_index - Key
 * @param cblock - Store cache block, if lookup success
 * @return true, if hit
 *         false, if miss
 */
bool lookup_mapping(mapping *mapping, char *full_path_name, unsigned page_index, unsigned *cblock);

/**
 * @brief Search hash table, Insert cache page into hash table and set it as dirty,
 *        if miss
 * @param mapping - Address of mapping table
 * @param full_path_name - Key
 * @param page_index - key
 * @param cblock - Store cache block, if hit or insert success
 * @return true, if hit or insert success
 *         false, if no free entry
 */
bool lookup_mapping_with_insert(mapping *mapping, char *full_path_name, unsigned page_index,
                                unsigned *cblock);

/**
 * @brief Remove and free cached entry
 * @param mapping - Address of mapping table
 * @param full_path_name - Key
 * @param page_index - Key
 * @return true, if success
 *         false, if cache miss
 */
bool remove_mapping(mapping *mapping, char *full_path_name, unsigned page_index);

/**
 * @brief Set an entry dirty or clean, the entry must in hash table
 * @param mapping - Address of mapping table
 * @param cblock - Which cache block to setup
 * @param dirty - True for dirty, false for clean
 * @return No return value
 */
void set_dirty_after_write(mapping *mapping, unsigned *cblock, bool dirty);

#endif
