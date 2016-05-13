// change this header if the machine is not 4x4!
#include "bsg_manycore_2x2.h"
#include "barrier.h"

// these are private variables
// we do not make them volatile
// so that they may be cached

int bsg_x = -1;
int bsg_y = -1;

int bsg_set_tile_x_y()
{
  // everybody stores to tile 0,0
  bsg_remote_store(0,0,&bsg_x,0);
  bsg_remote_store(0,0,&bsg_y,0);

  // make sure memory ops above are not moved down
  bsg_compiler_memory_barrier();

  // wait for my tile number to change
  bsg_wait_while((bsg_volatile_access(bsg_x) == -1) || (bsg_volatile_access(bsg_y) == -1));

  // make sure memory ops below are not moved above
  bsg_compiler_memory_barrier();

  // head of each column is responsible for
  // propagating to next column
  if ((bsg_x == 0)
      && ((bsg_y + 1) != bsg_tiles_Y)
    )
  {
    bsg_remote_store(0,bsg_y+1,&bsg_x,bsg_x);
    bsg_remote_store(0,bsg_y+1,&bsg_y,bsg_y+1);
  }

  // propagate across each row
  if ((bsg_x+1) != bsg_tiles_X)
  {
    bsg_remote_store(bsg_x+1,bsg_y,&bsg_x,bsg_x+1);
    bsg_remote_store(bsg_x+1,bsg_y,&bsg_y,bsg_y);
  }
}

int square(int c)
{
  return c*c;
}

#define ITERATION 3

int barrier_test_simple() {
	// Every core doing same # of instructions before reaching to the barrier.
	int i, t;

	for (i = 0; i < ITERATION; ++i) {
		for (t = 0; t < 4; ++t) {
			bsg_remote_ptr_io_store(0,0x2000+bsg_x*0x04+bsg_y*0x10,0);
		}

		bsg_remote_ptr_io_store(0,0x3000,bsg_x);
		bsg_remote_ptr_io_store(0,0x3004,bsg_y);
		barrier(bsg_x, bsg_y);
		bsg_remote_ptr_io_store(0,0x4000,bsg_x);
		bsg_remote_ptr_io_store(0,0x4004,bsg_y);
	}
}


int barrier_test_complex() {
	// Every core executes different # of instructions before reaching to the barrier.
	int i, t;

	for (i = 0; i < ITERATION; ++i) {
		for (t = 0; t < bsg_x+2; ++t) {
			bsg_remote_ptr_io_store(0,0x2000+bsg_x*0x04+bsg_y*0x10,0);
		}

		for (t = 0; t < bsg_y+3; ++t) {
			bsg_remote_ptr_io_store(0,0x2000+bsg_x*0x04+bsg_y*0x10,0);
		}

		bsg_remote_ptr_io_store(0,0x3000,bsg_x);
		bsg_remote_ptr_io_store(0,0x3004,bsg_y);
		barrier(bsg_x, bsg_y);
		bsg_remote_ptr_io_store(0,0x4000,bsg_x);
		bsg_remote_ptr_io_store(0,0x4004,bsg_y);
	}
}

int main()
{
  bsg_set_tile_x_y();

  bsg_remote_ptr_io_store(0,0x1260,bsg_x);
  bsg_remote_ptr_io_store(0,0x1264,bsg_y);

  // Test barrier library
  {
	  //barrier_test_simple();
	  barrier_test_complex();
  }

  if ((bsg_x == bsg_tiles_X-1) && (bsg_y == bsg_tiles_Y-1))
    bsg_finish();

  bsg_wait_while(1);
}

