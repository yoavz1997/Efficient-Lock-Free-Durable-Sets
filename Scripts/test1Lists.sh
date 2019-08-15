#!/bin/bash

	make -C ../ clean
	make -C ../ list
for keyRange in 256 1024
do

	for lookup in 90
	do
   	for algo in "LinkFreeList" "SOFTList"
		do
		rm -f $algo-READS-$lookup-KEY_RANGE-$keyRange.txt
			for numberOfThreads in 1 2 4 8 16 32 64
			do	
				for i in {1..10}
				do
					../list -a $algo -p $numberOfThreads -R $lookup -M $keyRange -I $i -d 5 -t 1
				done
			done
		done
	done
 
  rm -rf t1/List-$keyRange/
  mkdir -p t1/List-$keyRange
  mv *.txt t1/List-$keyRange/
  python3 graph.py -T 1 -D ./t1/List-$keyRange
done
