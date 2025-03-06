/* Copyright (C) 1999 Lucent Technologies */
/* Excerpted from 'The Practice of Programming' */
/* by Brian W. Kernighan and Rob Pike */

#include <stdio.h>
#include <ctype.h>

void strings(FILE *);

enum { MINLEN = 4 };

/* strings main: find printable strings in files */
int main(int argc, char *argv[])
{
	int i;
	FILE *fin;

	if (argc == 1)
		fprintf(stderr, "usage: %s filenames\n", argv[0]);
	else {
		for (i = 1; i < argc; i++) {
			if ((fin = fopen(argv[i], "rb")) == NULL)
				fprintf(stderr, "%s: can't open %s\n", argv[0], argv[i]);
			else {
				strings(fin);
				fclose(fin);
			}
		}
	}
	return 0;
}

/* strings: extract printable strings from stream */
void strings(FILE *fin)
{
	int c, i;
	char buf[BUFSIZ];

	do {	/* once for each string */
		for (i = 0; (c = getc(fin)) != EOF; ) {
			if (!isprint(c))
				break;
			buf[i++] = c;
			if (i >= BUFSIZ)
				break;
		}
		buf[i] = 0;
		if (i >= MINLEN) /* print if long enough */
			printf("%s\n", buf);
	} while (c != EOF);
}
