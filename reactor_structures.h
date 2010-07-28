#ifndef REACTOR_STRUCTURES_H
#define REACTOR_STRUCTURES_H

#include <stdint.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <semaphore.h>

#define DATA_QUEUE_SIZE 100000
#define EVENT_QUEUE_SIZE 1000000
#define EPOLL_TIMEOUT 100

#define DEFAULT_PORT 2007
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_MAX_USERS 1000
#define DEFAULT_LISTN_BACKLOG 1000
#define DEFAULT_WORKER_AMOUNT 4
#define IP_ADDR_SIZE 20
#define PACKET_SIZE sizeof ( uint32_t )
typedef char packet_t[ PACKET_SIZE ];

typedef struct data_queue_s {

  packet_t pack[ DATA_QUEUE_SIZE ];
  int head;
  int tail;
  int size;
  pthread_mutex_t mutex;
  
} data_queue_t;

typedef struct eventq_s {

  struct epoll_event ev;
  struct eventq_s * prev; 
  
} eventq_t;

typedef struct event_queue_s {

  eventq_t * head;
  eventq_t * tail;
  sem_t used;
  pthread_mutex_t read_mutex;
  pthread_mutex_t write_mutex;  
} event_queue_t;

typedef enum {

  ST_ACCEPT,
  ST_DATA,
  ST_NOT_ACTIVE,
  ST_LAST,
  
} sock_type_t;

typedef struct sock_desk_s {

  int sock;
  sock_type_t type;
  int idx;
  data_queue_t data_queue;
  packet_t send_pack;
  packet_t recv_pack;
  int send_ofs;
  int recv_ofs;

  // not using???
  pthread_mutex_t state_mutex;
  
  
} sock_desk_t;

typedef struct int_queue_s {

  int cap;
  int * val;
  int head;
  int tail;
  int size;
  pthread_mutex_t write_mutex;
  pthread_mutex_t sz_mutex;
    
} int_queue_t;

typedef struct reactor_pool_s {

  int max_n;
  int epfd;
  sock_desk_t * sock_desk;  
  event_queue_t event_queue;
  int_queue_t idx_queue;  
  
} reactor_pool_t;

typedef struct thread_pool_s {

  int n;
  pthread_t * worker;
  reactor_pool_t * rct_pool_p;
  int (*handle_event) ( struct epoll_event * ev, reactor_pool_t * rp_p );
  
} thread_pool_t;

typedef struct reactor_s {

  int max_n;
  int workers;
  thread_pool_t thread_pool;
  reactor_pool_t pool;
  
} reactor_t;

typedef struct run_mode_s {

  int port;
  char ip_addr[ IP_ADDR_SIZE ];
  int max_users;
  int listn_backlog;
  int workers;
  
} run_mode_t;


#endif /* End of REACTOR_STRUCTURES_H */
