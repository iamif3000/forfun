objects = heap_test.o heap.o atomic.o error_manager.o thread.o
LIBS = -lpthread
STD = c99
CC = cc -std=$(STD) -g

heap_test : $(objects)
	$(CC) -o $@ $(LIBS) $(objects)

heap_test.o : heap_test.c ../common/test_common.h
	$(CC) -c heap_test.c
heap.o : ../../base/heap.c
	$(CC) -c ../../base/heap.c
atomic.o : ../../port/atomic.c ../../port/atomic.h
	$(CC) -c ../../port/atomic.c
error_manager.o : ../../common/error_manager.c ../../common/error_manager.h
	$(CC) -c ../../common/error_manager.c
thread.o : ../../port/thread.c ../../port/thread.h
	$(CC) -c ../../port/thread.c
	
.PHONY : clean
clean : 
	rm -rf heap_test $(objects)
