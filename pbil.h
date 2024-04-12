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
#pragma once

#ifndef PBIL_H
#define PBIL_H


// Population based incremental learning

#include <vector>
#include <random>
#include <functional>

#include "threads.h"


class pbil {

 private:
  double mutate_prob, mutate_shift;
  double best_err, learn_rate, neg_learn_rate, etol;
  std::vector<double> probabilities;
  std::vector<std::vector<int>> samples;
  std::vector<int> best_sample, initial_guess;
  std::mt19937 rng;

  void educate();
  void educate(float min, float max);

  void initialize_probabilities();

  void update_probabilities(const std::vector<int>& min_gene,
			    const std::vector<int>& max_gene);
    
  void mutate();
  void init();
  
 public:
  
 pbil(const size_t& popsz,
      const size_t& nbits,
      const double& mutate_p,
      const double& mutate_s,
      const double& lr,
      const double& nlr,
      const double& tol) :
   mutate_prob(mutate_p), mutate_shift(mutate_s), best_err(1e10),
   learn_rate(lr), neg_learn_rate(nlr), etol(tol)
  {
    samples =
      std::vector<std::vector<int>>(popsz, std::vector<int>(nbits));

    probabilities = std::vector<double>(nbits);

    rng.seed(std::random_device{}());
  };
  
  pbil& operator=(const pbil& o) = delete;
  pbil& operator=(const pbil&& o) = delete;
  pbil(const pbil& o) = delete;
  ~pbil() { } 

  
  template<class T, typename... Args>
  void optimize(T&& residual, Args&&... args);

  void set_initial_guess(const std::vector<int>& guess) {
    for (auto& g : guess) initial_guess.push_back(g);
  }

};

#include "pbil.hpp"

#endif
