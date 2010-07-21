#include "socket_operations.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory.h>
#include <fcntl.h>
#include "log.h"

#define SOCKET_OP_LOG_LEVEL 0

int set_nonblock ( int sock ) {

  if ( -1 == fcntl ( sock, F_SETFL, O_NONBLOCK ) ) {

    ERROR_MSG ( "set sock %d nonblocking failed\n", sock );
   return -1;
  }

  return 0;
}

int make_reusable ( int sock ) {

  // call before bind()

   DEBUG_MSG ( "making socket reusable \n");
  
  int reuse_val = 1;
  if ( setsockopt ( sock, SOL_SOCKET, SO_REUSEADDR, &reuse_val, sizeof ( reuse_val ) ) == -1 ) {

    DEBUG_MSG ( "reuse problem\n");
    return 1;
  }
  
  return 0;
}

int bind_socket ( uint32_t listn_ip, uint16_t port, int listn_backlog ) {

  DEBUG_MSG ( "binding socket\n");

  int sock = socket ( PF_INET, SOCK_STREAM, 0 );
  if ( -1 == sock ) {    

    ERROR_MSG ( "socket problem\n");
    return -1;
  }

  if ( 0 != make_reusable ( sock ) ){
    
    ERROR_MSG ( "make reuse problem\n");
    return -1;
  }
  
  struct sockaddr_in serv_addr;
  memset ( &serv_addr, 0, sizeof ( struct sockaddr_in ) );
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = port;
  serv_addr.sin_addr.s_addr = listn_ip;

  if ( -1 == bind ( sock, (struct sockaddr *) &serv_addr, sizeof( struct sockaddr_in) ) ) {

    ERROR_MSG ( "bind problem\n");
    return -1;
  }

  if ( -1 == listen ( sock, listn_backlog ) ) {
    
    ERROR_MSG ( "listen problem\n");
    return -1;
  }

  DEBUG_MSG ( "bind success\n");

  return sock;
}

int connect_to_server ( uint32_t server_ip, uint16_t port ) {

  DEBUG_MSG ( "connecting to server\n");

  struct sockaddr_in serv_addr;
  int sock = socket ( PF_INET, SOCK_STREAM, 0 );
  
  if ( -1 == sock ) {    

    DEBUG_MSG ( "socket problem\n");
    return -1;
  }

  memset ( &serv_addr, 0, sizeof ( struct sockaddr_in ) );
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = port;
  serv_addr.sin_addr.s_addr = server_ip;

  if ( connect ( sock, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in) ) == -1 ) {

    DEBUG_MSG ( "connection to server failed\n");
    return -1;
  }   

  DEBUG_MSG ( "connected successfully\n");
  
  return sock;
}

int accept_wrap ( int sock ) {

  // ???
  DEBUG_MSG ( "accepting\n");

  static struct sockaddr_in clnt_addr;
  static socklen_t sock_len;
  int new_sock;
  new_sock = accept ( sock, (struct sockaddr *) &clnt_addr, &sock_len);

  return new_sock;
}
