#ifndef LINK_FREE_LIST_H_
#define LINK_FREE_LIST_H_

#include <vector>
#include <climits>
#include "utilities.h"
#include <atomic>
#include <cassert>
#include "ssmem.h"
#include <stdint.h>
#include <stdlib.h>

//#define LOAD_MO std::memory_order_relaxed
#define LOAD_MO

template <class T>
class LinkFreeList
{
public:
    class Node
    {
    public:
        std::atomic<uchar> metaData;
        std::atomic<bool> insertFlag;
        std::atomic<bool> deleteFlag;
        intptr_t key;
        T value;
        std::atomic<Node *> next;

        Node() : metaData(0), next(nullptr), insertFlag(false), deleteFlag(false) {}

        Node(intptr_t key, T value, Node *next) : key(key), value(value), next(next), insertFlag(false), deleteFlag(false) {}

        bool isMarked()
        {
            return linkFreeUtils::isMarked(next.load());
        }
    } __attribute__((aligned((32))));

private:
    Node *allocNode(intptr_t key, T value, Node *next)
    {
        Node *newNode = static_cast<Node *>(ssmem_alloc(alloc, sizeof(Node)));
        linkFreeUtils::flipV1(&newNode->metaData);
        std::atomic_thread_fence(std::memory_order_release);
        newNode->insertFlag.store(false, std::memory_order_relaxed);
        newNode->deleteFlag.store(false, std::memory_order_relaxed);
        newNode->key = key;
        newNode->value = value;
        newNode->next.store(next, std::memory_order_relaxed);
        return newNode;
    }

    void FLUSH_DELETE(Node *n)
    {
        if (LIKELY(n->deleteFlag.load()))
            return;
        FLUSH(n);
        n->deleteFlag.store(true, std::memory_order_release);
    }

    void FLUSH_INSERT(Node *n)
    {
        if (LIKELY(n->insertFlag.load()))
            return;
        FLUSH(n);
        n->insertFlag.store(true, std::memory_order_release);
    }

    //trim curr
    bool trim(Node *pred, Node *curr)
    {
        FLUSH_DELETE(curr);
        Node *succ = linkFreeUtils::getRef<Node>(curr->next.load());
        bool result = pred->next.compare_exchange_strong(curr, succ);
        if (LIKELY(result))
            ssmem_free(alloc, curr);
        return result;
    }

    Node *find(intptr_t key, Node **predPtr)
    {
        Node *prev = head, *curr = head->next.load();

        while (true)
        {
            // curr is not marked
            if (LIKELY(!linkFreeUtils::isMarked(curr->next)))
            {
                if (UNLIKELY(curr->key >= key))
                    break;
                prev = curr;
            }
            else
            {
                trim(prev, curr);
            }
            curr = linkFreeUtils::getRef<Node>(curr->next);
        }
        *predPtr = prev;
        return curr;
    }

public:
    LinkFreeList()
    {
        Node *max = new Node(INT_MAX, 0, nullptr);
        Node *min = new Node(INT_MIN, 0, max);
        head = min;
    }

    bool insert(intptr_t key, T value, int tid)
    {
        do
        {
            Node *pred = nullptr;
            Node *curr = find(key, &pred);

            if (curr->key == key)
            {
                linkFreeUtils::makeValid(&curr->metaData);
                FLUSH_INSERT(curr);
                return false;
            }

            Node *newNode = allocNode(key, value, curr);

            if (pred->next.compare_exchange_strong(curr, newNode))
            {
                linkFreeUtils::makeValid(&newNode->metaData);
                FLUSH_INSERT(newNode);
                return true;
            }

            // freeing newNode in a deleted and valid state
            newNode->next.store(linkFreeUtils::mark<Node>(nullptr));
            linkFreeUtils::makeValid(&newNode->metaData);
            ssmem_free(alloc, newNode);
        } while (true);
    }

    bool remove(intptr_t key, int tid)
    {
        bool result = false;
        Node *pred, *curr, *succ, *markedSucc;
        do
        {
            curr = find(key, &pred);
            if (curr->key != key)
                return false;

            succ = linkFreeUtils::getRef<Node>(curr->next);
            markedSucc = linkFreeUtils::mark<Node>(succ);
            linkFreeUtils::makeValid(&curr->metaData);
            result = curr->next.compare_exchange_strong(succ, markedSucc);
        } while (!result);
        trim(pred, curr);
        return true;
    }

    bool contains(intptr_t key, int tid)
    {
        Node *curr = head->next.load();
        bool marked = false;
        //wait free find
        while (curr->key < key)
        {
            curr = linkFreeUtils::getRef<Node>(curr->next.load());
        }
        if (curr->key != key)
            return false;

        marked = linkFreeUtils::isMarked(curr->next.load());
        if (marked)
        {
            //if the node is marked, it must be valid
            FLUSH_DELETE(curr);
            return false;
        }
        linkFreeUtils::makeValid(&curr->metaData);
        FLUSH_INSERT(curr);
        return true;
    }

    void quickInsert(Node *newNode)
    {
        intptr_t key = newNode->key;
        Node *pred = nullptr, *curr = nullptr, *succ = nullptr;
    retry:
        pred = const_cast<Node *>(head);
        curr = linkFreeUtils::getRef<Node>(pred->next.load());
        //inline the find function
        while (true)
        {
            succ = curr->next.load();
            //trimming
            while (linkFreeUtils::isMarked(succ))
            {
                assert(false);
            }
            //continue searching
            if (curr->key < key)
            {
                pred = curr;
                curr = succ;
            }
            //found the same
            else if (curr->key == key)
            {
                assert(false);
            }
            else
            {
                newNode->next.store(curr, std::memory_order_relaxed);
                if (!pred->next.compare_exchange_strong(curr, newNode))
                    goto retry;
                return;
            }
        }
    }

    void recover()
    {
        auto curr = alloc->mem_chunks;
        for (; curr != nullptr; curr = curr->next)
        {
            Node *currChunk = static_cast<Node *>(curr->obj);
            uint64_t numOfNodes = SSMEM_DEFAULT_MEM_SIZE / sizeof(Node);
            for (uint64_t i = 0; i < numOfNodes; i++)
            {
                Node *currNode = currChunk[i];
                // the node was never initialized, no need to free it or add it
                if (currNode->next.load() == nullptr && linkFreeUtils::isValid(currNode->metaData.load()))
                    continue;
                if (!linkFreeUtils::isValid(currNode->metaData.load()) || currNode->isMarked())
                {
                    currNode->next.store(linkFreeUtils::mark<Node>(nullptr));
                    linkFreeUtils::makeValid(&currNode->metaData);
                    ssmem_free(alloc, currNode);
                }
                else
                    quickInsert(currNode);
            }
        }
    }

private:
    Node *head;
};

#endif
