#ifndef MAPPING_H
#define MAPPING_H

/**
 *  Author: Hyam
 *  Date: 2023/03/19
 *  Description: mapping information
 */

#include "cache_type.h"

int init_mapping(mapping *mapping, unsigned block_size, unsigned cblock_num);
// int link_mapping(mapping *mapping);
void info_mapping(mapping *mapping);

/**
 * Description: Try to promotion > demotion > writeback
 * Return:  true, if done a work
 *          false, if donothing
 */
bool do_migration_work(mapping *mapping, void *dma_buf);

/**
 * Description: Try to writeback
 * Return:  true, if done a work
 *          false, if donothing
 */
bool do_writeback_work(mapping *mapping, void *dma_buf);

/**
 * Description: Search hash table
 * Return:  true, if hit
 *          false, if miss
 */
bool lookup_mapping(mapping *mapping, char *full_path_name, unsigned page_index, unsigned *cblock);

/**
 * Description: Search hash table, Insert cache page into hash table and set it as dirty,
 *                  if miss
 * Return:  true, if hit or insert success
 *          false, if no free entry
 */
bool lookup_mapping_with_insert(mapping *mapping, char *full_path_name, unsigned page_index,
                                unsigned *cblock);

/**
 * Description: Set an entry dirty or clean, the entry must in hash table
 * Return:  No return value
 */
void set_dirty_after_write(mapping *mapping, unsigned *cblock, bool is_write);

#endif
