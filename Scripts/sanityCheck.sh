#!/bin/bash

for keyRange in 256 1024 
do
	make -C ../ BUCKET_NUM=$keyRange
  	for algo in "SOFTList"
	do
		for numberOfThreads in 1 2 4 8 16 32 64
		do	
			for i in {1..5}
			do
				../exe -a $algo -p $numberOfThreads -c -M $keyRange -I $i
			done
		done
	done
done
