#include "mapping.h"
#include "cache_type.h"
#include <stdio.h>	/* printf */
#include <stdlib.h> /* malloc */
#include <time.h>	/* random */
#include <pthread.h> /* pthread */

#define users 1
#define test_time 1000000
#define EXCEPT 50	// 期望的hit ration
#define to_cache_page_index(page_index) (page_index >> 3)

// 期望的hit ratio = 裝置大小 / 檔案大小
// 50% , 裝置有100G ， 那檔案應該要有200G
const int MAX_PAGE_INDEX = CACHE_BLOCK_NUMBER * CACHE_BLOCK_SIZE / 1024 * 100 / EXCEPT / 4;


mapping mp;

static void* user_func(void *arg){
	for(int i = 0; i< test_time;i++){
		char* name = "test";
		unsigned page_index = rand()%MAX_PAGE_INDEX;
		unsigned cblock;
		lookup_mapping(&mp, name, page_index, &cblock) ? printf("hit\n") : printf("miss\n");	
	}
	return NULL;
}

static void* worker_func(void *arg){
	for(int i = 0; i< test_time * users;i++){
		do_migration_work(&mp);
	}
	return NULL;
}

int main(void){

	srand(time(NULL));
	printf("init rc = %d\n", init_mapping(&mp, 512, CACHE_BLOCK_NUMBER));


	pthread_t user[users];
	pthread_t worker;
	for(int i=0;i<users;i++) pthread_create(&user[i], NULL, user_func, NULL);
	pthread_create(&worker, NULL, worker_func, NULL);

	for(int i=0;i<users;i++) pthread_join(user[i], NULL);
	pthread_join(worker, NULL);

	info_mapping(&mp);
	printf("free rc = %d\n", free_mapping(&mp));
	printf("exit rc = %d\n", exit_mapping());
	
}