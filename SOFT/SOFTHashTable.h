#ifndef _SOFT_HASH_TABLE_H_
#define _SOFT_HASH_TABLE_H_

#include "SOFTList.h"
#include "utilities.h"
#include <cmath>

template <class T>
class SOFTHashTable
{
public:
    bool insert(int k, T item, int tid)
    {
        SOFTList<T> &bucket = getBucket(k);
        return bucket.insert(k, item, tid);
    }

    bool remove(int k, int tid)
    {
        SOFTList<T> &bucket = getBucket(k);
        return bucket.remove(k, tid);
    }

    bool contains(int k, int tid)
    {
        SOFTList<T> &bucket = getBucket(k);
        return bucket.contains(k, tid);
    }

  private:
    SOFTList<T> &getBucket(int k)
    {
        return table[std::abs(k % BUCKET_NUM)];
    }

    SOFTList<T> table[BUCKET_NUM];
};

#endif