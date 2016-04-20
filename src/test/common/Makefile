objects = error_manager_test.o error_manager.o base_string.o atomic.o thread.o align.o
LIBS = -lpthread
STD = c99
CC = cc -std=$(STD) -g

error_manager_test : $(objects)
	$(CC) -o $@ $(LIBS) $(objects)

error_manager_test.o : error_manager_test.c
	$(CC) -c error_manager_test.c
error_manager.o : ../../common/error_manager.h ../../common/error_manager.c
	$(CC) -c ../../common/error_manager.c
align.o : ../../common/align.h ../../common/align.c
	$(CC) -c ../../common/align.c
base_string.o : ../../base/base_string.c
	$(CC) -c ../../base/base_string.c
atomic.o : ../../port/atomic.c ../../port/atomic.h
	$(CC) -c ../../port/atomic.c
thread.o : ../../port/thread.c ../../port/thread.h
	$(CC) -c ../../port/thread.c
	
.PHONY : clean
clean : 
	rm -rf error_manager_test $(objects)
