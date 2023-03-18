#include "mapping.h"
#include "cache_type.h"
#include <stdio.h>	/* printf */
#include <stdlib.h> /* malloc */
#include <time.h>	/* random */
#include <pthread.h> /* pthread */

#define test_time 1000000
#define MAX_PAGE 1024
#define to_cache_page_index(page_index) (page_index >> 3)

mapping mp;

/* 使用者們會提交work到work queue */
/* mg_worker 會從work queue取得work */
static void* worker_for_work_queue(void *arg){
	unsigned cblock;
	for(int i = 0; i<test_time;i++){
		char* name = "test";
		unsigned page_index = rand()%MAX_PAGE;
		/* 假設使用者查找，miss的話會提交請求 */
		bool success = lookup_mapping(&mp, name, page_index, &cblock);
		if(!success){
			if((success = promotion_get_free_cblock(&mp, name,  to_cache_page_index(page_index), &cblock))){
				promotion_complete(&mp, &cblock, true);
			}else if((success = demotion_get_clean_cblock(&mp, &cblock))){
				demotion_complete(&mp, &cblock, true);
			}else if((success = writeback_get_dirty_cblock(&mp, &cblock))){
				writeback_complete(&mp, &cblock, true);
			}
		}
	}
	return 0;
}

/* optimizable 會直接處理mapping */
static void* users_for_opt_write(void *arg){
	unsigned cblock;
	for(int i = 0; i<test_time;i++){
		char* name = "test";
		unsigned page_index = (rand()%MAX_PAGE) / 8 * 8;
		bool success = lookup_mapping_with_insert(&mp, name, page_index, &cblock);
	}
	return 0;
}

/* wb_worker 會週期性的發起writeback */
static void* worker_for_period_wb(void *arg){
	unsigned cblock;
	bool success;
	for(int i = 0; i<test_time;i++){
		if((success = writeback_get_dirty_cblock(&mp, &cblock))){
			writeback_complete(&mp, &cblock, true);
		}
	}
	return 0;
}

int main(void){

	srand(time(NULL));
	printf("init rc = %d\n", init_mapping(&mp, 512, 32));

	int users = 10;
	pthread_t wb_worker;
	pthread_t mg_worker;
	pthread_t opt_writer[users];

	pthread_create(&wb_worker, NULL, worker_for_period_wb, NULL);
	pthread_create(&mg_worker, NULL, worker_for_work_queue, NULL);
	for(int i=0;i<users;i++) pthread_create(&opt_writer[i], NULL, users_for_opt_write, NULL);

	pthread_join(wb_worker, NULL);
	pthread_join(mg_worker, NULL);
	for(int i=0;i<users;i++) pthread_join(opt_writer[i], NULL);

	info_mapping(&mp);
	printf("free rc = %d\n", free_mapping(&mp));
	printf("exit rc = %d\n", exit_mapping());
	
}