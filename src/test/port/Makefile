objects = resource_queue_test.o resource_queue.o base_string.o atomic.o align.o
LIBS = -lpthread
STD = c99
CC = cc -std=$(STD) -g
#VPATH = ../common/:../../port/

resource_queue_test : $(objects)
	$(CC) -o $@ $(LIBS) $(objects)
	
resource_queue_test.o : resource_queue_test.c ../common/test_common.h
	$(CC) -c resource_queue_test.c
resource_queue.o : ../../port/resource_queue.h ../../port/resource_queue.c
	$(CC) -c ../../port/resource_queue.c
base_string.o : ../../base/base_string.c
	$(CC) -c ../../base/base_string.c
atomic.o : ../../port/atomic.h ../../port/atomic.c
	$(CC) -c ../../port/atomic.c
align.o : ../../common/align.c ../../common/align.h
	$(CC) -c ../../common/align.c
	
.PHONY : clean
clean : 
	-rm resource_queue_test $(objects)
