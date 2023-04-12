#include "spdk.h"

#include "atomic.h"
#include "spdk/env.h"
#include "spdk/nvme.h"

struct ctrlr_entry {
    char name[1024];
    struct spdk_nvme_ctrlr *ctrlr;
    struct spdk_nvme_ns *ns;
    struct spdk_nvme_qpair *io_read_qpair;
    struct spdk_nvme_qpair *io_write_qpair;
    struct spdk_nvme_qpair *mg_read_qpair;
    struct spdk_nvme_qpair *mg_write_qpair;
    bool isfind;
};

static struct ctrlr_entry target = {};
static struct spdk_nvme_transport_id g_trid = {};

static void trim_complete(void *cb_arg, const struct spdk_nvme_cpl *cpl) {
    if (spdk_nvme_cpl_is_error(cpl)) {
        fprintf(stderr, "Error: NVMe command failed with status code 0x%x\n", cpl->status.sc);
    }
}

static void io_complete(void *arg, const struct spdk_nvme_cpl *completion) {
    bool *is_completed = arg;
    *is_completed = true;
}

static void reset_ctrlr_and_ns(void) { memset(&target, 0, sizeof(struct ctrlr_entry)); }

static void create_qpair(queue_type type) {
    struct spdk_nvme_io_qpair_opts opts;
    spdk_nvme_ctrlr_get_default_io_qpair_opts(target.ctrlr, &opts, sizeof(opts));
    switch (type) {
        case IO_READ_QUEUE:
            target.io_read_qpair =
                spdk_nvme_ctrlr_alloc_io_qpair(target.ctrlr, &opts, sizeof(opts));
            if (!target.io_read_qpair) {
                printf("ERROR: spdk_nvme_ctrlr_alloc_io_qpair() failed\n");
            }
            break;

        case IO_WRITE_QUEUE:
            target.io_write_qpair =
                spdk_nvme_ctrlr_alloc_io_qpair(target.ctrlr, &opts, sizeof(opts));
            if (!target.io_write_qpair) {
                printf("ERROR: spdk_nvme_ctrlr_alloc_io_qpair() failed\n");
            }
            break;
        case MG_READ_QUEUE:
            target.mg_read_qpair =
                spdk_nvme_ctrlr_alloc_io_qpair(target.ctrlr, &opts, sizeof(opts));
            if (!target.mg_read_qpair) {
                printf("ERROR: spdk_nvme_ctrlr_alloc_io_qpair() failed\n");
            }
            break;

        case MG_WRITE_QUEUE:
            target.mg_write_qpair =
                spdk_nvme_ctrlr_alloc_io_qpair(target.ctrlr, &opts, sizeof(opts));
            if (!target.mg_write_qpair) {
                printf("ERROR: spdk_nvme_ctrlr_alloc_io_qpair() failed\n");
            }
            break;

        default:
            break;
    }
}

static void destroy_qpair(queue_type type) {
    switch (type) {
        case IO_READ_QUEUE:
            spdk_nvme_ctrlr_free_io_qpair(target.io_read_qpair);
            break;

        case IO_WRITE_QUEUE:
            spdk_nvme_ctrlr_free_io_qpair(target.io_write_qpair);
            break;

        case MG_READ_QUEUE:
            spdk_nvme_ctrlr_free_io_qpair(target.mg_read_qpair);
            break;

        case MG_WRITE_QUEUE:
            spdk_nvme_ctrlr_free_io_qpair(target.mg_write_qpair);
            break;

        default:
            break;
    }
}

static void register_ns(struct spdk_nvme_ctrlr *ctrlr, struct spdk_nvme_ns *ns) {
    if (!spdk_nvme_ns_is_active(ns)) {
        return;
    }
    target.ns = ns;
    target.isfind = true;
    printf("  Namespace ID: %d size: %juGB\n", spdk_nvme_ns_get_id(ns),
           spdk_nvme_ns_get_size(ns) / 1000000000);
}

// 每找到一個controll就會呼叫一次
static void attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
                      struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts) {
    int nsid;
    struct spdk_nvme_ns *ns;
    const struct spdk_nvme_ctrlr_data *cdata;

    // 是否為目標controller
    if (strcmp(trid->traddr, NVME_ADDR)) {
        return;
    }
    printf("Attached to %s\n", trid->traddr);
    cdata = spdk_nvme_ctrlr_get_data(ctrlr);
    snprintf(target.name, sizeof(target.name), "%-20.20s (%-20.20s)", cdata->mn, cdata->sn);
    target.ctrlr = ctrlr;

    // 只使用第一個 namespace
    for (nsid = spdk_nvme_ctrlr_get_first_active_ns(ctrlr); nsid != 0;
         nsid = spdk_nvme_ctrlr_get_next_active_ns(ctrlr, nsid)) {
        ns = spdk_nvme_ctrlr_get_ns(ctrlr, nsid);
        if (ns == NULL) {
            continue;
        }
        register_ns(ctrlr, ns);
        break;
    }
}

static void parse_args(struct spdk_env_opts *env_opts) {
    spdk_nvme_trid_populate_transport(&g_trid, SPDK_NVME_TRANSPORT_PCIE);
    snprintf(g_trid.subnqn, sizeof(g_trid.subnqn), "%s", SPDK_NVMF_DISCOVERY_NQN);
}

/* --------------------------------------------------- */

void *alloc_dma_buffer(unsigned len) {
    return spdk_zmalloc(len, 0x1000, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
}

void free_dma_buffer(void *dma_buf) { spdk_free(dma_buf); }

int init_spdk(void) {
    struct spdk_env_opts opts;
    spdk_env_opts_init(&opts);
    parse_args(&opts);
    opts.name = "udm-cache-spdk";
    if (spdk_env_init(&opts) < 0) {
        fprintf(stderr, "Unable to initialize SPDK env\n");
        return 1;
    }
    reset_ctrlr_and_ns();

    // 尋找目標nvme controller和namespace，並分別儲存在target, target
    if (spdk_nvme_probe(&g_trid, NULL, NULL, attach_cb, NULL)) {
        fprintf(stderr, "spdk_nvme_probe() failed\n");
        return 1;
    }

    if (!target.isfind) {
        fprintf(stderr, "No namespace found\n");
        return 1;
    }

    // 建立qpair用於IO
    create_qpair(IO_READ_QUEUE);
    create_qpair(IO_WRITE_QUEUE);
    create_qpair(MG_READ_QUEUE);
    create_qpair(MG_WRITE_QUEUE);
    return 0;
}

void exit_spdk(void) {
    if (!target.isfind) {
        printf("Error: No namespace found");
        return;
    }
    destroy_qpair(IO_READ_QUEUE);
    destroy_qpair(IO_WRITE_QUEUE);
    destroy_qpair(MG_READ_QUEUE);
    destroy_qpair(MG_WRITE_QUEUE);
    spdk_nvme_detach(target.ctrlr);
    spdk_env_fini();
    reset_ctrlr_and_ns();
}

void get_device_info(unsigned *block_size, unsigned long *device_size) {
    if (!target.isfind) {
        printf("Error: No namespace found");
        return;
    }
    *block_size = spdk_nvme_ns_get_sector_size(target.ns);
    *device_size = spdk_nvme_ns_get_size(target.ns);
}

int read_spdk(void *dma_buf, unsigned long offset_block, unsigned num_block, queue_type type) {
    int rc = 0;
    bool is_completed = false;
    rc = spdk_nvme_ns_cmd_read(target.ns,
                               type == IO_READ_QUEUE ? target.io_read_qpair : target.mg_read_qpair,
                               dma_buf, offset_block, num_block, io_complete, &is_completed, 0);
    if (rc) {
        fprintf(stderr, "starting read I/O failed\n");
        return rc;
    }
    while (!is_completed) {
        spdk_nvme_qpair_process_completions(
            type == IO_READ_QUEUE ? target.io_read_qpair : target.mg_read_qpair, 0);
    }

    return 0;
}
int write_spdk(void *dma_buf, unsigned long offset_block, unsigned num_block, queue_type type) {
    int rc = 0;
    bool is_completed = false;
    rc = spdk_nvme_ns_cmd_write(
        target.ns, type == IO_WRITE_QUEUE ? target.io_write_qpair : target.mg_write_qpair, dma_buf,
        offset_block, num_block, io_complete, &is_completed, 0);
    if (rc) {
        fprintf(stderr, "starting write I/O failed\n");
        return rc;
    }
    while (!is_completed) {
        spdk_nvme_qpair_process_completions(
            type == IO_WRITE_QUEUE ? target.io_write_qpair : target.mg_write_qpair, 0);
    }

    return 0;
}

int trim_spdk(unsigned long offset_block, unsigned num_block, queue_type type) {
    int rc = 0;
    struct spdk_nvme_dsm_range range;
    range.starting_lba = offset_block;
    range.length = num_block;
    // 走write qpair
    rc = spdk_nvme_ns_cmd_dataset_management(
        target.ns, type == IO_WRITE_QUEUE ? target.io_write_qpair : target.mg_write_qpair,
        SPDK_NVME_DSM_ATTR_DEALLOCATE, &range, 1, trim_complete, NULL);
    if (rc == -ENOMEM) {
        printf("Error: The request cannot be allocated\n");
    } else if (rc == -ENXIO) {
        printf("Error: The qpair is failed at the transport level\n");
    }
    return rc;
}