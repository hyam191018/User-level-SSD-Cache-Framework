#include "pio.h"
#include <stdio.h>	/* printf */
#include <stdlib.h>	/* malloc */

int main(void){
	void* buffer = malloc(4096);
	void* buffer2 = malloc(4096);
	void* buffer3 = malloc(4096);
	struct pio* pio = create_pio("test", 0, READ, buffer, 3);
	append_pio(pio, buffer2);
	append_pio(pio, buffer3);
	print_pio(pio);
	free_pio(pio);
}
