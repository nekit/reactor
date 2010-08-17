#ifndef REACTOR_STRUCTURES_H
#define REACTOR_STRUCTURES_H

#include <stdint.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <semaphore.h>

#define DATA_QUEUE_SIZE 32
#define EPOLL_TIMEOUT 100

#define DEFAULT_PORT 1050
#define DEFAULT_IP "192.168.16.152"
#define DEFAULT_MAX_USERS 100000
#define DEFAULT_LISTN_BACKLOG 30000
#define DEFAULT_WORKER_AMOUNT 8
#define DEFAULT_REACTOR_MODE R_REACTOR_SERVER
#define DEFAULT_LOG_LEVEL "INFO"
#define DEFAULT_FILENAME "log.txt"
#define DEFAULT_FREQ 10
#define IP_ADDR_SIZE 20
#define PACKET_SIZE sizeof ( uint32_t )
typedef char packet_t[ PACKET_SIZE ];

typedef struct data_queue_s {

  packet_t pack[ DATA_QUEUE_SIZE ];
  int head;
  int tail;
  sem_t used;
  sem_t empty;
  
} data_queue_t;

typedef struct event_queue_s {

  struct epoll_event * ev;
  int head;
  int tail;
  int cap;
  sem_t used;
  sem_t empty;
  pthread_mutex_t read_mutex;
  pthread_mutex_t write_mutex;
  
} event_queue_t;

typedef enum {

  ST_ACCEPT,
  ST_DATA,
  ST_NOT_ACTIVE,
  ST_LAST,
  
} sock_type_t;

typedef struct inq_s {

  __uint32_t flags;
  pthread_mutex_t mutex;
  
} inq_t;

typedef struct __attribute__ ((__packed__)) data_wrap_s {

  __uint32_t idx;
  __uint32_t key;
  
} data_wrap_t;

typedef union __attribute__ ((__transparent_union__)) udata_s {

  data_wrap_t data;
  __uint64_t u64;
  
} udata_t;

// TODO separate sock_desk
typedef struct sock_desk_s {

  int sock;
  int sock_dup;
  int timeout; // in milliseconds
  uint32_t send_idx;
  sock_type_t type;
  int key;
  data_queue_t data_queue;
  packet_t send_pack;
  packet_t recv_pack;
  int send_ofs;
  int recv_ofs;
  pthread_mutex_t read_mutex;
  pthread_mutex_t write_mutex;
  pthread_mutex_t state_mutex;
  inq_t inq;
  
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

typedef struct event_heap_element_s {

  struct timespec time;
  struct epoll_event ev;  
  
} event_heap_element_t;

typedef struct event_heap_s {

  // capacity of the heap
  int cap;
  int size;
  event_heap_element_t * ev;
  pthread_mutex_t mutex;
  pthread_cond_t sleep_cond;
  pthread_mutex_t sleep_mutex;
  
} event_heap_t;

typedef struct statistic_s {

  long long val;
  pthread_mutex_t mutex;
  
} statistic_t;

typedef struct reactor_pool_s {

  int max_n;
  int epfd;
  sock_desk_t * sock_desk;  
  event_queue_t event_queue;
  event_heap_t event_heap;
  int_queue_t idx_queue;
  statistic_t statistic;
  
} reactor_pool_t;

typedef struct thread_pool_s {

  int n;
  pthread_t * worker;
  reactor_pool_t * rct_pool_p;
  int (*handle_event) ( struct epoll_event * ev, reactor_pool_t * rp_p );
  
} thread_pool_t;

typedef struct reactor_s {

  int cn;
  int max_n;
  int workers;
  thread_pool_t thread_pool;
  reactor_pool_t pool;
  
} reactor_t;

typedef enum {

  R_REACTOR_CLIENT,
  R_REACTOR_SERVER,
  R_LAST,
  
} reactor_mode_t;

typedef struct run_mode_s {

  int port;
  char ip_addr[ IP_ADDR_SIZE ];
  int max_users;
  int listn_backlog;
  int workers;
  reactor_mode_t mode;
  int n;
  int freq; // tps for one client
  char file[ 30 ];
  
} run_mode_t;

#endif /* End of REACTOR_STRUCTURES_H */
