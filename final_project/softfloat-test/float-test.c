#include "bsg_manycore.h"
#include "softfloat.h"

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

volatile float32_t f1;
volatile float32_t f2;
volatile float32_t f3;

volatile float32_t f4;
volatile float32_t f5;
volatile float32_t f6;
volatile float32_t f7;


#define bsg_volatile_access_float(var)        (*((volatile float32_t*) (&(var))))

int main()
{
	/* code */
  bsg_set_tile_x_y();

  bsg_remote_ptr_io_store(0,0x1260,bsg_x);
  bsg_remote_ptr_io_store(0,0x1264,bsg_y);

  f1 = ui32_to_f32(5);
  f2 = ui32_to_f32(10);
  f3 = f32_div(f1,f2); // 0.5f

  f4 = ui32_to_f32(7);
  f5 = ui32_to_f32(10);
  f6 = f32_div(f4,f5); // 0.7f

  if (f32_lt(bsg_volatile_access_float(f6), bsg_volatile_access_float(f3))) // f3 << f6
  {
	  bsg_remote_ptr_io_store(0,0x1234,0x1);
  } else {
	  bsg_remote_ptr_io_store(0,0x1234,0x2);
  }

  f7 = f32_add(f3, f6); // 1.2f;
  f7 = f32_mul(f7, ui32_to_f32(10)); // 12
  int tmp = f32_to_i32(f7, 1, true);
  bsg_remote_ptr_io_store(0,0x2000,tmp); // IO to 0x0000000c

  if ((bsg_x == bsg_tiles_X-1) && (bsg_y == bsg_tiles_Y-1))
    bsg_finish();

  bsg_wait_while(1);

}
