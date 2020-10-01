#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

int main(int argc, char **argv) {
	char *filename="hole_file";
	int fd = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0666);
	int offset = (argc > 1) ? atoi(argv[1]) : 1000000;
	struct stat statbuf;

	write(fd, "S", 5);
	lseek(fd, offset, SEEK_SET);
	write(fd, "E", 5);
	close(fd);
	stat(filename, &statbuf);
	printf("%s - %lld size with %lld sectors\n", filename, (long long) statbuf.st_size, (long long) statbuf.st_blocks);

	return 0;
}
