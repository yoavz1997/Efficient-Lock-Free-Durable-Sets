#!/bin/bash

for numberOfThreads in 32
do
	for lookup in 90
	do
   	for algo in "LinkFreeHashTable" "SOFTHashTable"
		do
		rm -f $algo-READS-$lookup-THREADS-$numberOfThreads.txt
      for keyRange in 1024 16384 262144 4194304
			do
        make -C ../ clean
	      make -C ../ hash BUCKET_NUM=$keyRange	
				for i in {1..10}
				do
					../hash -a $algo -p $numberOfThreads -R $lookup -M $keyRange -I $i -d 5 -t 2
				done
			done
		done
	done
done
rm -rf t2/Hash/
mkdir -p t2/Hash
mv *.txt t2/Hash/
python3 graph.py -T 2 -D ./t2/Hash