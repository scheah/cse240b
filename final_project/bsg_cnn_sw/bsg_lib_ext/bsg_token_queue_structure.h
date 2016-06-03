// bsg token queue structure
// Moved from the orinal file due to the compile conflict
#ifndef _BSG_TOKEN_QUEUE_H_STRUCT_
#define _BSG_TOKEN_QUEUE_H_STRUCT_

typedef struct bsg_token_pair
{
  int send;
  int receive;
} bsg_token_pair_t;

typedef struct bsg_token_connection
{
  bsg_token_pair_t *local_ptr;
  volatile int *remote_ptr;
} bsg_token_connection_t;

#endif
