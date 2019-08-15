#ifndef SOFT_SKIP_LIST_H_
#define SOFT_SKIP_LIST_H_

#include <atomic>
#include <cstdlib>
#include <cstdio>
#include "rand_r_32.h"
#include "utilities.h"
#include "ssmem.h"

typedef softUtils::state state;

template <class T>
class SOFTSkipList
{
public:
	class Node
	{
	public:
		intptr_t key;
		T value;
		bool pValidity;
		std::atomic<bool> validStart, validEnd, deleted;
		uchar topLevel;
		std::atomic<Node *> next[MAX_LEVEL];

		Node(intptr_t key = 0,
			 uchar topLevel = 0) : key(key), pValidity(false), validStart(false),
								   validEnd(false), deleted(false), topLevel(topLevel) {}

		bool alloc()
		{
			return !validStart.load();
		}

		// a thread calls "create" privately
		void create(intptr_t key, T value, uchar topLevel, bool validity)
		{
			this->validStart.store(validity, std::memory_order_relaxed);
			std::atomic_thread_fence(std::memory_order_release);
			this->key = key;
			this->value = value;
			this->topLevel = topLevel;
			this->pValidity = validity;
		}

		// help can be called by multiple threads
		void help()
		{
			this->validEnd.store(pValidity, std::memory_order_release);
			BARRIER(this);
		}

		void destroy()
		{
			this->deleted.store(pValidity, std::memory_order_release);
			BARRIER(this);
		}

		bool isValid()
		{
			return validStart.load() == validEnd.load() &&
				   validEnd.load() != deleted.load();
		}

		bool isDeleted()
		{
			return validStart.load() == validEnd.load() &&
				   validEnd.load() == deleted.load();
		}

		bool stateCAS(state expected, state newState)
		{
			bool result = false;
			while (!result && softUtils::getState(this->next[0].load()) == expected)
				result = softUtils::stateCAS<Node>(this->next[0], expected, newState);
			return result;
		}

	} __attribute__((aligned((64))));

private:
	Node *allocNode(intptr_t key, T value, uchar toplevel)
	{
		Node *node = static_cast<Node *>(ssmem_alloc(alloc, sizeof(Node)));
		node->create(key, value, toplevel, node->alloc());
		return node;
	}

	bool find(intptr_t key, Node **preds, Node **succs, state *succStates)
	{
		Node *pred, *predNext, *succ, *succNext;
		state predState, succState;

	retry:
		pred = this->head;
		for (int i = MAX_LEVEL - 1; i >= 0; i--)
		{
			predNext = pred->next[i].load();
			predState = softUtils::getState(predNext);
			predNext = softUtils::getRef<Node>(predNext);
			if (predState == state::DELETED)
				goto retry;

			for (succ = predNext;; succ = succNext)
			{
				succNext = succ->next[i].load();
				succState = softUtils::getState(succNext);
				succNext = softUtils::getRef<Node>(succNext);
				while (succState == state::DELETED)
				{
					succ = succNext;
					succNext = succ->next[i].load();
					succState = softUtils::getState(succNext);
					succNext = softUtils::getRef<Node>(succNext);
				}
				if (succ->key >= key)
					break;
				pred = succ;
				predState = succState;
				predNext = succNext;
			}

			Node *before = softUtils::createRef<Node>(predNext, predState);
			Node *after = softUtils::createRef<Node>(succ, predState);
			if ((predNext != succ) && (!pred->next[i].compare_exchange_strong(before, after)))
				goto retry;

			if (preds != nullptr)
			{
				preds[i] = pred;
				succs[i] = after;
				succStates[i] = succState;
			}
		}

		return succ->key == key;
	}

	bool findNoCleanup(intptr_t key, Node **preds, Node **succs, state *succStates)
	{
		Node *pred, *succ;
		state predState, succState;

		pred = this->head;
		for (int i = MAX_LEVEL - 1; i >= 0; i--)
		{
			succ = softUtils::getRef<Node>(pred->next[i].load());
			predState = softUtils::getState(succ);
			succ = softUtils::getRef<Node>(succ);
			while (true)
			{
				succState = softUtils::getState(succ->next[i].load());
				if (succState != state::DELETED)
				{
					if (succ->key >= key)
						break;
					pred = succ;
					predState = succState;
				}
				succ = softUtils::getRef<Node>(succ->next[i].load());
			}
			preds[i] = pred;
			succs[i] = softUtils::createRef(succ, predState);
			succStates[i] = succState;
		}

		return succ->key == key;
	}

	bool findSuccsNoCleanup(intptr_t key, Node **succs, state *succStates)
	{
		Node *pred, *succ;
		state predState, succState;

		pred = this->head;
		for (int i = MAX_LEVEL - 1; i >= 0; i--)
		{
			succ = softUtils::getRef<Node>(pred->next[i].load());
			predState = softUtils::getState(succ);
			succ = softUtils::getRef<Node>(succ);
			while (true)
			{
				succState = softUtils::getState(succ->next[i].load());
				if (succState != state::DELETED)
				{
					if (succ->key >= key)
						break;
					pred = succ;
					predState = succState;
				}
				succ = softUtils::getRef<Node>(succ->next[i].load());
			}
			succs[i] = succ;
			succStates[i] = succState;
		}

		return succ->key == key;
	}

	inline bool markNodes(Node *n)
	{
		Node *next, *before, *after;

		for (int i = n->topLevel - 1; i >= 1; i--)
		{
			do
			{
				next = n->next[i].load();
				if (softUtils::getState(next) == state::DELETED)
					break;
				before = softUtils::getRef<Node>(next);
				after = softUtils::createRef<Node>(next, state::DELETED);
			} while (!n->next[i].compare_exchange_strong(before, after));
		}
		return n->stateCAS(state::INSERTED, state::INTEND_TO_DELETE); /* if I was the one that marked lvl 0 */
	}

public:
	SOFTSkipList()
	{

		Node *min, *max;
		max = new Node(INTPTR_MAX, MAX_LEVEL);
		min = new Node(INTPTR_MIN, MAX_LEVEL);
		for (int i = 0; i < MAX_LEVEL; i++)
		{
			min->next[i].store(max, std::memory_order_release);
			max->next[i].store(nullptr, std::memory_order_release);
		}
		this->head = min;
	}

	bool contains(intptr_t key, int tid)
	{
		Node *pred = this->head, *curr;

		for (int i = MAX_LEVEL - 1; i >= 0; i--)
		{
			curr = softUtils::getRef<Node>(pred->next[i].load());
			while (curr->key < key || softUtils::isOut(curr->next[i].load()))
			{
				if (!softUtils::isOut(curr->next[i].load()))
					pred = curr;
				curr = softUtils::getRef<Node>(curr->next[i].load());
			}

			// we found the right node
			if (curr->key == key)
				return true;
		}
		return false;
	}

	bool remove(intptr_t key, int tid)
	{
		Node *succs[MAX_LEVEL];
		state succStates[MAX_LEVEL];

		bool found = findSuccsNoCleanup(key, succs, succStates);
		if (!found)
			return false;

		Node *node = softUtils::getRef<Node>(succs[0]);
		if (isOut(succStates[0]))
			return false;

		bool result = markNodes(node);

		node->destroy();
		node->stateCAS(state::INTEND_TO_DELETE, state::DELETED);

		if (result)
		{
			find(key, nullptr, nullptr, nullptr);
			ssmem_free(alloc, node);
			return true;
		}

		return false;
	}

	bool insert(intptr_t key, T value, int tid)
	{
		Node *newNode, *pred, *succ, *next;
		Node *preds[MAX_LEVEL], *succs[MAX_LEVEL];
		state succStates[MAX_LEVEL];
		bool result;

	retry:

		bool found = findNoCleanup(key, preds, succs, succStates);
		if (found)
		{
			if (succStates[0] != state::INTEND_TO_INSERT)
				return false;
			newNode = softUtils::getRef<Node>(succs[0]);
			newNode->help();
			newNode->stateCAS(state::INTEND_TO_INSERT, state::INSERTED);
			return false;
		}
		newNode = allocNode(key, value, get_random_level());
		Node *succRef = softUtils::getRef<Node>(succs[0]);
		newNode->next[0].store(softUtils::createRef<Node>(succRef, state::INTEND_TO_INSERT),
							   std::memory_order_release);
		for (int i = 1; i < newNode->topLevel; i++)
		{
			Node *currRef = softUtils::getRef<Node>(succs[i]);
			newNode->next[i].store(softUtils::createRef<Node>(currRef, state::INSERTED),
								   std::memory_order_release);
		}

		Node *after = softUtils::createRef<Node>(newNode, softUtils::getState(succs[0]));
		if (!preds[0]->next[0].compare_exchange_strong(succs[0], after))
		{
			newNode->validStart.store(!newNode->pValidity);
			ssmem_free(alloc, newNode);
			goto retry;
		}

		newNode->help();
		newNode->stateCAS(state::INTEND_TO_INSERT, state::INSERTED);

		for (int i = 1; i < newNode->topLevel; i++)
		{
			while (true)
			{
				pred = preds[i];
				succ = succs[i];
				next = newNode->next[i].load();
				if (softUtils::isOut(next))
					return true;

				if (succ != next &&
					!newNode->next[i].compare_exchange_strong(next, succ))
				{
					return true;
				}

				if (pred->next[i].compare_exchange_strong(succ, newNode))
					break;

				find(key, preds, succs, succStates);
			}
		}
		return true;
	}

private:
	Node *head;

} __attribute__((aligned((64))));

#endif /* SOFT_SKIP_LIST_H_ */
