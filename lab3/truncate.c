/*
 * Expose truncate(2) as a program.
 */

#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	const char *filename;
	off_t trunced_len;
	int r;

	if (argc != 3)
	{
		printf("Usage: %s <filename> <desired_length>\n", argv[0]);
		printf("Calls truncate(filename, desired_length).\n");
		exit(1);
	}

	filename = argv[1];
	trunced_len = atoi(argv[2]);

	r = truncate(filename, trunced_len);
	if (r < 0)
	{
		perror("truncate");
		return 1;
	}

	return 0;
}
