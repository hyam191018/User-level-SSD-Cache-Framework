#ifndef CACHE_API_H
#define CACHE_API_H

/**
 *  Author: Hyam
 *  Date: 2023/03/19
 *  Description: udm-cache的操作接口，包含建立、提交與刪除等
 *              admin: init > running > exit
 *              users: link > running > free (暫時無法link到SPDK)
 */

#include "cache_type.h"
#include "config.h"
#include "pio.h"

/**
 * Description: Init share memory, wakeup workers
 * Return:  0, if success
 */
int init_udm_cache(void);

/**
 * Description: Unlink share memory, shotdown workers
 * Return:  0, if success
 */
int exit_udm_cache(void);

/**
 * Description: Map to share memory
 * Return:  0, if success
 */
// int link_udm_cache(void);

/**
 * Description: Unmap share memory
 * Return:  0, if success
 */
// int free_udm_cache(void);

/**
 * Description: Force to unlink share memory, lockless (for debug)
 * Return:  No return value
 */
void force_exit_udm_cache(void);

/**
 * Description: Print udm-cache mapping information
 * Return:  No return value
 */
void info_udm_cache(void);

/**
 * Description: Submit a page io to udm-cache-target
 * Return:  0, if success
 */
int submit_pio(struct pio* pio);

#endif