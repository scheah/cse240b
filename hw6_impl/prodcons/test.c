// change this header if the machine is not 4x4!
#include "bsg_manycore.h"
#include "prodcons.h"

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

#define PRODUCER_X 0
#define PRODUCER_Y 0
#define CONSUMER_X bsg_tiles_X-1
#define CONSUMER_Y bsg_tiles_Y-1
#define ITERATION 100

int producer_test() {
	int i;
	init_channel(CONSUMER_X, CONSUMER_Y);

	for (i = 0; i < ITERATION; ++i) {
		produce(i);
	}
}

int consumer_test() {
	int i, sum = 0;
	init_channel(PRODUCER_X, PRODUCER_Y);

	for (i = 0; i < ITERATION; ++i) {
		sum += consume();
	}
	
	bsg_remote_ptr_io_store(0,0x1234,sum);
}

int main()
{
  bsg_set_tile_x_y();

  bsg_remote_ptr_io_store(0,0x1260,bsg_x);
  bsg_remote_ptr_io_store(0,0x1264,bsg_y);

  // Test Producer and Consumer
  {
	  if (bsg_x == PRODUCER_X && bsg_y == PRODUCER_Y)
		  producer_test();
	  else if ((bsg_x == CONSUMER_X) && (bsg_y == CONSUMER_Y))
		  consumer_test();
  }

  if ((bsg_x == bsg_tiles_X-1) && (bsg_y == bsg_tiles_Y-1))
    bsg_finish();

  bsg_wait_while(1);
}

