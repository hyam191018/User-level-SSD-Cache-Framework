#include "spdk.h"

#include "spdk/env.h"
#include "spdk/nvme.h"

struct ctrlr_entry {
    char name[1024];
    struct spdk_nvme_ctrlr *ctrlr;
    struct spdk_nvme_ns *ns;
    struct spdk_nvme_qpair **qpair;
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
    target.qpair[type] = spdk_nvme_ctrlr_alloc_io_qpair(target.ctrlr, &opts, sizeof(opts));
    if (!target.qpair[type]) {
        printf("ERROR: spdk_nvme_ctrlr_alloc_io_qpair() failed\n");
    }
}

static void destroy_qpair(queue_type type) { spdk_nvme_ctrlr_free_io_qpair(target.qpair[type]); }

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
    opts.shm_id = 0;

    if (spdk_env_init(&opts) < 0) {
        fprintf(stderr, "Unable to initialize SPDK env\n");
        return 1;
    }
    reset_ctrlr_and_ns();

    // 尋找目標nvme controller和namespace，並分別儲存在target
    if (spdk_nvme_probe(&g_trid, NULL, NULL, attach_cb, NULL)) {
        fprintf(stderr, "spdk_nvme_probe() failed\n");
        return 1;
    }

    if (!target.isfind) {
        fprintf(stderr, "No namespace found\n");
        return 1;
    }

    // 建立qpair
    target.qpair = calloc(QPAIR_COUNT, sizeof(struct spdk_nvme_qpair *));
    for (unsigned type = 0; type < QPAIR_COUNT; type++) {
        create_qpair(type);
    }

    return 0;
}

void exit_spdk(void) {
    if (!target.isfind) {
        printf("Error: No namespace found");
        return;
    }
    for (unsigned type = 0; type < QPAIR_COUNT; type++) {
        destroy_qpair(type);
    }
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
    rc = spdk_nvme_ns_cmd_read(target.ns, target.qpair[type], dma_buf, offset_block, num_block,
                               io_complete, &is_completed, 0);

    if (rc) {
        fprintf(stderr, "starting read I/O failed\n");
        return rc;
    }

    while (!is_completed) {
        spdk_nvme_qpair_process_completions(target.qpair[type], 0);
    }

    return 0;
}

int write_spdk(void *dma_buf, unsigned long offset_block, unsigned num_block, queue_type type) {
    int rc = 0;
    bool is_completed = false;
    rc = spdk_nvme_ns_cmd_write(target.ns, target.qpair[type], dma_buf, offset_block, num_block,
                                io_complete, &is_completed, 0);

    if (rc) {
        fprintf(stderr, "starting write I/O failed\n");
        return rc;
    }

    while (!is_completed) {
        spdk_nvme_qpair_process_completions(target.qpair[type], 0);
    }

    return 0;
}

int trim_spdk(unsigned long offset_block, unsigned num_block, queue_type type) {
    int rc = 0;
    struct spdk_nvme_dsm_range range;
    range.starting_lba = offset_block;
    range.length = num_block;
    rc = spdk_nvme_ns_cmd_dataset_management(target.ns, target.qpair[type],
                                             SPDK_NVME_DSM_ATTR_DEALLOCATE, &range, 1,
                                             trim_complete, NULL);
    if (rc == -ENOMEM) {
        printf("Error: The request cannot be allocated\n");
    } else if (rc == -ENXIO) {
        printf("Error: The qpair is failed at the transport level\n");
    }
    return rc;
}

/* --------------------------------------------------- */

struct io_req {
    bool is_completed;
    unsigned iov_offset;
    struct iovec *iovs;
    int iov_pos;  // 目標iov
    int iov_cnt;  // iov總數
};

static void init_req(struct io_req *req, struct iovec *iov, int iovcnt) {
    req->is_completed = false;
    req->iovs = iov;
    req->iov_pos = 0;
    req->iov_cnt = iovcnt;
}

static void cb_fn(void *ctx, const struct spdk_nvme_cpl *cpl) {
    struct io_req *req = (struct io_req *)ctx;
    req->is_completed = true;
}

static void reset_sgl_fn(void *ref, uint32_t sgl_offset) {
    struct io_req *req = (struct io_req *)ref;
    struct iovec *iov;
    req->iov_offset = sgl_offset;
    for (req->iov_pos = 0; req->iov_pos < req->iov_cnt; req->iov_pos++) {
        iov = &req->iovs[req->iov_pos];
        if (req->iov_offset < iov->iov_len) {
            break;
        }
        req->iov_offset -= iov->iov_len;
    }
}
static int next_sge_fn(void *ref, void **address, uint32_t *length) {
    struct io_req *req = (struct io_req *)ref;
    struct iovec *iov;

    iov = &req->iovs[req->iov_pos];

    *address = iov->iov_base;
    *length = iov->iov_len;

    if (req->iov_offset) {
        *address += req->iov_offset;
        *length -= req->iov_offset;
    }

    req->iov_offset += *length;
    if (req->iov_offset == iov->iov_len) {
        req->iov_pos++;
        req->iov_offset = 0;
    }

    return 0;
}

int readv_spdk(struct iovec *iov, int iov_cnt, unsigned long offset_block, unsigned num_block,
               queue_type type) {
    int rc = 0;
    struct io_req req;
    init_req(&req, iov, iov_cnt);
    rc = spdk_nvme_ns_cmd_readv(target.ns, target.qpair[type], offset_block, num_block, cb_fn, &req,
                                0, reset_sgl_fn, next_sge_fn);

    if (rc) {
        fprintf(stderr, "starting write I/O failed\n");
        return rc;
    }

    while (!req.is_completed) {
        spdk_nvme_qpair_process_completions(target.qpair[type], 0);
    }
    return 0;
}

int writev_spdk(struct iovec *iov, int iov_cnt, unsigned long offset_block, unsigned num_block,
                queue_type type) {
    int rc = 0;
    struct io_req req;
    init_req(&req, iov, iov_cnt);
    rc = spdk_nvme_ns_cmd_writev(target.ns, target.qpair[type], offset_block, num_block, cb_fn,
                                 &req, 0, reset_sgl_fn, next_sge_fn);

    if (rc) {
        fprintf(stderr, "starting write I/O failed\n");
        return rc;
    }

    while (!req.is_completed) {
        spdk_nvme_qpair_process_completions(target.qpair[type], 0);
    }
    return 0;
}