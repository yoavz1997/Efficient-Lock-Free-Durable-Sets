#include "BenchUtils.h"

__thread unsigned int randSeed;
static uchar get_random_level()
{
    int i;
    uchar level = 1;

    for (i = 0; i < MAX_LEVEL - 1; i++)
    {
        if ((rand_r_32(&randSeed) & 0xFF) < 128)
            level++;
        else
            break;
    }
    return level;
}

#include "LinkFreeSkipList.h"
#include "SOFTSkipList.h"

template<class SET>
void specificInit(int id)
{
    randSeed = id + 2;
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

    if (!ALG_NAME.compare("LinkFreeSkipList"))
    {
            runBench<LinkFreeSkipList<intptr_t>>();
    }
    else if (!ALG_NAME.compare("SOFTSkipList"))
    {
            runBench<SOFTSkipList<intptr_t>>();
    }
    else
    {
        cout << "Algorithm not found." << endl;
        cout << ALG_NAME << endl;
    }

        file.close();
    return 0;
}
