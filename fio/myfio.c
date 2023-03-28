/*   SPDX-License-Identifier: BSD-3-Clause
 *   Copyright (c) Intel Corporation.
 *   All rights reserved.
 */

#include "cache_api.h"
#include "config-host.h"
#include "fio.h"
#include "optgroup.h"
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

static int myfio_setup(struct thread_data *td) {
    printf("myfio_setup\n");
    return 0;
}

static int myfio_init(struct thread_data *td) {
    printf("myfio_init\n");
    return 0;
}

static enum fio_q_status myfio_queue(struct thread_data *td, struct io_u *io_u) {
    char *buffer = malloc(PAGE_SIZE);
    struct pio *head = NULL;
    switch (io_u->ddir) {
        case DDIR_READ:
            head = create_pio(io_u->file->file_name, io_u->offset >> 12, READ, io_u->buf, 1);
            submit_pio(head);
            break;
        case DDIR_WRITE:
            head = create_pio(io_u->file->file_name, io_u->offset >> 12, WRITE, io_u->buf, 1);
            submit_pio(head);
            break;
        default:
            assert(false);
            break;
    }

    free_pio(head);
    free(buffer);
    return 0;
}

static int myfio_getevents(struct thread_data *td, unsigned int min, unsigned int max,
                           const struct timespec *t) {
    printf("myfio_getevent\n");
    return 0;
}

static struct io_u *myfio_event(struct thread_data *td, int event) {
    printf("myfio_event\n");
    return 0;
}

static int myfio_open(struct thread_data *td, struct fio_file *f) {
    printf("init rc = %d\n", init_udm_cache());
    return 0;
}

static int myfio_close(struct thread_data *td, struct fio_file *f) {
    info_udm_cache();
    printf("exit rc = %d\n", exit_udm_cache());
    return 0;
}

/* FIO imports this structure using dlsym */
struct ioengine_ops ioengine = {
    .name = "myfio",
    .version = FIO_IOOPS_VERSION,
    .setup = myfio_setup,
    .init = myfio_init,
    .queue = myfio_queue,
    .getevents = myfio_getevents,
    .event = myfio_event,
    .open_file = myfio_open,
    .close_file = myfio_close,
};

static void fio_init spdk_fio_register(void) { register_ioengine(&ioengine); }

static void fio_exit spdk_fio_unregister(void) { unregister_ioengine(&ioengine); }
