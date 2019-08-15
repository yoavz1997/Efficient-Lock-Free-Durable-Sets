#!/bin/bash

for keyRange in 1048576
do
  make -C ../ clean
	make -C ../ hash BUCKET_NUM=$keyRange
	for lookup in 90
	do
   	for algo in "LinkFreeHashTable" "SOFTHashTable"
		do
		rm -f $algo-READS-$lookup-KEY_RANGE-$keyRange.txt
			for numberOfThreads in 1 2 4 8 16 32 64
			do	
				for i in {1..10}
				do
					../hash -a $algo -p $numberOfThreads -R $lookup -M $keyRange -I $i -d 5 -t 1
				done
			done
		done
	done
done
rm -rf t1/Hash/
mkdir -p t1/Hash
mv *.txt t1/Hash/
python3 graph.py -T 1 -D ./t1/Hash