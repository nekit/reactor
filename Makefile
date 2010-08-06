target_name := reactor
target_list := reactor.o log.o data_queue_op.o event_queue_op.o thread_pool_op.o parse_args.o run_reactor.o socket_operations.o int_queue_op.o reactor_pool_op.o handle_event.o event_heap_op.o client_sheduler.o client_handle_event.o

CFLAGS += -O2 -Wall
LDFLAGS += -lpthread

all: $(target_name)

$(target_name): $(target_list)
	$(CC) $^ -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) -c -MD $(CFLAGS) $<

include $(wildcard *.d)

clean:
	rm *.o *.d $(target_name)