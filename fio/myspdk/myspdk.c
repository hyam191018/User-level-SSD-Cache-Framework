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

void *dma_buf;

static int myspdk_init(struct thread_data *td) {
    init_spdk();
    dma_buf = alloc_dma_buffer(4096);
    return 0;
}

static void myspdk_cleanup(struct thread_data *td) {
    free_dma_buffer(dma_buf);
    exit_spdk();
}

static enum fio_q_status myspdk_queue(struct thread_data *td, struct io_u *io_u) {
    // memcpy(dma_buf, io_u->xfer_buf, 4096);
    switch (io_u->ddir) {
        case DDIR_READ:
            read_spdk(dma_buf, 0, 8);
            break;
        case DDIR_WRITE:
            write_spdk(dma_buf, 0, 8);
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
                                .cleanup = myspdk_cleanup};

static void fio_init spdk_fio_register(void) { register_ioengine(&ioengine); }

static void fio_exit spdk_fio_unregister(void) { unregister_ioengine(&ioengine); }
