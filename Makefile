CFLAGS = -std=c++11 -fpermissive -O3

LFLAGS = -L./include -pthread -lssmem
LINKFREE = ./LinkFree
SOFT = ./SOFT
IFLAGS = -I./include -I$(LINKFREE) -I$(SOFT) -I. 
all: list hash sl

list: ListBench.cpp SOFT/SOFTList.h LinkFree/LinkFreeList.h include/BenchUtils.h
	make -C ./include all
	g++ ListBench.cpp $(CFLAGS) $(IFLAGS) $(LFLAGS) -o list

hash: HashBench.cpp SOFT/SOFTHashTable.h LinkFree/LinkFreeHashTable.h include/BenchUtils.h
	make -C ./include all
	g++ HashBench.cpp $(CFLAGS) $(IFLAGS) $(LFLAGS) -DBUCKET_NUM=$(BUCKET_NUM) -o hash

sl: SLBench.cpp SOFT/SOFTSkipList.h LinkFree/LinkFreeSkipList.h include/BenchUtils.h
	make -C ./include all
	g++ SLBench.cpp $(CFLAGS) $(IFLAGS) $(LFLAGS) -o sl

clean:
	rm -f list hash sl
	rm -f ./include/libssmem.a
