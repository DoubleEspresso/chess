#pragma once
#ifndef HEADWIG_THREADS_H
#define HEADWIG_THREADS_H

#include <vector>
#include <bitset>

#include "definitions.h"
#include "platform.h"
#include "board.h"
//#include "order.h"
//#include "search.h" // for limits definition


extern U16 BestMoves[2];

class ThreadPool;
class ThreadMaster;
class ThreadTimer;
class MoveSelect;
struct Limits;
struct Node;

const int SPLITS_PER_THREAD = 8;

extern ThreadPool Threads;

class NativeMutex
{
public:
	NativeMutex()
	{
#ifdef _WIN32
		InitializeCriticalSection(&mutex);
#else 
		mutex_init(mutex);
#endif
	};
	~NativeMutex() {};

	void lock() { mutex_lock(mutex); }
	void unlock() { mutex_unlock(mutex); }
	MUTEX mutex;
private:
	friend struct ConditionVariable;
};


struct ConditionVariable {
	ConditionVariable() { thread_cond_init(c); }
	~ConditionVariable() { cond_destroy(c); }

	void wait(NativeMutex& m) { thread_wait(c, m.mutex); }
#ifdef __linux
	void wait_for(NativeMutex& m, timespec& d) { thread_timed_wait(c, m.mutex, d); }
#else
	void wait_for(NativeMutex& m, int ms) { thread_timed_wait(c, m.mutex, ms); }
#endif
	void signal() { thread_signal(c); }

private:
	CONDITION c;
};


struct SplitBlock
{
	const Board * b;
	volatile bool cut_occurred;
	volatile U16 bestmove;
	volatile int alpha;
	volatile int beta;
	volatile int besteval;
	volatile bool allSlavesSearching;
	volatile int nodes_searched;
	int depth;
	SplitBlock* parentSplitBlock;
	bool cutNode;
	Node * node;

	NativeMutex split_mutex;
	MoveSelect * ms;
	Thread * masterThread;
	std::bitset<MAX_THREADS> slaves_mask;
};

class ThreadBase
{
public:
	ThreadBase() { };
	virtual ~ThreadBase() {};

	virtual void idle_loop() = 0;
	void notify();
	void wait_for(volatile const bool& b);

	NativeMutex mutex;
	ConditionVariable sleep_condition;
};

class Thread : public ThreadBase
{
public:
	Thread();
	virtual void idle_loop();
	bool found_cutoff() const;
	bool available_to(const Thread* master) const;

	bool split(Board& b, U16* bestmove, Node* node, int alpha, int beta, int* besteval, int depth, MoveSelect* ms, bool cutNode);

	SplitBlock SplitBlocks[SPLITS_PER_THREAD];
	Board* currBoard;
	size_t idx;
	SplitBlock* volatile currSplitBlock;
	volatile int nb_splits;
	volatile bool searching;
	THREAD handle;
};

class ThreadMaster : public Thread
{
public:
	ThreadMaster() : thinking(true) {};
	virtual void idle_loop();
	volatile bool thinking;
	THREAD handle;
};


class ThreadTimer : Thread
{
public:
	ThreadTimer();
	void init();
	virtual void idle_loop();
	void check_time();
	double estimate_max_time();

	volatile bool searching, adding_time;
	volatile double elapsed;
	Limits * search_limits;
	NativeMutex tmutex, moretime_mutex;
	ConditionVariable sleep_condition;
	ConditionVariable moretime_condition;
	THREAD handle;
private:
	double max, now;
};


class ThreadPool : public std::vector<Thread*>
{
public:
	void init();
	void exit();
	ThreadMaster* main() { return static_cast<ThreadMaster*>(at(0)); }
	Thread* available_slave(const Thread * master) const;
	void wait_for_search_finished();
	void start_thinking(const Board &b);

	NativeMutex mutex;
	ConditionVariable sleep_condition;

};

extern ThreadTimer * timer_thread;

#endif
