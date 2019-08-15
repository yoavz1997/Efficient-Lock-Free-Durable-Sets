#!/bin/bash

make -C ../ clean
make -C ../ sl
for numberOfThreads in 64
do
	for lookup in 90
	do
   	for algo in "LinkFreeSkipList" "SOFTSkipList"
		do
		rm -f $algo-READS-$lookup-THREADS-$numberOfThreads.txt
      for keyRange in 1024 16384 262144 4194304
			do	
				for i in {1..10}
				do
					../sl -a $algo -p $numberOfThreads -R $lookup -M $keyRange -I $i -d 5 -t 2
				done
			done
		done
	done
done
rm -rf t2/SL/
mkdir -p t2/SL
mv *.txt t2/SL/
python3 graph.py -T 2 -D ./t2/SL
