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

static int myfio_setup(struct thread_data *td) {
    printf(" --------- setup --------- \n");
    return 0;
}

// 連接ssd-cache
static int myfio_init(struct thread_data *td) {
    printf(" --------- init --------- \n");
    int rc = link_ssd_cache();
    printf("link rc = %d\n", rc);
    return rc;
}

// 關閉ssd-cache
static void myfio_cleanup(struct thread_data *td) {
    printf(" --------- cleanup --------- \n");
    info_ssd_cache();
    printf("unlink rc = %d\n", unlink_ssd_cache());
}

// 改成自己配置的malloc
static int myfio_iomem_alloc(struct thread_data *td, size_t total_mem) {
    printf(" --------- iomem_alloc --------- \n");
    td->orig_buffer = alloc_dma_buffer(PAGE_SIZE << 3);
    return td->orig_buffer == NULL;
}

static void myfio_iomem_free(struct thread_data *td) { free_dma_buffer(td->orig_buffer); }

// 開始做讀寫
static enum fio_q_status myfio_queue(struct thread_data *td, struct io_u *io_u) {
    struct pio *head = NULL;
    switch (io_u->ddir) {
        case DDIR_READ:
            head = create_pio(io_u->file->file_name, io_u->file->fd, io_u->offset >> 12, READ,
                              io_u->xfer_buf, io_u->buflen == 4096 ? 1 : 8);
            for (unsigned long long offset = PAGE_SIZE; offset < io_u->buflen;
                 offset += PAGE_SIZE) {
                append_pio(head, io_u->xfer_buf + offset);
            }
            submit_pio(head);
            break;
        case DDIR_WRITE:
            head = create_pio(io_u->file->file_name, io_u->file->fd, io_u->offset >> 12, WRITE,
                              io_u->xfer_buf, io_u->buflen == 4096 ? 1 : 8);
            for (unsigned long long offset = PAGE_SIZE; offset < io_u->buflen;
                 offset += PAGE_SIZE) {
                append_pio(head, io_u->xfer_buf + offset);
            }
            submit_pio(head);
            break;
        default:
            assert(false);
            break;
    }
    free_pio(head);
    return 0;
}

static int myfio_getevents(struct thread_data *td, unsigned int min, unsigned int max,
                           const struct timespec *t) {
    return 0;
}

static struct io_u *myfio_event(struct thread_data *td, int event) { return 0; }

/* FIO imports this structure using dlsym */
struct ioengine_ops ioengine = {.name = "ssd-cache-tester",
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
