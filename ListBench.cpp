#include "BenchUtils.h"

__thread ssmem_allocator_t *volatileAlloc;

#include "LinkFreeList.h"
#include "SOFTList.h"

template<>
void specificInit<SOFTList<intptr_t>>(int id){
    volatileAlloc = (ssmem_allocator_t *)malloc(sizeof(ssmem_allocator_t));
    ssmem_alloc_init(volatileAlloc, SSMEM_DEFAULT_MEM_SIZE, id);
}

template<class SET>
void specificInit(int id)
{
    return;
}

int main(int argc, char **argv)
{
    if (!parseArgs(argc, argv))
    {
        return 0;
    }
        switch(TEST_NUM){
            case 1:
                file.open(ALG_NAME + "-READS-" + to_string(RO_RATIO) + "-KEY_RANGE-" + to_string(KEY_RANGE) + ".txt", ofstream::app);
                if(ITERATION == 1)
                    file << "Threads Num: " << NUM_THREADS << endl;
                break;
            case 2:
                file.open(ALG_NAME + "-READS-" + to_string(RO_RATIO) + "-THREADS-" + to_string(NUM_THREADS) + ".txt", ofstream::app);
                if(ITERATION == 1)
                    file << "Key Range: " << KEY_RANGE << endl;
                break;
            case 3:
                file.open(ALG_NAME + "-KEY_RANGE-" + to_string(KEY_RANGE) + "-THREADS-" + to_string(NUM_THREADS) + ".txt", ofstream::app);
                if(ITERATION == 1)
                    file << "Reads: " << RO_RATIO << endl;
                break;
        }

    if (!ALG_NAME.compare("LinkFreeList"))
    {
            runBench<LinkFreeList<intptr_t>>();
    }
    else if (!ALG_NAME.compare("SOFTList"))
    {
            runBench<SOFTList<intptr_t>>();
    }
    else
    {
        cout << "Algorithm not found." << endl;
        cout << ALG_NAME << endl;
    }

        file.close();
    return 0;
}
