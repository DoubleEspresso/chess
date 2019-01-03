#pragma once
#ifndef ERROR_H
#define ERROR_H

#include <cuda_runtime.h>
#include <stdio.h>
#include <cstdlib>


#define gpu_check_err(ans) { gpu_assert((ans), __FILE__, __LINE__); }


inline void gpu_assert(cudaError_t code, const char * file, int line, bool abort = true) {
  if (code != cudaSuccess) {
    fprintf(stderr, "CUDA error: %s %s %d\n", cudaGetErrorString(code), file, line);
  }	    
}

#endif