#ifndef CACHE_API_H
#define CACHE_API_H

/**
 *  @author Hyam
 *  @date 2023/03/19
 *  @brief udm-cache的操作接口，包含建立、提交與刪除等
 *              admin: init > running > exit
 *              users: link > running > free
 */

#include "cache.h"
#include "config.h"
#include "pio.h"

/**
 * @brief Init share memory, wakeup workers
 * @return 0, if success
 *         non-zero, if fail
 */
int init_ssd_cache(void);

/**
 * @brief Unlink share memory, shotdown workers
 * @return 0, if success
 *         non-zero, if fail
 */
int exit_ssd_cache(void);

/**
 * @brief Map to share memory
 * @return 0, if success
 *         non-zero, if fail
 */
int link_ssd_cache(void);

/**
 * @brief Unmap share memory
 * @return 0, if success
 *         non-zero, if fail
 */
int unlink_ssd_cache(void);

/**
 * @brief Force to unlink share memory, lockless (for debug)
 * @return No return value
 */
void force_exit_ssd_cache(void);

/**
 * @brief Print udm-cache mapping information
 * @return No return value
 */
void info_ssd_cache(void);

/**
 * @brief Submit a page io to udm-cache-target
 * @param pio - Address of page io
 * @return 0, if success
 */
int submit_pio(struct pio* pio);

#endif