#!/bin/bash

make -C ../ clean
make -C ../ sl
for keyRange in 1048576
do
	for numberOfThreads in 64
	do
   	for algo in "LinkFreeSkipList" "SOFTSkipList"
		do
		rm -f $algo-KEY_RANGE-$keyRange-THREADS-$numberOfThreads.txt
      for lookup in 50 60 70 80 90 95 100
			do	
				for i in {1..10}
				do
					../sl -a $algo -p $numberOfThreads -R $lookup -M $keyRange -I $i -d 5 -t 3
				done
			done
		done
	done
done

rm -rf t3/SL/
mkdir -p t3/SL
mv *.txt t3/SL/
python3 graph.py -T 3 -D ./t3/SL
