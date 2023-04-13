/*   SPDX-License-Identifier: BSD-3-Clause
 *   Copyright (c) Intel Corporation.
 *   All rights reserved.
 */

#include "config-host.h"
#include "fio.h"
#include "optgroup.h"
#include "spdk.h"
#include "spdk/accel.h"
#include "spdk/bdev.h"
#include "spdk/bdev_zone.h"
#include "spdk/env.h"
#include "spdk/init.h"
#include "spdk/log.h"
#include "spdk/queue.h"
#include "spdk/stdinc.h"
#include "spdk/string.h"
#include "spdk/thread.h"
#include "spdk/util.h"
#include "spdk_internal/event.h"

static int myspdk_init(struct thread_data *td) {
    init_spdk();
    return 0;
}

static void myspdk_cleanup(struct thread_data *td) { exit_spdk(); }

static int myspdk_iomem_alloc(struct thread_data *td, size_t total_mem) {
    td->orig_buffer = alloc_dma_buffer(PAGE_SIZE);
    return td->orig_buffer == NULL;
}

static void myspdk_iomem_free(struct thread_data *td) { spdk_dma_free(td->orig_buffer); }

static enum fio_q_status myspdk_queue(struct thread_data *td, struct io_u *io_u) {
    switch (io_u->ddir) {
        case DDIR_READ:
            read_spdk(io_u->xfer_buf, io_u->offset >> 9, 8, IO_QUEUE);
            break;
        case DDIR_WRITE:
            write_spdk(io_u->xfer_buf, io_u->offset >> 9, 8, IO_QUEUE);
            break;
        default:
            assert(false);
            break;
    }
    return FIO_Q_COMPLETED;
}

static int myspdk_setup(struct thread_data *td) { return 0; }

static int myspdk_getevents(struct thread_data *td, unsigned int min, unsigned int max,
                            const struct timespec *t) {
    return 0;
}

static struct io_u *myspdk_event(struct thread_data *td, int event) { return 0; }

/* FIO imports this structure using dlsym */
struct ioengine_ops ioengine = {.name = "myspdk",
                                .version = FIO_IOOPS_VERSION,
                                .setup = myspdk_setup,
                                .init = myspdk_init,
                                .queue = myspdk_queue,
                                .getevents = myspdk_getevents,
                                .event = myspdk_event,
                                .open_file = generic_open_file,
                                .close_file = generic_close_file,
                                .iomem_alloc = myspdk_iomem_alloc,
                                .iomem_free = myspdk_iomem_free,
                                .cleanup = myspdk_cleanup};

static void fio_init spdk_fio_register(void) { register_ioengine(&ioengine); }

static void fio_exit spdk_fio_unregister(void) { unregister_ioengine(&ioengine); }
