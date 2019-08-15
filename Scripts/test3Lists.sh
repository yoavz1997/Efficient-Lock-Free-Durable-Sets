#!/bin/bash

make -C ../ clean
make -C ../ list
for keyRange in 256 1024
do
	for numberOfThreads in 64
	do
   	for algo in "LinkFreeList" "SOFTList"
		do
		rm -f $algo-KEY_RANGE-$keyRange-THREADS-$numberOfThreads.txt
      for lookup in 0 5 10 20 30 40 50 60 70 80 90 95 100
			do	
				for i in {1..10}
				do
					../list -a $algo -p $numberOfThreads -R $lookup -M $keyRange -I $i -d 5 -t 3
				done
			done
		done
	done
 
  rm -rf t3/List-$keyRange/
  mkdir -p t3/List-$keyRange
  mv *.txt t3/List-$keyRange/
  python3 graph.py -T 3 -D ./t3/List-$keyRange
done
