/*
-----------------------------------------------------------------------------
This source file is part of the Havoc chess engine
Copyright (c) 2020 Minniesoft
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
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
  
   Threadpool(const unsigned int n = std::thread::hardware_concurrency() - 1) :
     busy(0), processed(0), stop(false), num_threads(n)
   {
     for (unsigned int i = 0; i < n; ++i)
       workers.emplace_back(std::bind(&Threadpool::thread_func, this));
   }
   
   ~Threadpool() { if (!stop) exit(); }
  
   template<class T, typename... Args> void enqueue(T&& f, Args&&... args) {
     std::unique_lock<std::mutex> lock(m);
     // args to bind are copied or moved (not passed by reference) .. unless wrapped in std::ref()
     tasks.emplace_back(std::bind(std::forward<T>(f), std::ref(std::forward<Args>(args))...));
     cv_task.notify_one();
   }
   
   void clear_tasks() { while (!tasks.empty()) tasks.pop_front();  }

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
    for (auto& t : workers) t.join();
  }
};

extern Threadpool search_threads;

#endif
