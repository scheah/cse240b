// CSE240B: BSG Simulation on X86 - Token queue
// Same usage, but implemented on pthread and mutex

#ifndef _BSG_TOKEN_QUEUE_H_SIMUL_
#define _BSG_TOKEN_QUEUE_H_SIMUL_

#include "bsg_token_queue_structure.h"

#define bsg_declare_token_queue(x) bsg_token_pair_t BSG_VAR(x) [bsg_tiles_X][bsg_tiles_Y] = {0,0}

pthread_mutex_t  bsg_simul_token_mutex = PTHREAD_MUTEX_INITIALIZER;

inline bsg_token_connection_t bsg_tq_send_connection (bsg_token_pair_t BSG_VAR(token_array)[bsg_tiles_X][bsg_tiles_Y], int x, int y, int tile_x, int tile_y)
  {
    bsg_token_connection_t conn;
    
    conn.local_ptr  = &(BSG_VAR_SEL(token_array)[x][y]);
    conn.remote_ptr = &(bsg_remote_ptr(x,y,token_array)[tile_x][tile_y].send);

    return conn;
  }

inline bsg_token_connection_t bsg_tq_receive_connection (bsg_token_pair_t BSG_VAR(token_array)[bsg_tiles_X][bsg_tiles_Y], int x, int y, int tile_x, int tile_y)
{
  bsg_token_connection_t conn;
  
  conn.local_ptr  = &(BSG_VAR_SEL(token_array)[x][y]);
  conn.remote_ptr = &(bsg_remote_ptr(x,y,token_array)[tile_x][tile_y].receive);

  return conn;
}

// wait for at least depth address sets to be available to sender
inline int bsg_tq_sender_confirm(bsg_token_connection_t conn, int max_els, int depth)
{
  int i = (conn.local_ptr)->send;
  int tmp =  - max_els + depth + i;
  // wait while number of available elements
  //  bsg_wait_while((depth + i - bsg_volatile_access((conn.local_ptr)->receive)) > max_els);

	int cond = 1;
	while (cond) {
		  pthread_mutex_lock(&bsg_simul_token_mutex);
		  cond = (conn.local_ptr)->receive < tmp;
		  pthread_mutex_unlock(&bsg_simul_token_mutex);
	}

  return i;
}

// actually do the transfer; assumes that you have confirmed first
//

inline int bsg_tq_sender_xfer(bsg_token_connection_t conn, int max_els, int depth)
{
  int   i = (conn.local_ptr)->send + depth;

  // local version
  (conn.local_ptr)->send = i;

  // remote version
  pthread_mutex_lock(&bsg_simul_token_mutex);
  *(conn.remote_ptr) = i;
  pthread_mutex_unlock(&bsg_simul_token_mutex);

  //printf("s %d\n", i);
  
  return i;
}

// wait for at least depth address sets to be available to receiver
inline int bsg_tq_receiver_confirm(bsg_token_connection_t conn, int depth)
{
  int i = (conn.local_ptr)->receive;

  int tmp = depth+i;

	int cond = 1;
	while (cond) {
		  pthread_mutex_lock(&bsg_simul_token_mutex);
		  cond = (conn.local_ptr)->send < tmp;
		  pthread_mutex_unlock(&bsg_simul_token_mutex);
	}


  return i;
}

// return the addresses; assumes you have confirmed first

inline void bsg_tq_receiver_release(bsg_token_connection_t conn, int depth)
{
  int i = (conn.local_ptr)->receive+depth;

  // local version
  (conn.local_ptr)->receive=i;

  // remote version
  pthread_mutex_lock(&bsg_simul_token_mutex);
  *(conn.remote_ptr) = i;
  pthread_mutex_unlock(&bsg_simul_token_mutex);
  //printf("r %d\n", i);
}

#endif
