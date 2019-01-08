#include <vector>
#include <random>
#include <functional>
#include <iostream>

#include "pbil.h"

int integer(const std::vector<int>& sample, const int& sidx, const int& eidx) {
  int result = 0;
  for (int j = sidx, s = eidx - 1 - sidx; j < eidx; ++j, --s) {
    result |= (sample[j] << s); // bit ordering is e.g. 4 = {1,0,0}
  }
  return result;
}

// compute error for toy problem : find binary representation of a given "answer" integer.
double residual(std::vector<int> sample, int length, int answer) {
  return std::abs(integer(sample, 0, length) - answer);
}

void problem1() {
  // find binary representation of a given input integer  
  int answer = 93019431;
  int length = (int)log2(answer)+1;
  pbil p(300, length, 0.7, 0.15, 0.3, 0.05, 1e-6); 
  p.optimize(residual, length, answer);    
}


int main() {
  
  problem1();

}
