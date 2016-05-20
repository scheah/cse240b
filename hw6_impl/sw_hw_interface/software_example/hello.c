
#include "bsg_manycore.h"

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

volatile int tmp;
volatile int tmp2;

int main()
{
  bsg_set_tile_x_y();

  if (bsg_x == 1 && bsg_y == 1) {
	  tmp = *bsg_remote_ptr(0, 1, 0xABCD0);
	  bsg_remote_ptr_io_store(0, 0x2000, tmp);
	  tmp = *bsg_remote_ptr(0, 1, 0xBCDE0);
	  bsg_remote_ptr_io_store(0, 0x2000, tmp);
  } else if (bsg_x == 1 && bsg_y == 0) {
	  tmp2 = 0xCDEF;
	  tmp = tmp2;
	  bsg_remote_ptr_io_store(0, 0x2000, tmp);
  }

  if ((bsg_x == bsg_tiles_X-1) && (bsg_y == bsg_tiles_Y-1))
    bsg_finish();

  bsg_wait_while(1);
}

