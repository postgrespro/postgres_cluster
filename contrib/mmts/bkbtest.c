#include <stdio.h>
#include <stdint.h>
#include "bkb.h"

int main() { 
	nodemask_t matrix[64] = {0};
	nodemask_t clique;
	int clique_size;
	matrix[0] = 6;
	matrix[1] = 4;
	matrix[2] = 1;
	matrix[4] = 3;
	clique = MtmFindMaxClique(matrix, 64, &clique_size);
	printf("Clique=%llx\n", clique);
	return 0;
}
	
