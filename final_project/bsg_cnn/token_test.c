#include "bsg_util_x86_simul.h"
#include "bsg_token_queue_x86_simul.h"

bsg_declare_token_queue(tq);

#define kBufferWindows 2
#define kTransmitSize 100
#define kBufferSize (kTransmitSize*kBufferWindows)
#define kBlocks 100

int BSG_VAR(buffer)[kBufferSize+10];

int source_process(int *ptr)
{
	int j;
	for (j = 0; j < kTransmitSize; j+=2)
	{
		ptr[j]   = j;
		ptr[j+1] = j;
	}
}

int dest_process(int sum, int *ptr)
{
	int i;
	for (i = 0; i < kTransmitSize; i++) {
		sum += *ptr++;
	}

	bsg_remote_ptr_io_store(0, 0xCAB0, sum);

	return sum;
}

void body(int tile_x, int tile_y)
{
	int bsg_x, bsg_y;
	bsg_x = tile_x;
	bsg_y = tile_y;

	int i;

	int id = bsg_x_y_to_id(bsg_x,bsg_y);
	int *ptr = bsg_remote_ptr(1,0,buffer);
	int bufIndex = 0;

	bsg_print_time();

	if (id == 0)
	{
		int seq = 0;
		bsg_print_time();
		bsg_token_connection_t conn = bsg_tq_send_connection(tq,1,0 _BSG_TILE_ARG_);

		for (i = 0; i < kBlocks; i++)
		{
			// ensure that at least a frame is available to write
			// we could have also counted in terms of kTransmitSize word buffers
			// with input parameters of kBufferWindows,1
			bsg_tq_sender_confirm(conn,kBufferWindows,1);

			source_process(&ptr[bufIndex*kTransmitSize]);

			bufIndex++;
			if (bufIndex == kBufferWindows)
				bufIndex = 0;

			bsg_tq_sender_xfer(conn,kBufferWindows,1);
		}
	}
	else if (id == 1)
	{
		int sum=0;
		bsg_token_connection_t conn = bsg_tq_receive_connection(tq,0,0 _BSG_TILE_ARG_);
		for (i = 0; i < kBlocks; i++)
		{
			int * ptr = BSG_VAR_SEL(buffer);

			// ensure that at least a frame is available to write
			// we could have also counted in terms of kTransmitSize word buffers
			// with input parameters of kBufferWindows,1

			bsg_tq_receiver_confirm(conn,1);

			sum = dest_process(sum,&ptr[bufIndex*kTransmitSize]);

			bufIndex++;

			if (bufIndex == kBufferWindows)
				bufIndex = 0;

			bsg_tq_receiver_release(conn,1);
		}

		printf("SUM: %d\n", sum);
	}
}


int main() {
	init_x86_simul(body);
	return 0;
}
