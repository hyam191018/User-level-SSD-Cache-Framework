#ifndef SHM_H
#define SHM_H

/**
 *  @author Hyam
 *  @date 2023/03/17
 *  @brief 建立、連結、釋放、刪除共享記憶體位置
 */

#include "stdinc.h"

/**
 * @brief Open and mmap to share memory
 * @return Share memory address, if success
 *         NULL, if fail
 */
void* alloc_shm(char* shm_name, size_t shm_size);

/**
 * @brief Map to share memory
 * @return Share memory address, if success
 *         NULL, if fail
 */
void* link_shm(char* shm_name, size_t shm_size);

/**
 * @brief Unmap share memory
 * @return 0, if success
 *         non_zero, if fail
 */
int unmap_shm(void* shm_ptr, size_t shm_size);

/**
 * @brief Unlink share memory
 * @return 0, if success
 *         non-zero, if fail
 */
int unlink_shm(char* shm_name);

#endif