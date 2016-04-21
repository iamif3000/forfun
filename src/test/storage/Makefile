objects = volume_manager_test.o volume_manager.o volume.o os_file.o base_string.o atomic.o function.o align.o error_manager.o thread.o 
LIBS =
STD = c99
CC = gcc -g -std=$(STD)
#VPATH = ../common/:../../port/

volume_manager_test : $(objects)
	$(CC) -o $@ $(LIBS) $(objects)
	
os_file_test.o : os_file_test.c
	$(CC) -c os_file_test.c
volume_manager.o : ../../storage/volume_manager.h ../../storage/volume_manager.c
	$(CC) -c ../../storage/volume_manager.c
volume.o : ../../storage/volume.h ../../storage/volume.c
	$(CC) -c ../../storage/volume.c
os_file.o : ../../port/os_file.h ../../port/os_file.c
	$(CC) -c ../../port/os_file.c
base_string.o : ../../base/base_string.h ../../base/base_string.c
	$(CC) -c ../../base/base_string.c
atomic.o : ../../port/atomic.h ../../port/atomic.c
	$(CC) -c ../../port/atomic.c
function.o : ../../common/function.h ../../common/function.c
	$(CC) -c ../../common/function.c
align.o : ../../common/align.h ../../common/align.c
	$(CC) -c ../../common/align.c
error_manager.o : ../../common/error_manager.c ../../common/error_manager.h
	$(CC) -c ../../common/error_manager.c
thread.o : ../../port/thread.c ../../port/thread.h
	$(CC) -c ../../port/thread.c
	
.PHONY : clean
clean : 
	rm -rf volume_manager_test $(objects)
