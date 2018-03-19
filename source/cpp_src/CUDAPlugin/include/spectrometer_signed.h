#ifndef PLASMA_LINE_S
#define PLASMA_LINE_S

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
#define N_BLOCKS_S 8
#define N_THREADS_S 1024
#define SAMPLE_RATE_S 10000000000 //10GHz


// 5 ms blanking
//#define BLANKING 2000000

// 0 ms blanking
//#define BLANKING 0
// 11 ms
#define BLANKING_S (floor(SAMPLE_RATE * 11e-3))

// #define SPECTRUM_LENGTH 32768 //131072 //  8192*2*2
// #define SPECTRUM_LENGTH_S 524288
//#define SPECTRUM_LENGTH_S 2048
// #define SPECTRUM_LENGTH_S 32768
#define SPECTRUM_LENGTH_S 131072
// debug output files
//#define WRITE_INPUT 1
//#define DEBUG_Z_OUT 1
#define TIMING_PRINT_S 1
//#define TEST_SIGNAL 1

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
} spectrometer_data_s;

typedef struct spectrometer_files_str_s {
  int n_files;
  int file_size;
  uint64_t *file_numbers;
  char *dirname;
} spectrometer_files_s;

typedef struct spectrometer_output_str_s {
  FILE *out;
  uint64_t ut_sec;
  int year, month, day;
  char dname[4096];
  char fname[4096];
} spectrometer_output_s;

extern "C" void process_vector_s(int16_t *d_in, spectrometer_data_s *d, uint64_t t0, spectrometer_output_s *o, char const *dname_results);
extern "C" void process_vector_no_output_s(int16_t *d_in, spectrometer_data_s *d, uint64_t t0);
extern "C" spectrometer_data_s *new_spectrometer_data_s(int data_length, int spectrum_length);
extern "C" spectrometer_output_s *new_spectrometer_output_s();
extern "C" void free_spectrometer_data_s(spectrometer_data_s *d);
__global__ void short_to_float_s(int16_t *ds, float *df, float *w, int n_spectra, int spectrum_length);
__global__ void square_and_accumulate_sum_s(cufftComplex *z, float *spectrum);
extern "C" void blackmann_harris_s( float* pOut, unsigned int num );
extern "C" void cuda_alloc_pinned_memory( void** ptr, size_t s);
extern "C" void cuda_free_pinned_memory(void* ptr);


#endif
