#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blockmem.h"

#define BLOCKMEM_SIZE (1024 * 1024)
#define TEST_SIZE (10000)

int
main()
{
	int rc = EXIT_SUCCESS;
	void *origin = malloc(BLOCKMEM_SIZE);
	if (!blockmem_format(origin, BLOCKMEM_SIZE))
	{
		fprintf(stderr, "couldn't format the blockmem\n");
		rc = EXIT_FAILURE;
	}
	else
	{
		char *a, *b;
		int i, id;
		
		a = malloc(TEST_SIZE);
		b = malloc(TEST_SIZE);

		for (i = 0; i < TEST_SIZE; i++)
		{
			a[i] = rand() % ('z' - 'a' + 1) + 'a';
			b[i] = '\0';
		}
		a[TEST_SIZE - 1] = '\0';

		id = blockmem_put(origin, a, TEST_SIZE);
		i = blockmem_get(origin, id, b, TEST_SIZE);
		if (i != TEST_SIZE)
		{
			fprintf(stderr, "got %d instead of %d\n", i, TEST_SIZE);
			rc = EXIT_FAILURE;
		}

		if (strcmp(a, b))
		{
			fprintf(stderr, "did not get out what was put in\n");
			rc = EXIT_FAILURE;
		}

		free(b);
		free(a);

		free(origin);
	}

	if (rc == EXIT_SUCCESS)
		fprintf(stderr, "blockmem test ok\n");
	else
		fprintf(stderr, "blockmem test failed\n");

	return rc;
}
