#ifndef LINK_FREE_SKIP_LIST_H_
#define LINK_FREE_SKIP_LIST_H_

#include <vector>
#include <climits>
#include "utilities.h"
#include <atomic>
#include <cassert>
#include "ssmem.h"
#include <stdint.h>
#include <stdlib.h>

template <class T>
class LinkFreeSkipList
{
public:
    class Node
    {
    public:
        std::atomic<uchar> metaData;
        std::atomic<bool> insertFlag;
        std::atomic<bool> deleteFlag;
        uchar topLevel;
        intptr_t key;
        T value;
        std::atomic<Node *> next[MAX_LEVEL];

        Node() : metaData(0), insertFlag(false), deleteFlag(false) {}

        Node(intptr_t key, T value, uchar topLevel) : key(key), value(value), topLevel(topLevel),
                                                      insertFlag(false), deleteFlag(false) {}

        bool isMarked()
        {
            return linkFreeUtils::isMarked(next[0].load());
        }
    } __attribute__((aligned((64))));

private:
    Node *allocNode(intptr_t key, T value, uchar topLevel)
    {
        Node *newNode = static_cast<Node *>(ssmem_alloc(alloc, sizeof(Node)));
        linkFreeUtils::flipV1(&newNode->metaData);
        std::atomic_thread_fence(std::memory_order_release);
        newNode->insertFlag.store(false, std::memory_order_relaxed);
        newNode->deleteFlag.store(false, std::memory_order_relaxed);
        newNode->key = key;
        newNode->value = value;
        newNode->topLevel = topLevel;
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

    bool find(intptr_t key, Node **preds, Node **succs)
    {
        Node *pred, *predNext, *succ, *succNext;

    retry:
        pred = this->head;
        for (int i = MAX_LEVEL - 1; i >= 0; i--)
        {
            predNext = pred->next[i].load();
            if (linkFreeUtils::isMarked(predNext))
                goto retry;

            for (succ = predNext;; succ = succNext)
            {
                succNext = succ->next[i].load();
                while (linkFreeUtils::isMarked(succNext))
                {
                    // to make sure the deletion is in the NVRAM before skipping
                    //  if i != 0 the link never existed
                    if (i == 0)
                        FLUSH_DELETE(succ);
                    succ = linkFreeUtils::getRef<Node>(succNext);
                    succNext = succ->next[i].load();
                }
                if (succ->key >= key)
                    break;
                pred = succ;
                predNext = succNext;
            }

            if ((predNext != succ) && (!pred->next[i].compare_exchange_strong(predNext, succ)))
                goto retry;

            if (preds != nullptr)
            {
                preds[i] = pred;
                succs[i] = succ;
            }
        }

        return succ->key == key;
    }

    bool findNoCleanup(intptr_t key, Node **preds, Node **succs)
    {
        Node *pred, *succ;

        pred = this->head;
        for (int i = MAX_LEVEL - 1; i >= 0; i--)
        {
            succ = linkFreeUtils::getRef<Node>(pred->next[i].load());
            while (true)
            {
                if (!linkFreeUtils::isMarked(succ->next[i].load()))
                {
                    if (succ->key >= key)
                        break;
                    pred = succ;
                }
                succ = linkFreeUtils::getRef<Node>(succ->next[i].load());
            }
            preds[i] = pred;
            succs[i] = succ;
        }

        return succ->key == key;
    }

    bool findSuccsNoCleanup(intptr_t key, Node **succs)
    {
        Node *pred, *succ;

        pred = this->head;
        for (int i = MAX_LEVEL - 1; i >= 0; i--)
        {
            succ = linkFreeUtils::getRef<Node>(pred->next[i].load());
            while (true)
            {
                if (!linkFreeUtils::isMarked(succ->next[i].load()))
                {
                    if (succ->key >= key)
                        break;
                    pred = succ;
                }
                succ = linkFreeUtils::getRef<Node>(succ->next[i].load());
            }
            succs[i] = succ;
        }

        return succ->key == key;
    }

    inline bool markNode(Node *node)
    {
        bool result = false;
        Node *next;

        for (int i = node->topLevel - 1; i >= 0; i--)
        {
            do
            {
                next = node->next[i].load();
                if (linkFreeUtils::isMarked(next))
                {
                    result = false;
                    break;
                }
                Node *before = linkFreeUtils::getRef<Node>(next);
                Node *after = linkFreeUtils::mark<Node>(next);
                result = node->next[i].compare_exchange_strong(before, after);
            } while (!result);
        }

        return result;
    }

public:
    LinkFreeSkipList()
    {
        this->head = new Node(INT_MIN, 0, MAX_LEVEL);
        Node *last = new Node(INT_MAX, 0, MAX_LEVEL);
        for (int i = 0; i < MAX_LEVEL; i++)
        {
            this->head->next[i].store(last);
            last->next[i].store(nullptr);
        }
    }

    bool insert(intptr_t k, T item, int tid)
    {
        Node *newNode, *pred, *succ, *next;
        Node *preds[MAX_LEVEL], *succs[MAX_LEVEL];

    retry:

        bool found = findNoCleanup(k, preds, succs);
        if (found)
        {
            linkFreeUtils::makeValid(&succs[0]->metaData);
            FLUSH_INSERT(succs[0]);
            return false;
        }

        newNode = allocNode(k, item, get_random_level());

        for (int i = 0; i < newNode->topLevel; i++)
        {
            newNode->next[i].store(succs[i], std::memory_order_release);
        }

        Node *before = linkFreeUtils::getRef<Node>(succs[0]);
        if (!preds[0]->next[0].compare_exchange_strong(before, newNode))
        {
            newNode->next[0].store(linkFreeUtils::mark<Node>(nullptr));
            linkFreeUtils::makeValid(&newNode->metaData);
            ssmem_free(alloc, newNode);
            goto retry;
        }

        linkFreeUtils::makeValid(&newNode->metaData);
        FLUSH_INSERT(newNode);

        for (int i = 1; i < newNode->topLevel; i++)
        {
            while (true)
            {
                pred = preds[i];
                succ = succs[i];
                next = newNode->next[i].load();
                if (linkFreeUtils::isMarked(next))
                    return true;

                if (succ != next &&
                    !newNode->next[i].compare_exchange_strong(next, succ))
                {
                    return true;
                }

                if (pred->next[i].compare_exchange_strong(succ, newNode))
                    break;

                find(k, preds, succs);
            }
        }
        return true;
    }

    bool remove(intptr_t k, int tid)
    {
        Node *succs[MAX_LEVEL];

        bool found = findSuccsNoCleanup(k, succs);
        if (!found)
            return false;

        Node *node = succs[0];
        linkFreeUtils::makeValid(&node->metaData);
        bool result = markNode(node);
        FLUSH_DELETE(node);
        if (result)
        {
            find(k, nullptr, nullptr);
            ssmem_free(alloc, node);
            return true;
        }
        return false;
    }

    bool contains(intptr_t k, int tid)
    {
        Node *pred = this->head, *curr;

        for (int i = MAX_LEVEL - 1; i >= 0; i--)
        {
            curr = linkFreeUtils::getRef<Node>(pred->next[i].load());
            while (curr->key < k || linkFreeUtils::isMarked(curr->next[i].load()))
            {
                if (!linkFreeUtils::isMarked(curr->next[i].load()))
                    pred = curr;
                else if (i == 0 && curr->key == k)
                {
                    FLUSH_DELETE(curr);
                    return false;
                }
                curr = linkFreeUtils::getRef<Node>(curr->next[i].load());
            }

            // we found the right node
            if (curr->key == k)
            {
                linkFreeUtils::makeValid(&curr->metaData);
                FLUSH_INSERT(curr);
                return true;
            }
        }
        return false;
    }

private:
    Node *head;
};

#endif
