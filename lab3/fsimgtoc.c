#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

/****************************************************************************
 * fsimgtoc
 *
 *   Reads in a file system image and writes out C code containing that image.
 *
 ****************************************************************************/

static int designated_initializers = 1;

void
print(FILE *f, long size, FILE *out)
{
	int c;
	long n = 0;
	long last = 0;
	long printed = 0;

	fprintf(out, "unsigned char ospfs_data[%ld] = {\n", size);
	c = getc(f);
	while (c != EOF) {
		if (c == 0 && designated_initializers)
			goto skip;
		else if (last <= n - 4)
			fprintf(out, "[%ld]=", n);
		else
			for (; last < n; last++)
				fprintf(out, "0,");
		fprintf(out, "%d,", c);
		last = n + 1;
		if (++printed % 19 == 0)
			fprintf(out, "\n");
	skip:
		n++;
		c = getc(f);
	}
	fprintf(out, "};\nuint32_t ospfs_length = %lu;\n", size);
}

int
main(int argc, char *argv[])
{
	FILE *in = stdin, *out = stdout;
	long in_size;

	if (argc > 3) {
		fprintf(stderr, "Usage: fsimgtoc [IN [OUT]]\n");
		exit(1);
	}
	if (argc > 2 && strcmp(argv[2], "-") != 0
	    && (out = fopen(argv[2], "wb")) == 0) {
		perror(argv[2]);
		exit(1);
	}
	if (argc > 1 && strcmp(argv[1], "-") != 0
	    && (in = fopen(argv[1], "rb")) == 0) {
		perror(argv[1]);
		exit(1);
	}

	// find file size
	if (fseek(in, 0, SEEK_END) < 0) {
		perror(argv[1]);
		exit(1);
	}
	in_size = ftell(in);
	if (fseek(in, 0, SEEK_SET) < 0) {
		perror(argv[1]);
		exit(1);
	}
	
	fprintf(out, "#include <linux/autoconf.h>\n\
#include <linux/version.h>\n\
#include <linux/module.h>\n\
#include <linux/types.h>\n\
\n");
	print(in, in_size, out);
	
	exit(0);
}
