#ifndef TARGET_H
#define TARGET_H

/**
 *  @author Hyam
 *  @date 2023/03/17
 *  @brief 查找mapping，並根據結果發起IO
 */

#include "cache.h"
#include "config.h"
#include "pio.h"

/**
 * @brief Start processing the request
 * @return 0, if success
 *         non-zero, if fail
 */
int _submit_pio(struct cache* cache, struct pio* pio);

#endif
