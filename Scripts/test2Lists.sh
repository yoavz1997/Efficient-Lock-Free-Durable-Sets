#!/bin/bash

make -C ../ clean
make -C ../ list
for numberOfThreads in 64
do
  	for lookup in 90
	do
   	for algo in "LinkFreeList" "SOFTList"
		do
		rm -f $algo-READS-$lookup-THREADS-$numberOfThreads.txt
      for keyRange in 16 64 256 1024 4096 16384
			do	
				for i in {1..10}
				do
					../list -a $algo -p $numberOfThreads -R $lookup -M $keyRange -I $i -d 5 -t 2
				done
			done
		done
	done
 
  rm -rf t2/List/
  mkdir -p t2/List
  mv *.txt t2/List/
  python3 graph.py -T 2 -D ./t2/List
done
