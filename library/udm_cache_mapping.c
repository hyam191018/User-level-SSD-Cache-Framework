#include "mapping.h"
#include "shm.h"
#include "stdinc.h"
#include "config.h"
#include "work_queue.h"

/* --------------------------------------------------- */

#define INDEXER_NULL ((1u << 28u) - 1u)

static int alloc_es(struct entry_space *es, unsigned nr_entries)
{
	if (!nr_entries)
		return 1;

	es->begin = alloc_shm(SHM_ENTRY_SPACE, sizeof(struct entry) * nr_entries);

	if (!es->begin)
	{
		return 1;
	}

	es->end = es->begin + nr_entries;
	return 0;
}

static int unmap_es(struct entry_space *es, unsigned nr_entries)
{
	if (!es->begin || !nr_entries)
		return 1;
	return unmap_shm(es->begin, sizeof(struct entry) * nr_entries);
}

static int unlink_es(void)
{
	return unlink_shm(SHM_ENTRY_SPACE);
}

static struct entry *__get_entry(struct entry_space *es, unsigned block)
{
	struct entry *e;
	e = es->begin + block;
	return e;
}

static unsigned to_index(struct entry_space *es, struct entry *e)
{
	return e - es->begin;
}

static struct entry *to_entry(struct entry_space *es, unsigned block)
{
	if (block == INDEXER_NULL)
		return NULL;
	return __get_entry(es, block);
}

/* --------------------------------------------------- */

static void l_init(struct ilist *l)
{
	l->nr_elts = 0;
	l->head = l->tail = INDEXER_NULL;
}

static struct entry *l_head(struct entry_space *es, struct ilist *l)
{
	return to_entry(es, l->head);
}

static struct entry *l_tail(struct entry_space *es, struct ilist *l)
{
	return to_entry(es, l->tail);
}

static struct entry *l_next(struct entry_space *es, struct entry *e)
{
	return to_entry(es, e->next);
}

static struct entry *l_prev(struct entry_space *es, struct entry *e)
{
	return to_entry(es, e->prev);
}

static int l_empty(struct ilist *l)
{
	return l->head == INDEXER_NULL;
}

static void l_add_head(struct entry_space *es, struct ilist *l, struct entry *e)
{

	struct entry *head = l_head(es, l);

	e->next = l->head;
	e->prev = INDEXER_NULL;

	if (head)
		head->prev = l->head = to_index(es, e);
	else
		l->head = l->tail = to_index(es, e);

	l->nr_elts++;
}

static void l_add_tail(struct entry_space *es, struct ilist *l, struct entry *e)
{

	struct entry *tail = l_tail(es, l);

	e->next = INDEXER_NULL;
	e->prev = l->tail;

	if (tail)
		tail->next = l->tail = to_index(es, e);
	else
		l->head = l->tail = to_index(es, e);

	l->nr_elts++;
}

static void l_del(struct entry_space *es, struct ilist *l, struct entry *e)
{

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

static struct entry *l_pop_head(struct entry_space *es, struct ilist *l)
{

	struct entry *e;
	for (e = l_head(es, l); e; e = l_next(es, e))
	{
		l_del(es, l, e);
		return e;
	}
	return NULL;
}

/* --------------------------------------------------- */

static void init_allocator(struct entry_alloc *ea, struct entry_space *es, unsigned begin, unsigned end)
{

	unsigned i;

	ea->es = es;
	ea->nr_allocated = 0;
	ea->begin = begin;

	l_init(&ea->free);
	for (i = begin; i != end; i++)
	{
		l_add_tail(ea->es, &ea->free, __get_entry(ea->es, i));
	}
}

static void link_allocator(struct entry_alloc *ea, struct entry_space *es)
{
	ea->es = es;
}

static void init_entry(struct entry *e)
{
	e->hash_next = INDEXER_NULL;
	e->prev = INDEXER_NULL;
	e->next = INDEXER_NULL;
	e->param = 0;
	strcpy(e->full_path_name, "");
	e->cache_page_index = INDEXER_NULL;
}

static void entry_set_pending(struct entry *e, bool pending)
{
	if (pending)
	{
		e->param |= 1;
	}
	else
	{
		e->param &= 0;
	}
}

static bool entry_get_pending(struct entry *e)
{
	if (!e)
	{
		printf("MSG: entry is null\n");
		return false;
	}
	unsigned n = e->param;
	return (n << 15) >> 15;
}

static void entry_set_dirty(struct entry *e, bool dirty)
{
	if (dirty)
	{
		e->param |= 2;
	}
	else
	{
		e->param &= ~2;
	}
}

static bool entry_get_dirty(struct entry *e)
{
	if (!e)
	{
		printf("MSG: entry is null\n");
		return false;
	}
	unsigned n = e->param;
	return (n << 14) >> 15;
}

static struct entry *alloc_entry(struct entry_alloc *ea)
{
	struct entry *e;
	if (l_empty(&ea->free))
		return NULL;

	e = l_pop_head(ea->es, &ea->free);
	init_entry(e);

	ea->nr_allocated++;

	return e;
}

static void free_entry(struct entry_alloc *ea, struct entry *e)
{
	ea->nr_allocated--;
	l_add_tail(ea->es, &ea->free, e);
}

static int allocator_empty(struct entry_alloc *ea)
{
	return l_empty(&ea->free);
}

static unsigned get_index(struct entry_alloc *ea, struct entry *e)
{
	return to_index(ea->es, e) - ea->begin;
}

static unsigned infer_cblock(mapping *mapping, struct entry *e)
{
	return get_index(&mapping->ea, e);
}

/* --------------------------------------------------- */

/* 演算法是用chatGPT產生的: djb2 hash */
static unsigned hash_32(char *full_path_name, unsigned cache_page_index, unsigned long long hash_bits)
{
	unsigned hash_val = 5381;
	unsigned len = strlen(full_path_name);

	// hash the integer first
	hash_val = ((hash_val << 5) + hash_val) + cache_page_index;

	// hash the string next
	for (unsigned i = 0; i < len; i++)
	{
		hash_val = ((hash_val << 5) + hash_val) + full_path_name[i]; /* hash * 33 + c */
	}

	hash_val = (hash_val << (32 - hash_bits)) >> (32 - hash_bits);
	return 0;
}

static unsigned roundup_pow_of_two(unsigned n)
{
	unsigned res = 1;
	while (res < n)
	{
		res <<= 1;
	}
	return res;
}

static int alloc_hash_table(struct hash_table *ht, struct entry_space *es, unsigned nr_entries)
{

	unsigned i, nr_buckets;

	ht->es = es;
	nr_buckets = roundup_pow_of_two(nr_entries);
	ht->hash_bits = 31 - __builtin_clz(nr_buckets);
	ht->buckets = alloc_shm(SHM_BUCKETS, nr_buckets * sizeof(*ht->buckets));
	if (!ht->buckets)
	{
		return 1;
	}

	for (i = 0; i < nr_buckets; i++)
		ht->buckets[i] = INDEXER_NULL;

	return 0;
}

static int link_hash_table(struct hash_table *ht, struct entry_space *es, unsigned nr_entries)
{

	unsigned nr_buckets;

	ht->es = es;
	nr_buckets = roundup_pow_of_two(nr_entries);
	ht->hash_bits = 31 - __builtin_clz(nr_buckets);
	ht->buckets = alloc_shm(SHM_BUCKETS, nr_buckets * sizeof(*ht->buckets));
	if (!ht->buckets)
	{
		return 1;
	}

	return 0;
}

static int unmap_hash_table(struct hash_table *ht, unsigned nr_entries)
{
	unsigned nr_buckets = roundup_pow_of_two(nr_entries);
	if (!ht->buckets || !nr_entries)
		return 1;
	return unmap_shm(ht->buckets, nr_buckets * sizeof(*ht->buckets));
}

static int unlink_hash_table(void)
{
	return unlink_shm(SHM_BUCKETS);
}

static struct entry *h_head(struct hash_table *ht, unsigned bucket)
{
	return to_entry(ht->es, ht->buckets[bucket]);
}

static struct entry *h_next(struct hash_table *ht, struct entry *e)
{
	return to_entry(ht->es, e->hash_next);
}

static void __h_insert(struct hash_table *ht, unsigned bucket, struct entry *e)
{
	e->hash_next = ht->buckets[bucket];
	ht->buckets[bucket] = to_index(ht->es, e);
}

static void h_insert(struct hash_table *ht, struct entry *e)
{
	unsigned h = hash_32(e->full_path_name, e->cache_page_index, ht->hash_bits);
	__h_insert(ht, h, e);
}

static struct entry *__h_lookup(struct hash_table *ht, unsigned h, char *full_path_name, unsigned cache_page_index, struct entry **prev)
{

	struct entry *e;
	*prev = NULL;
	for (e = h_head(ht, h); e; e = h_next(ht, e))
	{
		if ((cache_page_index == e->cache_page_index) && (strcmp(full_path_name, e->full_path_name) == 0))
		{
			return e;
		}
		*prev = e;
	}

	return NULL;
}

static void __h_unlink(struct hash_table *ht, unsigned h, struct entry *e, struct entry *prev)
{
	if (prev)
	{
		prev->hash_next = e->hash_next;
	}
	else
	{
		ht->buckets[h] = e->hash_next;
	}
}

static struct entry *h_lookup(struct hash_table *ht, char *full_path_name, unsigned cache_page_index)
{

	struct entry *e, *prev;
	unsigned h = hash_32(full_path_name, cache_page_index, ht->hash_bits);

	e = __h_lookup(ht, h, full_path_name, cache_page_index, &prev);
	if (e && prev)
	{
		/*
		 * Move to the front because this entry is likely
		 * to be hit again.
		 */
		__h_unlink(ht, h, e, prev);
		__h_insert(ht, h, e);
	}

	return e;
}

static void h_remove(struct hash_table *ht, struct entry *e)
{

	unsigned h = hash_32(e->full_path_name, e->cache_page_index, ht->hash_bits);
	struct entry *prev;

	/*
	 * The down side of using a singly linked list is we have to
	 * iterate the bucket to remove an item.
	 */
	e = __h_lookup(ht, h, e->full_path_name, e->cache_page_index, &prev);

	if (e)
	{
		__h_unlink(ht, h, e, prev);
	}
}

/* --------------------------------------------------- */

int init_mapping(mapping *mapping, unsigned block_size, unsigned cblock_num)
{

	int rc = 0;
	mapping->block_size = block_size;
	mapping->cblock_num = cblock_num;

	rc += alloc_es(&mapping->es, mapping->cblock_num);
	if (rc)
	{
		goto end;
	}

	init_allocator(&mapping->ea, &mapping->es, 0, mapping->cblock_num);
	l_init(&mapping->clean);
	l_init(&mapping->dirty);

	rc += alloc_hash_table(&mapping->table, &mapping->es, mapping->cblock_num);
	if (rc)
	{
		goto end;
	}

	mapping->hit_time = 0;
	mapping->miss_time = 0;
	mapping->promotion_time = 0;
	mapping->demotion_time = 0;
	mapping->writeback_time = 0;
	spinlock_init(&mapping->mapping_lock);
end:

	return rc;
}

int link_mapping(mapping *mapping)
{
	spinlock_lock(&mapping->mapping_lock);
	int rc = 0;
	link_allocator(&mapping->ea, &mapping->es);
	rc += alloc_es(&mapping->es, mapping->cblock_num);

	if (rc)
	{
		goto end;
	}
	rc += link_hash_table(&mapping->table, &mapping->es, mapping->cblock_num);
end:
	spinlock_unlock(&mapping->mapping_lock);
	return rc;
}

int free_mapping(mapping *mapping)
{
	spinlock_lock(&mapping->mapping_lock);
	int rc = 0;
	rc += unmap_es(&mapping->es, mapping->cblock_num);
	if (rc)
	{
		goto end;
	}

	rc += unmap_hash_table(&mapping->table, mapping->cblock_num);

end:
	spinlock_unlock(&mapping->mapping_lock);
	return rc;
}

int exit_mapping(void)
{
	int rc = 0;
	rc += unlink_es();
	rc += unlink_hash_table();
	return rc;
}

/* debug */
static void list_entrys_info(mapping *mapping)
{
	printf("---> Information of entrys <---\n");
	for (unsigned i = 0; i < mapping->cblock_num; i++)
	{
		struct entry *e = mapping->es.begin + i;
		printf("/ entry = %s and %u\n", e->full_path_name, e->cache_page_index);
	}
}

void info_mapping(mapping *mapping)
{
	spinlock_lock(&mapping->mapping_lock);
	printf("---> Information of mapping table <---\n");
	printf("/ free  entrys = %u\n", mapping->ea.free.nr_elts);
	printf("/ clean entrys = %u\n", mapping->clean.nr_elts);
	printf("/ dirty entrys = %u\n", mapping->dirty.nr_elts);
	unsigned hit_ratio = safe_div((mapping->hit_time * 100), (mapping->hit_time + mapping->miss_time));
	printf("/ promotion time = %u\n", mapping->promotion_time);
	printf("/ demotion  time = %u\n", mapping->demotion_time);
	printf("/ writeback time = %u\n", mapping->writeback_time);
	printf("/ hit time = %u, miss time = %u, hit ratio = %u%%\n", mapping->hit_time, mapping->miss_time, hit_ratio);
	// list_entrys_info(mapping);
	spinlock_unlock(&mapping->mapping_lock);
}
/* --------------------------------------------------- */

bool do_writeback_work(mapping *mapping)
{
	unsigned cblock;
	unsigned success;
	if (writeback_get_dirty_cblock(mapping, &cblock))
	{
		printf("( SSD to HDD )\n");
		success = true;
		writeback_complete(mapping, &cblock, success);
		return true;
	}
	return false;
}

bool do_migration_work(mapping *mapping)
{
	char full_path_name[MAX_PATH_SIZE];
	unsigned cache_page_index;
	unsigned cblock;
	bool success;
	/* get a work */
	if (peak_work(&mapping->wq, full_path_name, &cache_page_index))
	{
		if (promotion_get_free_cblock(mapping, full_path_name, cache_page_index, &cblock))
		{
			success = true; // HDD to SSD
			promotion_complete(mapping, &cblock, success);
		}
		else if (demotion_get_clean_cblock(mapping, &cblock))
		{
			success = true; // update metadata
			demotion_complete(mapping, &cblock, success);
		}
		else if (writeback_get_dirty_cblock(mapping, &cblock))
		{
			success = true; // SSD to HDD
			writeback_complete(mapping, &cblock, success);
		}
		remove_work(&mapping->wq);
		return true;
	}
	return false;
}

#define to_cache_page_index(page_index) (page_index >> 3)

bool lookup_mapping(mapping *mapping, char *full_path_name, unsigned page_index, unsigned *cblock)
{
	spinlock_lock(&mapping->mapping_lock);
	struct entry *e = h_lookup(&mapping->table, full_path_name, to_cache_page_index(page_index));
	if (e)
	{
		mapping->hit_time++;
		*cblock = infer_cblock(mapping, e);
	}
	else
	{
		mapping->miss_time++;
		insert_work(&mapping->wq, full_path_name, strlen(full_path_name), to_cache_page_index(page_index));
	}
	spinlock_unlock(&mapping->mapping_lock);
	return (e != NULL);
}

bool lookup_mapping_with_insert(mapping *mapping, char *full_path_name, unsigned page_index, unsigned *cblock)
{
	spinlock_lock(&mapping->mapping_lock);
	bool res = true;
	struct entry *e = h_lookup(&mapping->table, full_path_name, to_cache_page_index(page_index));

	if (e)
	{
		mapping->hit_time++;
		*cblock = (e ? infer_cblock(mapping, e) : 0);
		/* hit */
		res = true;
		goto end;
	}
	else
	{
		mapping->miss_time++;
	}

	e = alloc_entry(&mapping->ea);
	if (!e)
	{
		/* insert fail */
		res = false;
		goto end;
	}

	/* build new entry */
	entry_set_pending(e, false);
	entry_set_dirty(e, true);
	strcpy(e->full_path_name, full_path_name);
	e->cache_page_index = to_cache_page_index(page_index);

	/* push into hash table */
	h_insert(&mapping->table, e);
	l_add_tail(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);

	*cblock = infer_cblock(mapping, e);
end:
	spinlock_unlock(&mapping->mapping_lock);
	return res;
}

bool promotion_get_free_cblock(mapping *mapping, char *full_path_name, unsigned cache_page_index, unsigned *cblock)
{
	spinlock_lock(&mapping->mapping_lock);
	bool res = true;
	/* check whether the free list is empty */
	if (allocator_empty(&mapping->ea))
	{
		res = false;
		goto end;
	}
	/* get entry form free list */
	struct entry *e = alloc_entry(&mapping->ea);
	strcpy(e->full_path_name, full_path_name);
	e->cache_page_index = cache_page_index;
	entry_set_pending(e, true);
	*cblock = infer_cblock(mapping, e);
end:
	spinlock_unlock(&mapping->mapping_lock);
	return res;
}

void promotion_complete(mapping *mapping, unsigned *cblock, bool success)
{
	spinlock_lock(&mapping->mapping_lock);
	struct entry *e = to_entry(&mapping->es, *cblock);
	entry_set_pending(e, false);

	if (success)
	{
		// !h, !q, a -> h, q, a
		entry_set_dirty(e, false);
		h_insert(&mapping->table, e);
		l_add_tail(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);
		mapping->promotion_time++;
	}
	else
	{
		// !h, !q, a -> !h, !q, !a
		free_entry(&mapping->ea, e);
	}
	spinlock_unlock(&mapping->mapping_lock);
}

bool demotion_get_clean_cblock(mapping *mapping, unsigned *cblock)
{
	spinlock_lock(&mapping->mapping_lock);
	bool res = true;
	/* get entry from clean list */
	if (mapping->clean.nr_elts <= (mapping->cblock_num >> 1))
	{
		res = false;
		goto end;
	}

	/* get entry form clean list */
	struct entry *e = l_pop_head(&mapping->es, &mapping->clean);
	entry_set_pending(e, true);
	*cblock = infer_cblock(mapping, e);
end:
	spinlock_unlock(&mapping->mapping_lock);
	return res;
}

void demotion_complete(mapping *mapping, unsigned *cblock, bool success)
{
	spinlock_lock(&mapping->mapping_lock);
	struct entry *e = to_entry(&mapping->es, *cblock);
	entry_set_pending(e, false);

	if (success)
	{
		// h, !q, a -> !h, !q, !a
		h_remove(&mapping->table, e);
		free_entry(&mapping->ea, e);
		mapping->demotion_time++;
	}
	else
	{
		// h, !q, a -> h, q, a
		l_add_head(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);
	}
	spinlock_unlock(&mapping->mapping_lock);
}

bool writeback_get_dirty_cblock(mapping *mapping, unsigned *cblock)
{
	spinlock_lock(&mapping->mapping_lock);
	bool res = true;
	/* get entry from dirty list */
	if (!mapping->dirty.nr_elts)
	{
		res = false;
		goto end;
	}

	struct entry *e = l_pop_head(&mapping->es, &mapping->dirty);
	entry_set_pending(e, true);
	*cblock = infer_cblock(mapping, e);
end:
	spinlock_unlock(&mapping->mapping_lock);
	return res;
}

void writeback_complete(mapping *mapping, unsigned *cblock, bool success)
{
	spinlock_lock(&mapping->mapping_lock);
	struct entry *e = to_entry(&mapping->es, *cblock);
	entry_set_pending(e, false);

	if (success)
	{
		// h, !q, a -> h, q, a
		entry_set_dirty(e, false);
		l_add_tail(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);
		mapping->writeback_time++;
	}
	else
	{
		// h, !q, a -> h, q, a
		l_add_head(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);
	}
	spinlock_unlock(&mapping->mapping_lock);
}

void set_dirty_after_write(mapping *mapping, unsigned *cblock, bool dirty)
{
	spinlock_lock(&mapping->mapping_lock);
	struct entry *e = to_entry(&mapping->es, *cblock);
	if (entry_get_pending(e))
	{
		entry_set_dirty(e, dirty);
	}
	else
	{
		l_del(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);
		entry_set_dirty(e, dirty);
		l_add_tail(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);
	}
	spinlock_unlock(&mapping->mapping_lock);
}