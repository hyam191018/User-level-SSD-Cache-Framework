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

/* return true, if success */
bool insert_mapping_before_io(mapping *mapping, char *full_path_name, unsigned page_index, unsigned *cblock);

void insert_mapping_after_io(mapping *mapping, unsigned *cblock, bool success);



#endif
