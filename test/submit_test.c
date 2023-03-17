#include "cache_api.h"
#include <stdio.h>	/* printf */
#include <stdlib.h> /* malloc */
#include <time.h>	/* random*/

static int test_submit(void){
	void* buffer1 = malloc(PAGE_SIZE);
	void* buffer2 = malloc(PAGE_SIZE);

	unsigned page_index = rand()%256;
	unsigned pio_cnt = 8 - (page_index%8);
	struct pio* pio = create_pio("testpio", page_index, READ, buffer1, pio_cnt);
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

int main(void){

	printf("init rc = %d\n", init_udm_cache()); /* for daemon */
	info_udm_cache();

/*
	srand(time(NULL));
	printf("MSG: random test submit pio\n");
	int res = 0;
	for(int i=0;i<100000;i++){
		res += test_submit();
	}
	printf("res = %d\n", res);
*/

	
	printf("free rc = %d\n", free_udm_cache());	/* for all */
	printf("exit rc = %d\n", exit_udm_cache());	/* for daemon */
}