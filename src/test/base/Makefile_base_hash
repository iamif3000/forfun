objects = base_hash_test.o base_hash.o atomic.o align.o
LIBS = -lpthread
STD = c99
CC = cc -std=$(STD) -g

base_hash_test : $(objects)
	$(CC) -o $@ $(LIBS) $(objects)

base_hash_test.o : base_hash_test.c ../common/test_common.h
	$(CC) -c base_hash_test.c
base_hash.o : ../../base/base_hash.c
	$(CC) -c ../../base/base_hash.c
atomic.o : ../../port/atomic.c ../../port/atomic.h
	$(CC) -c ../../port/atomic.c
align.o : ../../common/align.c ../../common/align.h
	$(CC) -c ../../common/align.c
	
.PHONY : clean
clean : 
	-rm base_hash_test $(objects)
