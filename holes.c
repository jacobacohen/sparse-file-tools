#define _GNU_SOURCE

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

extern int errno;

//open file for reading and call hole routine
int main(int argc, char *argv[]) {
	FILE *regular_input;
	int enhanced_input, random_input;
	regular_input = fopen(argv[1], "r");
	printf("regular run\n");
	hole_detect_seek_regular(regular_input, argv[1]);
	fclose(regular_input);
	enhanced_input = open(argv[1], O_RDONLY);
	printf("enhanced run\n");
	hole_detect_seek_enhanced(enhanced_input, argv[1]);
	close(enhanced_input);
	random_input = open(argv[1], O_RDONLY);
	printf("random run\n");
	hole_detect_seek_random(random_input, argv[1]);
	close(random_input);
	return 0;
}


// Basic hole detection
// Linux 3.1 Hole detection with SEEK_DATA and SEEK_HOLE
int hole_detect_seek_enhanced(int input, char *filename) {
	unsigned long long cur_hole_size = 0L, cur_data_size = 0L;
	unsigned long long tot_hole_size = 0L;
	off_t pos = 0, hole_move=0, data_move = 0, eof = 0;
	int errnum; 
	// stat file if needed

	// get end of file
	eof = lseek(input,0,SEEK_END);
	printf("%d\n", eof);
	// begin seek
	pos=lseek(input,0,SEEK_SET);
	printf("starting read at: %lu\n", pos);	
	printf("move to hole %lu\n", lseek(input,pos,SEEK_HOLE));
	printf("move to data %lu\n", lseek(input,pos,SEEK_DATA));
	// attempt next seek to data, followed by an attempt to next seek to the next hole if that fails
	while (pos >= 0 && pos != eof) {
		// check results of hole and data seeking to figure out where to go
		hole_move = lseek(input, pos, SEEK_HOLE);
		data_move = lseek(input, pos, SEEK_DATA);
		if (hole_move < data_move) {
			// we are in a hole, use the start of the next data to benchmark
			cur_hole_size = (long) data_move - (long) pos;
			printf("HOLE: %lu\n", cur_hole_size);
			tot_hole_size += cur_hole_size; 
			pos = data_move;
		}
		else {
			// we are in data, use the start of the next hole to benchmark
			cur_data_size = (long) hole_move - (long) pos;
			printf("DATA: %lu\n", cur_data_size);
			pos = hole_move;
		}
	printf("cur position: %lu\n", pos);
	}
	return 0;
}

// generate random bytes to read mod file size
// worry about duplicate points?
int hole_detect_seek_random(int input, char *filename) {
	// Select 200 random points on the file to seek to and read
	static char zeroes[4097];
	char buffer[sizeof(zeroes)];
	unsigned long long read_points[200];
	struct stat file_stat;
	off_t pos;
	int data_hits = 0, hole_hits = 0;

	stat(filename, &file_stat);
	unsigned long long file_size = file_stat.st_size;

	srand(time(0));
	// fill read_points array
	int i, array_size;
	array_size = sizeof(read_points)/sizeof(unsigned long long);
	for (i = 0; i < array_size; i++) {
		// multiply casted ints to get random starting points
		read_points[i] = ((unsigned long long) rand() * (unsigned long long) rand()) % file_size;
	}
	// seek to each 
	for (i = 0; i < array_size; i++) {
		pos=lseek(input, (off_t) read_points[i], SEEK_SET);
		// fread and compare to figure out if we're on a hole
		if ( read(input, buffer, sizeof(buffer)) > 0 ) {
			// found hole
			if (!memcmp(buffer, zeroes, sizeof(zeroes)))
				hole_hits += 1;
			else
				data_hits += 1;
		}
	}
	printf("HOLE HITS: %d\n", hole_hits);
	printf("DATA HITS: %d\n", data_hits);

	return 0;
}


int hole_detect_seek_regular(FILE *input, char *filename) {
	static char zeroes[4097];
	char buffer[sizeof(zeroes)];
	unsigned long long cur_hole_size = 0L, cur_data_size = 0L;
	unsigned long long tot_hole_size = 0L;
	// stat file to get total length
	struct stat file_stat;
	// error check stat call?
	stat(filename, &file_stat);
	unsigned long long file_size = file_stat.st_size;
	printf("stat reported file size:%llu\n", file_size);

	// get starting position
	// pos = lseek(input, 0L, SEEK_SET);
	if (input != NULL) {
		while (fread(buffer, sizeof(*buffer), sizeof(buffer), input) > 0) {
			// compare to an empty 4k block to disover or add to hole
			if (!memcmp(buffer, zeroes, sizeof(zeroes))) {
				//printf("%s\n", "found hole");
				// print the data block if we encountered it, then clear data block
				cur_data_size == 0L ? 0 : printf("DATA:\t%llu\n", cur_data_size);
				cur_data_size = 0L;
				// add to hole size
				cur_hole_size += (unsigned long)sizeof(buffer)-1;
				tot_hole_size += (unsigned long)sizeof(buffer)-1;
			}
			else {
				//printf("%s\n", "not hole");
				// print the hole block if we encountered it, then clear hole block
				cur_hole_size == 0L ? 0 : printf("HOLE:\t%llu\n", cur_hole_size);
				cur_hole_size = 0L;
				// add to hole size
				cur_data_size += (unsigned long)sizeof(buffer)-1;
			}	
		}
	}
	// print the non-zero one at the end
	cur_hole_size == 0L ? printf("DATA:\t%llu\n", cur_data_size) : printf("HOLE\t%llu\n", cur_hole_size);
	// give total sparse percentage
	printf("How much of file was sparse: ~%f\n", (tot_hole_size / (float) file_size));

	return 0;
}
