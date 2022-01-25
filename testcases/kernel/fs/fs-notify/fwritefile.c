#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
	int fd;
	char buf[BUFSIZ];
	FILE *f;

	memset(buf, 1, BUFSIZ);
	while(1) {
		f = fopen(argv[1], "w+");
		if (f == NULL) {
			perror("fwritefile fopen");
			exit(EXIT_FAILURE);
		}
		fwrite(buf, sizeof(char), BUFSIZ, f);
		usleep(1);
	}
	fclose(f);
	return 0;
}
