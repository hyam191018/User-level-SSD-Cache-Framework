#ifndef TARGET_H
#define TARGET_H

/*
 *	Author: Hyam
 *	Date: 2023/03/17
 *	Description: 接受user的pio，查找mapping，並根據結果發起IO
 */

#include "cache_type.h"
#include "config.h"
#include "pio.h"

/*
 * Description: start processing the request
 * Return:  0, if success
 *          >0, for error cnt
 */
int _submit_pio(struct cache* cache, struct pio* pio);

#endif
