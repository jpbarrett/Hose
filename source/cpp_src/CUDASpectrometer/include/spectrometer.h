#ifndef HSPECTROMETER_H__
#define HSPECTROMETER_H__

// system includes
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include <math.h>

#include <sys/stat.h>
#include <dirent.h>

// CUDA includes
#include <cuComplex.h>
#include <cufft.h>
#include <stdint.h>
#include <cuda_runtime_api.h>
#include <cuda.h>


// GPU parallelization
#define N_BLOCKS 8
#define N_THREADS 1024

// GPU parallelization
#define N_BLOCKS_S 8
#define N_THREADS_S 1024

typedef struct spectrometer_data_str_s
{
  float *d_in;
  int16_t *ds_in;
  cufftComplex *d_z_out;
  float *d_spectrum;
  float *spectrum;
  float *d_window;
  float *window;
  cufftHandle plan;
  int spectrum_length;
  int data_length;
  int n_spectra;
  uint64_t acquistion_start_second;
  uint64_t leading_sample_index;
  double sample_rate;
  int validity_flag;
} spectrometer_data_s;

typedef struct spectrometer_data_str
{
  float *d_in;
  uint16_t *ds_in;
  cufftComplex *d_z_out;
  float *d_spectrum;
  float *spectrum;
  float *d_window;
  float *window;
  cufftHandle plan;
  int spectrum_length;
  int data_length;
  int n_spectra;
  uint64_t acquistion_start_second;
  uint64_t leading_sample_index;
  double sample_rate;
  int validity_flag;
} spectrometer_data;

extern "C" void process_vector_no_output(uint16_t *d_in, spectrometer_data *d);
extern "C" void process_vector_no_output_s(int16_t *d_in, spectrometer_data_s *d);

extern "C" spectrometer_data *new_spectrometer_data(int data_length, int spectrum_length);
extern "C" spectrometer_data_s *new_spectrometer_data_s(int data_length, int spectrum_length);

extern "C" void free_spectrometer_data(spectrometer_data *d);
extern "C" void free_spectrometer_data_s(spectrometer_data_s *d);

__global__ void short_to_float(uint16_t *ds, float *df, float *w, int n_spectra, int spectrum_length);
__global__ void short_to_float_s(int16_t *ds, float *df, float *w, int n_spectra, int spectrum_length);

__global__ void square_and_accumulate_sum(cufftComplex *z, float *spectrum);

extern "C" void blackmann_harris( float* pOut, unsigned int num );

extern "C" void cuda_alloc_pinned_memory( void** ptr, size_t s);
extern "C" void cuda_free_pinned_memory(void* ptr);


#endif
