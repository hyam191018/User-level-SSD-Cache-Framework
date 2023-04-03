#include "spdk.h"

#include "spdk/env.h"
#include "spdk/nvme.h"

struct ctrlr_entry {
    bool isfind;
    struct spdk_nvme_ctrlr *ctrlr;
    char name[1024];
};

struct ns_entry {
    bool isfind;
    struct spdk_nvme_ctrlr *ctrlr;
    struct spdk_nvme_ns *ns;
    struct spdk_nvme_qpair *qpair;
};

static struct ctrlr_entry tgt_ctrlr = {};
static struct ns_entry tgt_ns = {};
static struct spdk_nvme_transport_id g_trid = {};

static void trim_complete(void *cb_arg, const struct spdk_nvme_cpl *cpl) {
    if (spdk_nvme_cpl_is_error(cpl)) {
        fprintf(stderr, "Error: NVMe command failed with status code 0x%x\n", cpl->status.sc);
    }
}

static void read_complete(void *arg, const struct spdk_nvme_cpl *completion) {
    bool *is_completed = arg;
    *is_completed = true;
    if (spdk_nvme_cpl_is_error(completion)) {
        spdk_nvme_qpair_print_completion(tgt_ns.qpair, (struct spdk_nvme_cpl *)completion);
        fprintf(stderr, "I/O error status: %s\n",
                spdk_nvme_cpl_get_status_string(&completion->status));
        fprintf(stderr, "Write I/O failed, aborting run\n");
    }
}

static void write_complete(void *arg, const struct spdk_nvme_cpl *completion) {
    bool *is_completed = arg;
    *is_completed = true;
    if (spdk_nvme_cpl_is_error(completion)) {
        spdk_nvme_qpair_print_completion(tgt_ns.qpair, (struct spdk_nvme_cpl *)completion);
        fprintf(stderr, "I/O error status: %s\n",
                spdk_nvme_cpl_get_status_string(&completion->status));
        fprintf(stderr, "Write I/O failed, aborting run\n");
    }
}

static void reset_ctrlr_and_ns(void) {
    memset(&tgt_ctrlr, 0, sizeof(struct ctrlr_entry));
    memset(&tgt_ns, 0, sizeof(struct ns_entry));
}

static void create_qpair(void) {
    tgt_ns.qpair = spdk_nvme_ctrlr_alloc_io_qpair(tgt_ns.ctrlr, NULL, 0);
    if (!tgt_ns.qpair) {
        printf("ERROR: spdk_nvme_ctrlr_alloc_io_qpair() failed\n");
    }
}

static void destroy_qpair(void) { spdk_nvme_ctrlr_free_io_qpair(tgt_ns.qpair); }

static void register_ns(struct spdk_nvme_ctrlr *ctrlr, struct spdk_nvme_ns *ns) {
    if (!spdk_nvme_ns_is_active(ns)) {
        return;
    }
    tgt_ns.ctrlr = ctrlr;
    tgt_ns.ns = ns;
    tgt_ns.isfind = true;
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
    snprintf(tgt_ctrlr.name, sizeof(tgt_ctrlr.name), "%-20.20s (%-20.20s)", cdata->mn, cdata->sn);
    tgt_ctrlr.ctrlr = ctrlr;
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

    // 尋找目標nvme controller和namespace，並分別儲存在tgt_ctrlr, tgt_ns
    if (spdk_nvme_probe(&g_trid, NULL, NULL, attach_cb, NULL)) {
        fprintf(stderr, "spdk_nvme_probe() failed\n");
        return 1;
    }

    if (!tgt_ns.isfind) {
        fprintf(stderr, "No namespace found\n");
        return 1;
    }

    // 建立qpair用於IO
    create_qpair();
    return 0;
}

void exit_spdk(void) {
    if (!tgt_ns.isfind) {
        printf("Error: No namespace found");
        return;
    }
    destroy_qpair();
    spdk_nvme_detach(tgt_ctrlr.ctrlr);
    spdk_env_fini();
    reset_ctrlr_and_ns();
}

void get_device_info(unsigned *block_size, unsigned long *device_size) {
    if (!tgt_ns.isfind) {
        printf("Error: No namespace found");
        return;
    }
    *block_size = spdk_nvme_ns_get_sector_size(tgt_ns.ns);
    *device_size = spdk_nvme_ns_get_size(tgt_ns.ns);
}

void *alloc_dma_buffer(unsigned len) {
    void *dma_buf = spdk_zmalloc(len, 0x1000, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
    return dma_buf;
}

void free_dma_buffer(void *dma_buf) { spdk_free(dma_buf); }

int read_spdk(void *dma_buf, unsigned long offset_block, unsigned num_block) {
    int rc = 0;
    bool is_completed = false;
    rc = spdk_nvme_ns_cmd_read(tgt_ns.ns, tgt_ns.qpair, dma_buf, offset_block, num_block,
                               read_complete, &is_completed, 0);
    if (rc) {
        fprintf(stderr, "starting read I/O failed\n");
        return rc;
    }
    while (!is_completed) {
        spdk_nvme_qpair_process_completions(tgt_ns.qpair, 0);
    }
    return 0;
}

int write_spdk(void *dma_buf, unsigned long offset_block, unsigned num_block) {
    int rc = 0;
    bool is_completed = false;
    rc = spdk_nvme_ns_cmd_write(tgt_ns.ns, tgt_ns.qpair, dma_buf, offset_block, num_block,
                                write_complete, &is_completed, 0);
    if (rc) {
        fprintf(stderr, "starting write I/O failed\n");
        return rc;
    }
    while (!is_completed) {
        spdk_nvme_qpair_process_completions(tgt_ns.qpair, 0);
    }
    return 0;
}

int trim_spdk(unsigned long offset_block, unsigned num_block) {
    int rc = 0;
    struct spdk_nvme_dsm_range range;
    range.starting_lba = offset_block;
    range.length = num_block;
    rc = spdk_nvme_ns_cmd_dataset_management(tgt_ns.ns, tgt_ns.qpair, SPDK_NVME_DSM_ATTR_DEALLOCATE,
                                             &range, 1, trim_complete, NULL);
    if (rc == -ENOMEM) {
        printf("Error: The request cannot be allocated\n");
    } else if (rc == -ENXIO) {
        printf("Error: The qpair is failed at the transport level\n");
    }
    return rc;
}