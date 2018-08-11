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

class Threadpool {
 private:
  std::vector< std::thread > workers;
  std::deque< std::function<void() > > tasks;
  std::mutex m;
  std::condition_variable cv_task;
  std::condition_variable cv_finished;
  std::atomic_uint busy;
  std::atomic_uint processed;
  std::atomic_bool stop;

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
  
 Threadpool(const unsigned int n = std::thread::hardware_concurrency()-1) :
  busy(0), processed(0), stop(false)
    {
      for (unsigned int i=0; i<n; ++i)
	workers.emplace_back(std::bind(&Threadpool::thread_func, this));
    }
  
  ~Threadpool() { if (!stop) exit(); }
  
  template<class T, typename... Args> void enqueue(T&& f, Args&&... args) {
    std::unique_lock<std::mutex> lock(m);
    // args to bind are copied or moved (not passed by reference) .. unless wrapped in std::ref()
    tasks.emplace_back(std::bind(std::forward<T>(f), std::ref(std::forward<Args>(args))...));
    cv_task.notify_one();
  }
  
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
    for (auto& t : workers) t.join();
  }
};


#endif
