#ifndef PERSISTENT_NODE_H_
#define PERSISTENT_NODE_H_

#include <atomic>
#include "utilities.h"

template <class T>
class PNode
{
  public:
	std::atomic<bool> validStart, validEnd, deleted;
	atomic<intptr_t> key;
	atomic<T> value;

	PNode() : key(0), validStart(false), validEnd(false), deleted(false) {}

	bool alloc()
	{
		return !this->validStart.load();
	}

	void create(intptr_t key, T value, bool validity)
	{
		this->validStart.store(validity, std::memory_order_relaxed);
		std::atomic_thread_fence(std::memory_order_release);
		this->key.store(key, std::memory_order_relaxed);
		this->value.store(value, std::memory_order_relaxed);
		this->validEnd.store(validity, std::memory_order_release);
		BARRIER(this);
	}

	void destroy(bool validity)
	{
		this->deleted.store(validity, std::memory_order_release);
		BARRIER(this);
	}

	bool isValid()
	{
		return validStart.load() == validEnd.load() && validEnd.load() != deleted.load();
	}

	bool isDeleted()
	{
		return validStart.load() == validEnd.load() && validEnd.load() == deleted.load();
	}

	bool recoveryValidity()
	{
		return validStart.load();
	}

} __attribute__((aligned((32))));

#endif
