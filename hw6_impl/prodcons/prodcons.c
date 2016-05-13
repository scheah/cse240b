#include "prodcons.h"
#include "bsg_manycore_2x2.h"

// Destination tiles
int dest_x;
int dest_y;

// Queue management
int local_queue[PD_FIFO_SIZE]; // circular queue (consumer side one will be used)
int queue_head; // should be syncronized each other (Updated by consumer)
int queue_tail; // should be syncronized each other (Updated by producer)

void init_channel(int x, int y) {
	dest_x = x;
	dest_y = y;

	// Init queue positions each other
	bsg_remote_store(x,y,&queue_head,0);
	bsg_remote_store(x,y,&queue_tail,0);
}

void produce(int v) {
	// Wait until it's not full
	bsg_wait_while(
			(bsg_volatile_access(queue_head) - bsg_volatile_access(queue_tail) == 1) || 
			(bsg_volatile_access(queue_tail) - bsg_volatile_access(queue_head) == PD_FIFO_SIZE - 1)
 	);

	// Store data to the remote consumer's queue
	bsg_remote_store(dest_x, dest_y, &local_queue[queue_tail], v);

	// Position update
	queue_tail++;
	if (queue_tail == PD_FIFO_SIZE) // circular queue condition
		queue_tail = 0;

	// Update consumer's tail position
	bsg_remote_store(dest_x, dest_y, &queue_tail, queue_tail);
}

int consume() {
	int v;

	// Wait until it's not empty
	bsg_wait_while(
			(bsg_volatile_access(queue_head) == bsg_volatile_access(queue_tail))
 	);

	// Read data
	v = local_queue[queue_head];

	// Position update
	queue_head++;
	if (queue_head == PD_FIFO_SIZE) // circular queue condition
		queue_head = 0;

	// Update producer's head position
	bsg_remote_store(dest_x, dest_y, &queue_head, queue_head);

	return v;
}
