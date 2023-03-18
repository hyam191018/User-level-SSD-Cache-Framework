#include "mapping.h"
#include "shm.h"
#include "stdinc.h"
#include "config.h"

/* --------------------------------------------------- */

#define INDEXER_NULL ((1u << 28u) - 1u)

static int alloc_es(struct entry_space *es, unsigned nr_entries){
    es->begin = alloc_shm(SHM_ENTRY_SPACE, sizeof(struct entry) * nr_entries);
    if(!es->begin){
        perror("alloc_shm");
        return 1;
    }
	es->end = es->begin + nr_entries;
	return 0;
}

static int unmap_es(struct entry_space *es, unsigned nr_entries){
    if( !es->begin || !nr_entries ) return 1;
    return unmap_shm(es->begin, sizeof(struct entry) * nr_entries);
}

static int unlink_es(void){
	return unlink_shm(SHM_ENTRY_SPACE);
}

static struct entry *__get_entry(struct entry_space *es, unsigned block){
	struct entry *e;
	e = es->begin + block;
	return e;
}

static unsigned to_index(struct entry_space *es, struct entry *e){
	return e - es->begin;
}

static struct entry *to_entry(struct entry_space *es, unsigned block){
	if (block == INDEXER_NULL) return NULL;
	return __get_entry(es, block);
}

/* --------------------------------------------------- */

static void l_init(struct ilist *l){
	l->nr_elts = 0;
	l->head = l->tail = INDEXER_NULL;
}

static struct entry *l_head(struct entry_space *es, struct ilist *l){
	return to_entry(es, l->head);
}

static struct entry *l_tail(struct entry_space *es, struct ilist *l){
	return to_entry(es, l->tail);
}

static struct entry *l_next(struct entry_space *es, struct entry *e){
	return to_entry(es, e->next);
}

static struct entry *l_prev(struct entry_space *es, struct entry *e){
	return to_entry(es, e->prev);
}

static int l_empty(struct ilist *l){
	return l->head == INDEXER_NULL;
}

static void l_add_head(struct entry_space *es, struct ilist *l, struct entry *e){

	struct entry *head = l_head(es, l);

	e->next = l->head;
	e->prev = INDEXER_NULL;

	if (head)
		head->prev = l->head = to_index(es, e);
	else
		l->head = l->tail = to_index(es, e);

	l->nr_elts++;
}

static void l_add_tail(struct entry_space *es, struct ilist *l, struct entry *e){

	struct entry *tail = l_tail(es, l);

	e->next = INDEXER_NULL;
	e->prev = l->tail;

	if (tail)
		tail->next = l->tail = to_index(es, e);
	else
		l->head = l->tail = to_index(es, e);

	l->nr_elts++;
}

static void l_del(struct entry_space *es, struct ilist *l, struct entry *e){

	struct entry *prev = l_prev(es, e);
	struct entry *next = l_next(es, e);

	if (prev)
		prev->next = e->next;
	else
		l->head = e->next;

	if (next)
		next->prev = e->prev;
	else
		l->tail = e->prev;
	
	l->nr_elts--;
}

static struct entry *l_pop_head(struct entry_space *es, struct ilist *l){

	struct entry *e;

	for (e = l_head(es, l); e; e = l_next(es, e)){
		l_del(es, l, e);
		return e;
	}

	return NULL;
}

/* --------------------------------------------------- */

static void init_allocator(struct entry_alloc *ea, struct entry_space *es, unsigned begin, unsigned end){

	unsigned i;

	ea->es = es;
	ea->nr_allocated = 0;
	ea->begin = begin;

	l_init(&ea->free);
	for (i = begin; i != end; i++){
		l_add_tail(ea->es, &ea->free, __get_entry(ea->es, i));
	}
}

static void link_allocator(struct entry_alloc *ea, struct entry_space *es){
	ea->es = es;
}

static void init_entry(struct entry *e){
	e->hash_next = INDEXER_NULL;
	e->prev = INDEXER_NULL;
	e->next = INDEXER_NULL;
	e->param = 0;
	memset(e->full_path_name, 0, MAX_PATH_SIZE);
	e->cache_page = INDEXER_NULL;
}

static void entry_set_pending(struct entry *e, bool pending){
	if(pending){
		e->param |= 1;
	}else{
		e->param &= 0;
	}
}

static bool entry_get_pending(struct entry *e){
	unsigned n = e->param;
	return (n << 15) >> 15;
}

static void entry_set_dirty(struct entry *e, bool dirty){
	if(dirty){
		e->param |= 2;
	}else{
		e->param &= ~2;
	}
}

static bool entry_get_dirty(struct entry *e){
	unsigned n = e->param;
	return (n << 14) >> 15;
}

static struct entry *alloc_entry(struct entry_alloc *ea){
	struct entry *e;

	if (l_empty(&ea->free))
		return NULL;

	e = l_pop_head(ea->es, &ea->free);
	init_entry(e);
	ea->nr_allocated++;

	return e;
}

static void free_entry(struct entry_alloc *ea, struct entry *e){
	ea->nr_allocated--;
	l_add_tail(ea->es, &ea->free, e);
}

static int allocator_empty(struct entry_alloc *ea){
	return l_empty(&ea->free);
}


static unsigned get_index(struct entry_alloc *ea, struct entry *e){
	return to_index(ea->es, e) - ea->begin;
}

static struct entry *get_entry(struct entry_alloc *ea, unsigned index){
	return __get_entry(ea->es, ea->begin + index);
}

static unsigned infer_cblock(mapping *mapping, struct entry *e){
	return get_index(&mapping->ca, e);
}

/* --------------------------------------------------- */

/* 演算法是用chatGPT產生的: djb2 hash */
static unsigned hash_32(char* full_path_name, unsigned cache_page, unsigned long long hash_bits){
	unsigned hash_val = 5381;
    int c;

    // hash the integer first
    hash_val = ((hash_val << 5) + hash_val) + cache_page;

    // hash the string next
    while ((c = *full_path_name++)) {
        hash_val = ((hash_val << 5) + hash_val) + c; /* hash * 33 + c */
    }

	hash_val = ( hash_val <<  (32 - hash_bits) ) >> (32 - hash_bits);
	return hash_val;
}

static unsigned roundup_pow_of_two(unsigned n){
    unsigned res = 1;
    while (res < n) {
        res <<= 1;
    }
    return res;
}

static int alloc_hash_table(struct hash_table *ht, struct entry_space *es, unsigned nr_entries){

	unsigned i, nr_buckets;

	ht->es = es;
	nr_buckets = roundup_pow_of_two(nr_entries);
	ht->hash_bits = 31 - __builtin_clz(nr_buckets);
    ht->buckets = alloc_shm(SHM_BUCKETS, nr_buckets * sizeof(*ht->buckets));
    if(!ht->buckets){
        perror("alloc_shm");
        return 1;
    }

	for (i = 0; i < nr_buckets; i++)
		ht->buckets[i] = INDEXER_NULL;

    return 0;
}

static int link_hash_table(struct hash_table *ht, struct entry_space *es, unsigned nr_entries){

	unsigned nr_buckets;

	ht->es = es;
	nr_buckets = roundup_pow_of_two(nr_entries);
	ht->hash_bits = 31 - __builtin_clz(nr_buckets);
    ht->buckets = alloc_shm(SHM_BUCKETS, nr_buckets * sizeof(*ht->buckets));
    if(!ht->buckets){
        perror("alloc_shm");
        return 1;
    }
    
    return 0;
}

static int unmap_hash_table(struct hash_table *ht, unsigned nr_entries){
    unsigned nr_buckets = roundup_pow_of_two(nr_entries);
    if( !ht->buckets || !nr_entries ) return 1;
	return unmap_shm(ht->buckets, nr_buckets * sizeof(*ht->buckets));
}

static int unlink_hash_table(void){
	return unlink_shm(SHM_BUCKETS);
}

static struct entry *h_head(struct hash_table *ht, unsigned bucket){
	return to_entry(ht->es, ht->buckets[bucket]);
}

static struct entry *h_next(struct hash_table *ht, struct entry *e){
	return to_entry(ht->es, e->hash_next);
}

static void __h_insert(struct hash_table *ht, unsigned bucket, struct entry *e){
	e->hash_next = ht->buckets[bucket];
	ht->buckets[bucket] = to_index(ht->es, e);
}

static void h_insert(struct hash_table *ht, struct entry *e){
	unsigned h = hash_32(e->full_path_name, e->cache_page, ht->hash_bits);
	__h_insert(ht, h, e);
}

static struct entry *__h_lookup(struct hash_table *ht, unsigned h, char* full_path_name, unsigned cache_page, struct entry **prev){

	struct entry *e;
	*prev = NULL;
	for (e = h_head(ht, h); e; e = h_next(ht, e)) {
		if ( (cache_page == e->cache_page) && strcmp(full_path_name,e->full_path_name) == 0)
			return e;

		*prev = e;
	}
	return NULL;
}

static void __h_unlink(struct hash_table *ht, unsigned h, struct entry *e, struct entry *prev){
	if (prev)
		prev->hash_next = e->hash_next;
	else
		ht->buckets[h] = e->hash_next;
}

static struct entry *h_lookup(struct hash_table *ht, char* full_path_name, unsigned cache_page){

	struct entry *e, *prev;
	unsigned h = hash_32(full_path_name, cache_page, ht->hash_bits);

	e = __h_lookup(ht, h, full_path_name, cache_page, &prev);
	if (e && prev) {
		/*
		 * Move to the front because this entry is likely
		 * to be hit again.
		 */
		__h_unlink(ht, h, e, prev);
		__h_insert(ht, h, e);
	}

	return e;
}

static void h_remove(struct hash_table *ht, struct entry *e){

	unsigned h = hash_32(e->full_path_name, e->cache_page, ht->hash_bits);
	struct entry *prev;

	/*
	 * The down side of using a singly linked list is we have to
	 * iterate the bucket to remove an item.
	 */
	e = __h_lookup(ht, h, e->full_path_name, e->cache_page, &prev);
	if (e)
		__h_unlink(ht, h, e, prev);
}


/* --------------------------------------------------- */

int init_mapping(mapping* mapping, unsigned block_size, unsigned cblock_num){
	spinlock_lock(&mapping->mapping_lock);
	int rc = 0;
    mapping->block_size = block_size;
    mapping->cblock_num = cblock_num;

    rc += alloc_es(&mapping->es, mapping->cblock_num);
    if(rc){
		goto err;
    }
    
	init_allocator(&mapping->ca, &mapping->es, 0, mapping->cblock_num);
    l_init(&mapping->clean);
	l_init(&mapping->dirty);

    rc += alloc_hash_table(&mapping->table, &mapping->es, mapping->cblock_num);
    if(rc){
		goto err;
    }

    mapping->hit_time = 0;
    mapping->miss_time = 0;

err:
	spinlock_unlock(&mapping->mapping_lock);
    return rc;
}


int link_mapping(mapping* mapping){
	spinlock_lock(&mapping->mapping_lock);
    int rc = 0;

	link_allocator(&mapping->ca, &mapping->es);

    rc += alloc_es(&mapping->es, mapping->cblock_num);
	if(rc){
		goto err;
    }

	rc += link_hash_table(&mapping->table, &mapping->es, mapping->cblock_num);

err:
	spinlock_unlock(&mapping->mapping_lock);
    return rc;
}

int free_mapping(mapping* mapping){
	spinlock_lock(&mapping->mapping_lock);
    int rc = 0;
    rc += unmap_es(&mapping->es, mapping->cblock_num);
	if(rc){
		goto err;
    }

    rc += unmap_hash_table(&mapping->table, mapping->cblock_num);

err:
	spinlock_unlock(&mapping->mapping_lock);
    return rc;
}

int exit_mapping(void){
    int rc = 0;
    rc += unlink_es();
    rc += unlink_hash_table();
    return rc;
}

void info_mapping(mapping* mapping){
	spinlock_lock(&mapping->mapping_lock);
    printf("---> Information of mapping table <---\n");
    printf("/ free  entrys = %u\n", mapping->ca.free.nr_elts);
	printf("/ clean entrys = %u\n", mapping->clean.nr_elts);
	printf("/ dirty entrys = %u\n", mapping->dirty.nr_elts);
	unsigned hit_ratio = safe_div((mapping->hit_time * 100), (mapping->hit_time + mapping->miss_time));
    printf("/ hit time = %u, miss time = %u, hit ratio = %u%%\n",mapping->hit_time, mapping->miss_time, hit_ratio);
	spinlock_unlock(&mapping->mapping_lock);
}

#define to_cache_page(page_index) (page_index >> 3)

bool lookup_mapping(mapping *mapping, char *full_path_name, unsigned page_index, unsigned *cblock){
    spinlock_lock(&mapping->mapping_lock);
    struct entry* e = h_lookup(&mapping->table, full_path_name, to_cache_page(page_index));
	e ? mapping->hit_time++ : mapping->miss_time++;
    *cblock = (e ? infer_cblock(mapping, e) : 0);
    spinlock_unlock(&mapping->mapping_lock);
    return (e != NULL);
}

bool insert_mapping_before_io(mapping *mapping, char *full_path_name, unsigned page_index, unsigned *cblock){
    struct entry* e = h_lookup(&mapping->table, full_path_name, to_cache_page(page_index));
    if (e) {
        return false;
    }

    spinlock_lock(&mapping->mapping_lock);
    int res = true;
    e = alloc_entry(&mapping->ca);
    if (!e) {
        res = false;
        goto err;
    }
    init_entry(e);
    strcpy(e->full_path_name, full_path_name);
    e->cache_page = to_cache_page(page_index);
    entry_set_pending(e, true);
    *cblock = infer_cblock(mapping, e);

err:
    spinlock_unlock(&mapping->mapping_lock);
    return res;
}

void insert_mapping_after_io(mapping *mapping, unsigned *cblock, bool success){
    spinlock_lock(&mapping->mapping_lock);
    struct entry *e = get_entry(&mapping->ca, *cblock);
    entry_set_pending(e, false);
    if (success) {
        h_insert(&mapping->table, e);
        entry_set_dirty(e, true);
        l_add_tail(&mapping->es, &mapping->dirty, e);
    } else {
        free_entry(&mapping->ca, e);
    }
    spinlock_unlock(&mapping->mapping_lock);
}