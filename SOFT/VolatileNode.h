#ifndef VOLATILE_NODE_H_
#define VOLATILE_NODE_H_

#include "PNode.h"
#include <atomic>

template <class T>
class Node
{
  public:
	intptr_t key;
	T value;
	PNode<T> *pptr;
	bool pValidity;
	std::atomic<Node *> next;

	Node(intptr_t key, T value, PNode<T> *pptr, bool pValidity) : key(key), value(value), pptr(pptr), pValidity(pValidity), next(nullptr) {}

}; 

#endif
