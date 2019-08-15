CFLAGS = -Wall -std=c++11
LDFLAGS = -lm -lrt -pthread -lssmem -ltcmalloc
IFLAGS = -I../

ifeq ($(VERSION),DEBUG) 
CFLAGS += -O0 -g -DDEBUG
else
CFLAGS += -O3
endif

UNAME := $(shell uname -n)

all: libssmem.a

ssmem.o: ./ssmem.c 
	g++ $(VER_FLAGS) -c ./ssmem.c $(CFLAGS) $(IFLAGS)

libssmem.a: ssmem.o ssmem.h
	@echo Archive name = libssmem.a
	ar -r libssmem.a ssmem.o
	rm -f *.o	

clean:
	rm -f *.o *.a ssmem_test
