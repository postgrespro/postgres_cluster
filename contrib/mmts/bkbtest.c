#include <stdio.h>
#include <stdint.h>
#include "bkb.h"

int main() { 
	nodemask_t matrix[64] = {0};
	nodemask_t clique;
	matrix[0] = 6;
	matrix[1] = 4;
	matrix[2] = 1;
	matrix[4] = 3;
	clique = MtmFindMaxClique(matrix, 64);
	printf("Clique=%lx\n", clique);
	return 0;
}
	
