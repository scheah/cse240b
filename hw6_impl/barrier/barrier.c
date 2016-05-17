#include "barrier.h"
#include "bsg_manycore.h"

int barrier_array[bsg_tiles_X][bsg_tiles_Y]; // Should be syncronized each other
int barrier_count; // Locally maintained

int is_still_barrier_waiting() {
	int i, j;
	for (i = 0; i < bsg_tiles_X; ++i) {
		for (j = 0; j < bsg_tiles_Y; ++j) {
			if (bsg_volatile_access(barrier_array[i][j]) < barrier_count)
				return 1;
		}
	}

	return 0;
}

#define USE_BROADCAST_IMPL
#ifdef USE_BROADCAST_IMPL
void barrier(int x, int y) {
	int i, j, tmp;
	barrier_array[x][y] = ++barrier_count;

	bsg_compiler_memory_barrier();

	// Broadcast barrier_count
	for (i = 0; i < bsg_tiles_X; ++i) {
		for (j = 0; j < bsg_tiles_Y; ++j) {
			bsg_remote_store(i,j,&barrier_array[x][y],barrier_count);
		}
	}
		
	bsg_compiler_memory_barrier();

	// Wait until it's resolved
	while (1) {
		if (is_still_barrier_waiting() == 0)
			break;
	}
}
#else
// Centralized control
//#define BARRIER_CENTRAL_X 0 
//#define BARRIER_CENTRAL_Y 0
//int barrier_central_count; // Updated by the central node
//
//void barrier(int x, int y) {
//	barrier_array[x][y] = ++barrier_count;
//
//	if (x == BARRIER_CENTRAL_X && y == BARRIER_CENTRAL_Y) {
//		// Case 1 (central node): wait until all are resolved
//		while (1) {
//			if (!is_still_barrier_waiting())
//				break;
//		}
//
//		// Update its own barrier mincount
//		barrier_central_count = barrier_count;
//	} else {
//		// Case 2 (non-central nodes): send barrier_count to the central node,
//		// and wait until it is resolved by the propagation below
//		bsg_remote_store(BARRIER_CENTRAL_X,BARRIER_CENTRAL_Y,&barrier_array[x][y],barrier_count);
//
//		bsg_compiler_memory_barrier();
//
//		bsg_wait_while(
//			(bsg_volatile_access(barrier_central_count) < barrier_count)
//		);
//	}
//
//	// propagate to next column
//	if ((x == 0)
//			&& ((y + 1) != bsg_tiles_Y)
//	   )
//	{
//		bsg_remote_store(0,y+1,&barrier_central_count,barrier_central_count);
//	}
//
//	// propagate across each row
//	if ((x+1) != bsg_tiles_X)
//	{
//		bsg_remote_store(x+1,y,&barrier_central_count,barrier_central_count);
//	}
//}

#endif
