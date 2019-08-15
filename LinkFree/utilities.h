#ifndef LINK_FREE_UTILS_H_
#define LINK_FREE_UTILS_H_

#include "common.h"

#define HIGH_CHAR_MASK 0x80
#define LOW_MASK 0x1    

namespace linkFreeUtils
{

static inline void flipV1(std::atomic<uchar> *metaData)
{
    uchar oldMetaData = metaData->load();
    uchar newV1 = (oldMetaData & HIGH_CHAR_MASK) ^ HIGH_CHAR_MASK;
    uchar newMetaData = newV1 + (oldMetaData & (~HIGH_CHAR_MASK));
    metaData->store(newMetaData, std::memory_order_release);
}

static inline bool isValid(uchar oldMetaData)
{
    uchar v1 = (oldMetaData & HIGH_CHAR_MASK) >> 7;
    uchar v2 = oldMetaData & LOW_MASK;
    return v1 == v2;
}

static inline void makeValid(std::atomic<uchar> *metaData)
{
    uchar oldMetaData = metaData->load();
    if (isValid(oldMetaData))
        return;
    uchar v1 = (oldMetaData & HIGH_CHAR_MASK) >> 7;
    uchar newMetaData = v1 + (oldMetaData & (~LOW_MASK));
    metaData->store(newMetaData, std::memory_order_release);
}

static inline bool isMarked(void *ptr)
{
    auto ptrLong = (uintptr_t)(ptr);
    return ((ptrLong & 1) == 1);
}

template <class Node>
static inline Node *getRef(Node *ptr)
{
    auto ptrLong = (uintptr_t)(ptr);
    ptrLong &= ~1;
    return (Node *)(ptrLong);
}

template <class Node>
static inline Node *mark(Node *ptr)
{
    auto ptrLong = (uintptr_t)(ptr);
    ptrLong |= 1;
    return (Node *)(ptrLong);
}

} // namespace linkFreeUtils

#endif