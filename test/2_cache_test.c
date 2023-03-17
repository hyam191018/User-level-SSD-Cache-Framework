#include "cache_api.h"
#include <stdio.h>	/* printf */

int main(void){

	printf("#### test user link fail\n");
	printf("link rc = %d\n", link_udm_cache()); /* for user */
	info_udm_cache();
	printf("free rc = %d\n", free_udm_cache());	/* for users and daemon*/

	/* for daemon */
	printf("#### test daemon with exit\n");
	printf("init rc = %d\n", init_udm_cache()); /* for daemon */
	info_udm_cache();
	printf("free rc = %d\n", free_udm_cache());	/* for users and daemon*/
	printf("exit rc = %d\n", exit_udm_cache());	/* for daemon */

	printf("#### test daemon withough exit\n");
	printf("init rc = %d\n", init_udm_cache()); /* for daemon */
	info_udm_cache();
	printf("free rc = %d\n", free_udm_cache());	/* for users and daemon*/

	printf("#### test user link success\n");
	printf("link rc = %d\n", link_udm_cache()); /* for user */
	info_udm_cache();
	printf("free rc = %d\n", free_udm_cache());	/* for users and daemon*/

	printf("#### test complete\n");
	printf("exit rc = %d\n", exit_udm_cache());	
}