#ifndef TARGET_H
#define TARGET_H

/*
 *	Author: Hyam
 *	Date: 2023/03/17
 *	Description: 接受pio，計算命中與否，發起IO
 */

#include "cache_type.h"
#include "config.h"
#include "pio.h"

/* in target.c */
int _submit_pio(struct cache *cache, struct pio *pio);

#endif
