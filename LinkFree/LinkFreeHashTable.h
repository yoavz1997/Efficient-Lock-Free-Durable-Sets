#ifndef LINK_FREE_HASH_TABLE_H_
#define LINK_FREE_HASH_TABLE_H_

#include "utilities.h"
#include "LinkFreeList.h"
#include <cmath>

template <class T>
class LinkFreeHashTable
{
  public:
    bool insert(int k, T item, int tid)
    {
        LinkFreeList<T> &bucket = getBucket(k);
        return bucket.insert(k, item, tid);
    }

    bool remove(int k, int tid)
    {
        LinkFreeList<T> &bucket = getBucket(k);
        return bucket.remove(k, tid);
    }

    bool contains(int k, int tid)
    {
        LinkFreeList<T> &bucket = getBucket(k);
        return bucket.contains(k, tid);
    }

    std::string myName(){
        return "Link Free Hash Table";
    }

  private:
    LinkFreeList<T>& getBucket(int k){
        return table[std::abs(k % BUCKET_NUM)];
    }

    LinkFreeList<T> table[BUCKET_NUM];
};

#endif