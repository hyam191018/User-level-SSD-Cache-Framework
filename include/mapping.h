#ifndef MAPPING_H
#define MAPPING_H

/*
 *  Author: Hyam
 *  Date: 2023/03/19
 *  Description: 管理mapping資訊
 */

#include "cache_type.h"

int init_mapping(mapping *mapping, unsigned block_size, unsigned cblock_num);
int link_mapping(mapping *mapping);
int free_mapping(mapping *mapping);
int exit_mapping(void);
void info_mapping(mapping *mapping);

/*
 * Description: promotion > demotion > writeback
 * Return:  true, if done a work
 *          false, if donothing
 */
bool do_migration_work(mapping *mapping);

/*
 * Description: try to do writeback
 * Return:  true, if done a work
 *          false, if donothing
 */
bool do_writeback_work(mapping *mapping);
/*
 * Description: Search hash table
 * Return:  true, if hit
 *          false, if miss
 */
bool lookup_mapping(mapping *mapping, char *full_path_name, unsigned page_index, unsigned *cblock);

/*
 * Description: Search the hash table, if it is a miss, then insert it into the
 *              hash table and set it as dirty
 * Return:  true, if hit or insert success
 *          false, if no free entry
 */
bool lookup_mapping_with_insert(mapping *mapping, char *full_path_name, unsigned page_index,
                                unsigned *cblock);

/*
 * Description: set an entry to dirty or clean, the entry must in hash table
 * Return:  No return value
 */
void set_dirty_after_write(mapping *mapping, unsigned *cblock, bool is_write);

#endif
