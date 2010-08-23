target_name := reactor
target_list := reactor.o log.o data_queue_op.o event_queue_op.o parse_args.o socket_operations.o int_queue_op.o run_server.o reactor_pool_op.o reactor_core_op.o server_handle_event.o thread_pool_op.o run_client.o event_heap_op.o client_scheduler.o thread_statistics.o client_handle_event.o

CFLAGS += -g3 -Wall 
LDFLAGS += -lpthread 

all: $(target_name)

$(target_name): $(target_list)
	$(CC) $^ -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) -c -MD $(CFLAGS) $<

include $(wildcard *.d)

clean:
	rm *.o *.d $(target_name)
