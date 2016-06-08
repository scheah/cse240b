// CSE240B: BSG Simulation on X86
// Functional equivance with the original bsg utils based on pthread
#include "bsg_util_x86_simul.h"

#include <sys/time.h>

typedef struct {
	int bsg_x;
	int bsg_y;
	void (*body)(int, int);
} tile_struct, * p_tile_struct;

void* _thread_body(void *arg) {
	p_tile_struct p = (p_tile_struct)arg;

	p->body(p->bsg_x, p->bsg_y);

	pthread_exit((void *) 0);
}

static pthread_barrier_t barrier_val;
void init_x86_simul(void (*body)(int, int)) {
	int x, y;
	pthread_t threads[bsg_tiles_X][bsg_tiles_Y];
	tile_struct tile_info[bsg_tiles_X][bsg_tiles_Y];
	pthread_barrier_init(&barrier_val, NULL, bsg_num_tiles);

	for (x = 0; x < bsg_tiles_X; ++x) {
		for (y = 0; y < bsg_tiles_Y; ++y) {
			tile_info[x][y].bsg_x = x;
			tile_info[x][y].bsg_y = y;
			tile_info[x][y].body = body;
			pthread_create(&threads[x][y], NULL, &_thread_body, (void*)&tile_info[x][y]);
		}
	}

	int rc, status;
	int i, j;
	for (i = 0; i < bsg_tiles_X; ++i) {
		for (j = 0; j < bsg_tiles_Y; ++j) {
			rc = pthread_join(threads[i][j], (void **)&status);
			if (rc == 0) {
				printf("Finished tile: %d %d\n", i, j);
			} else {
				printf("E %d %d %d\n", i, j, status);
			}
		}
	}
}

void bsg_print_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);

	long long int milli_time = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
	printf("BSG Time\t%lld\n", milli_time);
}

void bsg_remote_ptr_io_store(int x, int addr, int data) {
	printf("IO %d %08x %08x\n", x, addr, data);
}

void barrier() {
    pthread_barrier_wait(&barrier_val);
}

