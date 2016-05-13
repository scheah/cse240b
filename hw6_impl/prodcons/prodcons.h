// A single producer - consumer model library
// Each core can be either producer or consumer at a time.

#ifndef _PRODCONS_H_
#define _PRODCONS_H_

#define PD_FIFO_SIZE 2 // FIFO SIZE

void init_channel(int x, int y);
void produce(int v);
int consume();

#endif
