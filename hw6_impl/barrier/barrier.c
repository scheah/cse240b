#include "barrier.h"
#include "bsg_manycore_2x2.h"

int barrier_array[bsg_tiles_X][bsg_tiles_Y]; // Should be syncronized each other
int barrier_count; // Locally maintained

int is_still_barrier_waiting() {
	int i, j;
	for (i = 0; i < bsg_tiles_X; ++i)
		for (j = 0; j < bsg_tiles_Y; ++j) {
			if (bsg_volatile_access(barrier_array[i][j]) < barrier_count)
				return 1;
		}

	return 0;
}

void barrier(int x, int y) {
	int i, j;
	barrier_array[x][y] = ++barrier_count;

	// Broadcast barrier_count
	for (i = 0; i < bsg_tiles_X; ++i)
		for (j = 0; j < bsg_tiles_Y; ++j) {
			bsg_remote_store(i,j,&barrier_array[x][y],barrier_count);
		}

	// Wait until it's resolved
	while (1) {
		if (!is_still_barrier_waiting())
			break;
	}
}

