#!/bin/bash

for keyRange in 1048576
do
	make -C ../ clean
	make -C ../ hash BUCKET_NUM=$keyRange
	for numberOfThreads in 32
	do
   	for algo in "LinkFreeHashTable" "SOFTHashTable"
		do
		rm -f $algo-KEY_RANGE-$keyRange-THREADS-$numberOfThreads.txt
      for lookup in 0 5 10 20 30 40 50 60 70 80 90 95 100
			do	
				for i in {1..10}
				do
					../hash -a $algo -p $numberOfThreads -R $lookup -M $keyRange -I $i -d 5 -t 3
				done
			done
		done
	done
done

rm -rf t3/Hash/
mkdir -p t3/Hash
mv *.txt t3/Hash/
python3 graph.py -T 3 -D ./t3/Hash
