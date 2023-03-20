#include "mapping.h"
#include "work_queue.h"
#include "cache_type.h"
#include <stdio.h>	/* printf */
#include <stdlib.h> /* malloc */
#include <time.h>	/* random */
#include <pthread.h> /* pthread */

#define test_time 10000
#define MAX_PAGE 1024
#define to_cache_page_index(page_index) (page_index >> 3)

mapping mp;
work_queue wq;

static void* user_func(void *arg){

	for(int i = 0; i<test_time;i++){
		char* name = "test";
		unsigned page_index = rand()%MAX_PAGE;
		unsigned cblock;
		if(lookup_mapping(&mp, name, page_index, &cblock)){
				
		}else{
			if(push_work(&wq, name, strlen(name), to_cache_page_index(page_index))){
				printf("Producer: %s %u\n", name, to_cache_page_index(page_index));	
			}		
		}
		
	}
	return NULL;
}

static void* worker_func(void *arg){
	for(int i = 0; i<test_time*10;i++){
		char name[MAX_PATH_SIZE];
		unsigned cache_page_index;
		unsigned cblock;
		bool success;
		if(peak_work(&wq, name, &cache_page_index)){
			printf("Consumer: %s %u\n", name, cache_page_index);
			remove_work(&wq);
            if(promotion_get_free_cblock(&mp, name, cache_page_index, &cblock)){
                success = true; // HDD to SSD        
                promotion_complete(&mp, &cblock, success);
            }else if(demotion_get_clean_cblock(&mp, &cblock)){
                success = true; // update metadata
                demotion_complete(&mp, &cblock, success);
            }else if(writeback_get_dirty_cblock(&mp, &cblock)){
                success = true; // SSD to HDD
                writeback_complete(&mp, &cblock, success);
            }
		}else{

		}
	}
	return NULL;
}

int main(void){

	srand(time(NULL));
	printf("init rc = %d\n", init_mapping(&mp, 512, 32));

	int users = 10;
	int workers = 1;
	pthread_t user[users];
	pthread_t worker[workers];

	for(int i=0;i<users;i++) pthread_create(&user[i], NULL, user_func, NULL);
	for(int i=0;i<workers;i++) pthread_create(&worker[i], NULL, worker_func, NULL);

	for(int i=0;i<users;i++) pthread_join(user[i], NULL);
	for(int i=0;i<workers;i++) pthread_join(worker[i], NULL);

	info_mapping(&mp);
	printf("free rc = %d\n", free_mapping(&mp));
	printf("exit rc = %d\n", exit_mapping());
	
}