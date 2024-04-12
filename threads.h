
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <deque>
#include <functional>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <atomic>
#include <cassert>

#include "material.h"
#include "pawns.h"


class Workerthread {
private:
	std::thread m_thread;
	std::function<void()> m_func;

public:
	Workerthread() { }
	Workerthread(std::function<void()> func) : m_thread(func), m_func(func) { }
	Workerthread(const Workerthread& o) {
		m_thread = std::thread(o.m_func);
	}
	virtual ~Workerthread() { }

	std::thread& thread() { return m_thread; }
};


class Searchthread : public Workerthread {
public:
	material_table materialTable;
	pawn_table pawnTable;

public:
	Searchthread() {}
	Searchthread(std::function<void()> func) : Workerthread(func) { }
	Searchthread(const Searchthread& o) {
		pawnTable = o.pawnTable;
		materialTable = o.materialTable;
	}
};


class Timerthread : public Workerthread {
public:
	Timerthread() {}
	Timerthread(std::function<void()> func) : Workerthread(func) { }
};

template<class T>
class Threadpool {
private:
	std::vector<T*> workers;
	std::deque< std::function<void() > > tasks;
	std::mutex m;
	std::condition_variable cv_task;
	std::condition_variable cv_finished;
	std::atomic_uint busy;
	std::atomic_uint processed;
	std::atomic_bool stop;
	unsigned int num_threads;

	void thread_func() {
		while (true) {
			std::unique_lock<std::mutex> lock(m);
			cv_task.wait(lock, [this]() { return stop || !tasks.empty(); });
			if (!tasks.empty()) {
				++busy;
				auto fn = tasks.front();
				tasks.pop_front();
				lock.unlock();
				fn();
				++processed;
				lock.lock();
				--busy;
				cv_finished.notify_one();
			}
			else if (stop) break;
		}
	}


public:
	Threadpool<T>() { }

	Threadpool<T>(const unsigned int n) :
		busy(0), processed(0), stop(false), num_threads(n)
	{
		for (unsigned int i = 0; i < n; ++i)
			workers.emplace_back(new T(std::bind(&Threadpool<T>::thread_func, this)));
	}

	~Threadpool<T>() { if (!stop) exit(); }

	T* operator[](const int& idx) { return workers[idx]; }

	void init(const int& numThreads) {
		busy = 0;
		processed = 0;
		stop = false;
		num_threads = numThreads;
		for (unsigned int i = 0; i < num_threads; ++i)
			workers.emplace_back(new T(std::bind(&Threadpool<T>::thread_func, this)));
	}

	template<class T, typename... Args> void enqueue(T&& f, Args&&... args) {
		std::unique_lock<std::mutex> lock(m);
		// args to bind are copied or moved (not passed by reference) .. unless wrapped in std::ref()
		tasks.emplace_back(std::bind(std::forward<T>(f), std::ref(std::forward<Args>(args))...));
		cv_task.notify_one();
	}

	void clear_tasks() { while (!tasks.empty()) tasks.pop_front(); }

	unsigned int size() { return num_threads; }

	void wait_finished() {
		std::unique_lock<std::mutex> lock(m);
		cv_finished.wait(lock, [this]() { return tasks.empty() && (busy == 0); });
	}

	unsigned int get_processed() const { return processed; }

	void exit() {
		std::unique_lock<std::mutex> lock(m);
		stop = true;
		cv_task.notify_all();
		lock.unlock();
		for (auto& t : workers) {
			t->thread().join();
			delete t;
		}
	}
};

extern Threadpool<Searchthread> SearchThreads;

#endif
