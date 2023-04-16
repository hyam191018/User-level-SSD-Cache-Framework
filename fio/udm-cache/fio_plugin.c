/*   SPDX-License-Identifier: BSD-3-Clause
 *   Copyright (c) Intel Corporation.
 *   All rights reserved.
 */

#include "cache_api.h"
#include "config-host.h"
#include "fio.h"
#include "optgroup.h"
#include "spdk.h"
#include "spdk_internal/event.h"

// 開啟udm-cache
static int myfio_init(struct thread_data *td) {
    printf("exit rc = %d\n", exit_udm_cache());
    printf("init rc = %d\n", init_udm_cache());
    return 0;
}

// 關閉udm-cache
static void myfio_cleanup(struct thread_data *td) { info_udm_cache(); }

// 改成自己配置的malloc
static int myfio_iomem_alloc(struct thread_data *td, size_t total_mem) {
    td->orig_buffer = alloc_dma_buffer(PAGE_SIZE);
    return td->orig_buffer == NULL;
}

static void myfio_iomem_free(struct thread_data *td) { free_dma_buffer(td->orig_buffer); }

// 開始做讀寫
static enum fio_q_status myfio_queue(struct thread_data *td, struct io_u *io_u) {
    struct pio *head = NULL;
    switch (io_u->ddir) {
        case DDIR_READ:
            head = create_pio(io_u->file->file_name, io_u->file->fd, io_u->offset >> 12, READ,
                              io_u->xfer_buf, 1);
            submit_pio(head);
            break;
        case DDIR_WRITE:
            head = create_pio(io_u->file->file_name, io_u->file->fd, io_u->offset >> 12, WRITE,
                              io_u->xfer_buf, 1);
            submit_pio(head);
            break;
        default:
            assert(false);
            break;
    }
    free_pio(head);
    return 0;
}

static int myfio_setup(struct thread_data *td) {
    force_exit_udm_cache();
    return 0;
}

static int myfio_getevents(struct thread_data *td, unsigned int min, unsigned int max,
                           const struct timespec *t) {
    return 0;
}

static struct io_u *myfio_event(struct thread_data *td, int event) { return 0; }

/* FIO imports this structure using dlsym */
struct ioengine_ops ioengine = {.name = "myfio",
                                .version = FIO_IOOPS_VERSION,
                                .setup = myfio_setup,
                                .init = myfio_init,
                                .queue = myfio_queue,
                                .getevents = myfio_getevents,
                                .event = myfio_event,
                                .open_file = generic_open_file,
                                .close_file = generic_close_file,
                                .iomem_alloc = myfio_iomem_alloc,
                                .iomem_free = myfio_iomem_free,
                                .cleanup = myfio_cleanup};

static void fio_init spdk_fio_register(void) { register_ioengine(&ioengine); }

static void fio_exit spdk_fio_unregister(void) { unregister_ioengine(&ioengine); }
