#include "cache_api.h"
#include <stdio.h>	/* printf */
#include <stdlib.h> /* malloc */
#include <time.h>	/* random */
#include <pthread.h> /* pthread */

static int test_submit(unsigned page_index){
	void* buffer1 = malloc(PAGE_SIZE);
	void* buffer2 = malloc(PAGE_SIZE);
;
	unsigned pio_cnt = 8 - (page_index%8);
	struct pio* pio = create_pio("testpio", page_index, WRITE, buffer1, pio_cnt);
	append_pio(pio, buffer2);
	if(submit_pio(pio) != 0){
		return 1;
	}
	// print_pio(pio);
	free_pio(pio);
	free(buffer1);
	free(buffer2);
	return 0;
}

static void* myloop(void* arg){
	int res = 0;
	for(int i=0;i<1000000;i++){
		res += test_submit(rand()%1024);
	}
	printf("res = %d\n", res);
	pthread_exit(NULL);
	return 0;
}

int main(void){

	srand(time(NULL));
	printf("init rc = %d\n", init_udm_cache()); 
	printf("free rc = %d\n", free_udm_cache());	
	printf("link rc = %d\n", link_udm_cache()); 

	pthread_t t[5];
	for(int i=0;i<5;i++){
		pthread_create(&t[i], NULL, myloop, NULL);
	}
	for(int i=0;i<5;i++){
		pthread_join(t[i], NULL);
	}

	info_udm_cache();
	
	printf("free rc = %d\n", free_udm_cache());
	printf("exit rc = %d\n", exit_udm_cache());	
}