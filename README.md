# Durable-Set

* [Artifact Overview](#artifact-overview)
  * [Directory Layout](#directory-layout)
* [Getting Started](#getting-started)
* [Step-by-Step Guide](#step-by-step-guide)
  * [Throughput Measurements](#throughput-measurements)
  * [Manual Testing](#manual-testing)
  * [Customizing Tests](#customizing-tests)
* [Code Overview](#code-overview)
  * [Link-Free List](#link-free-list)
  * [SOFT List](#soft-list)

## Artifact Overview 

We provide a zip file containing:
* The code for both implementations, Link-Free and SOFT. There is a code for a linked list, a fixed sized hash table, and a skip list.
  The code of the link-free and SOFT lists matches Section 3 and 4 respectively.
* The memory manager we used in order to allocate durable areas (Section 5).
* A number of scripts for reproducing the figures depicted in Section 6.

### Directory Layout
The main directory `Durable-Set` contains the following:
* 'ListBench.cpp, HashBench.cpp and SLBench.cpp' which are three main files to run tests for linked lists, hash tables and
  skip lists respectively.
* `Makefile` to build the three different main files.
* `include/` is a directory holding some header files used by all the different implementations such as `common.h` and `barrier.h`.
  Additionally, it has the memory manager in `ssmem.c` and its makefile.
* `Scripts/` is a directory with all the scripts needed to run the tests in the paper and to create the graphs as well.
* `LinkFree/` is where the three implementations of the link free sets are. There is also a utility header with the main
  auxiliary functions.
* `SOFT/` is the directory where the implementations of the SOFT list, hash table, and skip list are.
  In addition, it has the PNode (Section 4.1) and the Volatile Node (Section 4.2) implementations.

## Getting Started
Our code requires some basic dependencies:
* The compiler we used is `g++` version 8.3.0 and with C++11 standard.
  There is no inherent dependency on the g++ version but it has to support C++11 atomics.
* In order to compile the code we used `GNU Make` version 4.1
* The scripts running the code are written in bash. We used version 4.3.48
* The graphs are drawn using `python 3` (we used version 3.7.3) and it uses the package `matplotlib` (requires python 3.6).
  If the package is not installed go to the following command `pip install matplotlib` or go to this [website](https://matplotlib.org/3.1.0/users/installing.html).

## Step-by-Step Guide
### Throughput Measurements
The artifact addresses all claims from the evaluation section (Section 6) of the paper.

We tested our data structures in three different experiments.

The first one where the key range and the percentage of the reads are fixed, tests the scalability of our data structures (with up to 64 threads).
As can be seen in the paper, the lists were tested with two different key ranges to get a better understanding of the behavior of short v.s. long lists.

In order to run this test, run `Scripts/runTest1.sh`.
This script runs and creates the three graphs shown in Figure 1 of the paper (in a dedicated directory), and runs the same test for the link-free and SOFT skip-lists.

The number of threads in the next two tests is fixed to 64 threads or 32 for hash tables. If your machine has fewer cores, you may want to consider reducing that number in order to get clearer results (without a lot of context-switch overhead).
To do that see this [part](#customizing-tests).

In the second test the number of threads is fixed while the key range is the variable.
Similar to the first test, in order to run this one, use the script `Scripts/runTest2.sh`.

The third and the final test case has a varied percentage of reads (contains) operations to emulate different workloads.
The script to run this test is `Scripts/runTest3.sh`.

You can run all of the tests one after the other, by simply using the script `Scripts/runAll.sh`.
Note that, each iteration in each test takes 5 seconds (and each configuration is run 10 times), meaning that running all the different tests would take a few hours.

All the graphs are stored under the corresponding directory (`t1`, `t2` or `t3`) in a png format.

Each of these three experiments takes about 40 minutes to run.
If you wish to run a single execution, see this [part](#manual-testing), or consider running only a partial test.
The partial tests measure the performance of a single data structure, for instance `Scripts/t1Lists.h` runs the first test but only for lists and not hash tables or skip-list.
These partial tests take around 10 minutes to run.
To make them run faster you can change the time each iteration takes (it is 5 seconds in the default) or the number of iterations per configuration (10 iterations by default).
The how-to appears in the [customization part](#customizing-tests).

See the [customization part](#customizing-tests) for more possibilities of testing.

### Manual Testing
By using the provided `Makefile` all of the three executables can be compiled.
The list and skip-list executables are compiled by simply running `make list` or `make sl`.

The hash table executable in particular is compiled by executing `make hash BUCKET_NUM=...` where the following number is the number of buckets in the hash tables.

After compiling (let's say the list), you have the exe file.
First, you can run `list -h` to get more information about each command line parameter.
The parameters are:
* `-a` is the algorithm's name, e.g, "LinkFreeList", "SOFTHashTable" or "LinkFreeSkipList".
* `-p` specifies the number of threads to run.
* `-d` determines the duration of the run (in whole seconds).
* `-R` is the ratio of read operations (e.g, if it is 90 then 90% of the operations will be reads).
* `-M` is the size of the key range.
* `-I` and `-t` are format flags for the different tests.

### Customizing Tests
All the different tests are built up the same way.
There are three loops, one for each parameter (number of thread, key range size, and percentage of reads).
All you have to do in order to run a modified test is change the parameter list of one or more of the loops.
In order to decrease the number of iterations each configuration is run, change the number in the last loop, from 10 to the new number.
The duration of each iteration can be changed as well.
In the execution line, the number after the `-d` option signifies the number of seconds, so it can be decreased to make each test shorter.

For example, let's look at test 3 where we only tested 50 to 100 percents of reads.
In order to extend that test/only measure 0 to 50, we look at `Scripts/t3Lists.sh`, line 12, and add to the list all the other different percents to get `0 10 20 30 40 50 60 70 80 90 95 100`.
Then, all we have to do is run this script (or change the others' parameters as well, and run `Scripts/runTest3.sh`).
Note that, the scripts clean the results' directories before each run, so if you wish to run multiple versions, change the names of the created directories.

## Code Overview
### Link-Free List
In the directory `LinkFree/` there is all the code for the list, hash table, and skip list.
The file `LinkFree/LinkFreeList.h` matches Section 3 of the paper.
Listings 1 to 5 all appear in that file.
The functions dealing with the validity scheme and the marking system can be found in `LinkFree/utilities.h`.

As per the request of one of our reviewers we add the code for our skip-list, file `LinkFree/LinkFreeSkipList.h`, which applies the link-free technique.

### SOFT List
The code of SOFT list, matching Section 5 of the paper, can be found in `SOFT/SOFTList.h`.
Listings 6 and 7 of the PNode is in `SOFT/PNode.h` and Listing 8 of the Volatile Node is in `SOFT/VolatileNode.h`.
The auxiliary functions in Listing 9 and some others can be found in `SOFT/utilities.h`.
The main functions, Listings 9, 10 and 11 are in `SOFT/SOFTList.h`.

The code of our SOFT skip list is in the file named `SOFT/SOFTSkipList.h`.
We experiment with the idea of consolidating both nodes (PNode and the Volatile Node) into one smaller node.
The skip-list presented in this file has this new modification.
The algorithm remains as it were, but instead of first initializing a Volatile Node and then the PNode, they are initialized together.
The same flow as one would expect from a SOFT data structure remains.
The main change is saving memory by using a single node and not two.
