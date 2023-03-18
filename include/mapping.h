#ifndef MAPPING_H
#define MAPPING_H

/*
 *	Author: Hyam
 *	Date: 2023/03/17
 *	Description: 管理mapping資訊
 */

#include "cache_type.h"

/* in mapping.c */
int init_mapping(mapping* mapping, unsigned block_size, unsigned cblock_num);   // open and init: return 0 if success
int link_mapping(mapping* mapping);   // opan and map
int free_mapping(mapping* mapping);   // unmap
int exit_mapping(void);   // unlink
void info_mapping(mapping* mapping);

/* return true, if hit */
bool lookup_mapping(mapping *mapping, char *full_path_name, unsigned page_index, unsigned *cblock);

/* return true, if hit or insert success */
bool lookup_mapping_with_insert(mapping *mapping, char *full_path_name, unsigned page_index, unsigned *cblock);

/* return true, if get free entry 
            false, if no free entry */
bool promotion_get_free_cblock(mapping* mapping, char* full_path_name, unsigned cache_page_index, unsigned *cblock);
void promotion_complete(mapping* mapping, unsigned *cblock, bool success);

/* return true, if get clean entry */
bool demotion_get_clean_cblock(mapping* mapping, unsigned *cblock);
void demotion_complete(mapping* mapping, unsigned *cblock, bool success);

/* return true, if get dirty entry */
bool writeback_get_dirty_cblock(mapping* mapping, unsigned *cblock);
void writeback_complete(mapping* mapping, unsigned *cblock, bool success);

/* return true, if success */
bool set_dirty_after_write(mapping* mapping, unsigned *cblock, bool dirty);




#endif
