#ifndef MAPPING_H
#define MAPPING_H

/*
 *	Author: Hyam
 *	Date: 2023/03/19
 *	Description: 管理mapping資訊
 */

#include "cache_type.h"

/* in udm_cache_mapping.c */
int init_mapping(mapping* mapping, unsigned block_size, unsigned cblock_num);
int link_mapping(mapping* mapping);
int free_mapping(mapping* mapping);
int exit_mapping(void);
void info_mapping(mapping* mapping);

/*
 * Description: Search hash table
 * Return:  true, if hit
 *          false, if miss
 */
bool lookup_mapping(mapping *mapping, char *full_path_name, unsigned page_index, unsigned *cblock);

/*
 * Description: Search the hash table, if it is a miss, then insert it into the hash table and set it as dirty
 * Return:  true, if hit or insert success
 *          false, if no free entry
 */
bool lookup_mapping_with_insert(mapping *mapping, char *full_path_name, unsigned page_index, unsigned *cblock);

/*
 * Description: get an entry from free list
 * Return:  true, if success, entry index will store in cblock
 *          false, if no free entry
 */
bool promotion_get_free_cblock(mapping* mapping, char* full_path_name, unsigned cache_page_index, unsigned *cblock);

/*
 * Description: if success is true, put entry into clean list, otherwise, backto free list
 * Return:  No return value
 */
void promotion_complete(mapping* mapping, unsigned *cblock, bool success);

/*
 * Description: get an entry from clean list
 * Return:  true, if success, entry index will store in cblock
 *          false, if no clean entry or clean entrys > (cblock num / 2)
 */
bool demotion_get_clean_cblock(mapping* mapping, unsigned *cblock);

/*
 * Description: if success is true, put entry into free list, otherwise, backto clean list
 * Return:  No return value
 */
void demotion_complete(mapping* mapping, unsigned *cblock, bool success);

/*
 * Description: get an entry from dirty list
 * Return:  true, if success, entry index will store in cblock
 *          false, if no dirty entry
 */
bool writeback_get_dirty_cblock(mapping* mapping, unsigned *cblock);

/*
 * Description: if success is true, put entry into clean list, otherwise, backto dirty list
 * Return:  No return value
 */
void writeback_complete(mapping* mapping, unsigned *cblock, bool success);

/*
 * Description: set an entry to dirty or clean, the entry must in hash table
 * Return:  No return value
 */
void set_dirty_after_write(mapping* mapping, unsigned *cblock, bool dirty);




#endif
