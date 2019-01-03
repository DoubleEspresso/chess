#include <iostream>
#include <cuda.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <device_functions.h>


#include "error.cuh"


#define BX blockIdx.x
#define BY blockIdx.y
#define TX threadIdx.x
#define TY threadIdx.y
#define DX blockDim.x 
#define DY blockDim.y
#define TIDX blockDim.x * blockIdx.x + threadIdx.x
#define TIDY blockDim.y * blockIdx.y + threadIdx.y


template<typename T> __global__ void mm_kernel(const T * a, const T * b, T * c, const int ra, const int ca, const int cb);


template<typename T>
void mm_device(const T * a, const T * b, T * c, const int ra, const int ca, const int cb) {
  cudaEvent_t start, stop;
  cudaEventCreate(&start);
  cudaEventCreate(&stop);
  
  T * ad; T * bd; T * cd;
  size_t sz_a = ra * ca * sizeof(T);
  size_t sz_b = ca * cb * sizeof(T);
  size_t sz_c = ra * cb * sizeof(T);

  /*
  gpuCheckErr(cudaMalloc((void**)&ad, sz_a));
  gpuCheckErr(cudaMalloc((void**)&bd, sz_b));
  gpuCheckErr(cudaMalloc((void**)&cd, sz_c));

  gpuCheckErr(cudaMemcpy(ad, a, sz_a, cudaMemcpyHostToDevice));
  gpuCheckErr(cudaMemcpy(bd, b, sz_b, cudaMemcpyHostToDevice));
  gpuCheckErr(cudaMemcpy(cd, c, sz_c, cudaMemcpyHostToDevice));
  */
  dim3 Threads(16, 16);
  int bx = (cb + Threads.x - 1) / Threads.x; bx = bx < 1024 ? bx : 1024;
  int by = (ra + Threads.y - 1) / Threads.y; by = by < 1024 ? by : 1024;		
  dim3 Grid(bx, by, 1);
  
  //printf("..dbg a(%d, %d), b(%d, %d), grid(%d,%d), threads(%d,%d)\n", ra, ca, ca, cb, bx, by, Threads.x, Threads.y);
  
  cudaEventRecord(start);
  mm_kernel<T><<<Grid, Threads>>>(ad, bd, cd, ra, ca, cb);
  gpu_check_err(cudaPeekAtLastError());
  gpu_check_err(cudaDeviceSynchronize());
  cudaEventRecord(stop);
  
  cudaMemcpy(c, cd, sz_c, cudaMemcpyDeviceToHost);
  cudaFree(ad);
  cudaFree(bd);
  cudaFree(cd);
  
  float ms = 0;
  cudaEventElapsedTime(&ms, start, stop);
  printf("..gpu_mm(%3.1fms)\n", ms);
}

template<typename T> __global__ void mm_kernel(const T * a, const T * b, T * c, const int ra, const int ca, const int cb) {
  // note : assumed that thread indices cover matrix 
  int tx = TIDX; // col
  int ty = TIDY; // row
  
  if (tx >= cb || ty >= ra) return;
  
  const int r_ca = ca - ca / DX * DX;  
  int num_mults = ca / DX;  
  int mm = (r_ca > 0 ? num_mults + 1 : num_mults);  
  int cidx = ty * cb + tx;
  
  for (int i = 0; i < mm; ++i) {
    int sa = DY * (i + ca * BY); // move to "right" in matrix "A" by 16x16 chunks 
    int sb = DX * (i * cb + BX); // move "down" matrix B by 16x16 chunks
    
    const T * sm_a = &(a[sa]); // collect sub-matrix of A
    const T * sm_b = &(b[sb]); // collect sub-matrix of B
    
    // fill one element of result matrix "c" 
    int mx = i >= num_mults ? r_ca : DX;    
    int cc = ca * TY;
    
    for (int j = 0; j < mx; ++j) {
      c[cidx] += sm_a[cc + j] * sm_b[cb * j + TX];
    }
    //__syncthreads();
  }  
}

//----------------------------------
// template specializations
//----------------------------------
template void mm_device(const char * a, const char * b, char * c, const int ra, const int ca, const int cb);
template void mm_device(const int * a, const int * b, int * c, const int ra, const int ca, const int cb);
template void mm_device(const float * a, const float * b, float * c, const int ra, const int ca, const int cb);
template void mm_device(const double * a, const double * b, double * c, const int ra, const int ca, const int cb);

template __global__ void mm_kernel(const char * a, const char * b, char * c, const int ra, const int ca, const int cb);
template __global__ void mm_kernel(const int * a, const int * b, int * c, const int ra, const int ca, const int cb);
template __global__ void mm_kernel(const float * a, const float * b, float * c, const int ra, const int ca, const int cb);
template __global__ void mm_kernel(const double * a, const double * b, double * c, const int ra, const int ca, const int cb);
