#ifndef PLASMA_LINE
#define PLASMA_LINE

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
// what is the number of files to  process
//#define N_FILES 300
// require files to be of this size
#define SAMPLE_RATE 200000000

// 5 ms blanking
//#define BLANKING 2000000

// 0 ms blanking
//#define BLANKING 0
// 11 ms
#define BLANKING (floor(SAMPLE_RATE * 11e-3))

// #define SPECTRUM_LENGTH 32768 //131072 //  8192*2*2
#define SPECTRUM_LENGTH 131072 //  8192*2*2
// debug output files
//#define WRITE_INPUT 1
//#define DEBUG_Z_OUT 1
#define TIMING_PRINT 1
//#define TEST_SIGNAL 1

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
} spectrometer_data;

typedef struct spectrometer_files_str {
  int n_files;
  int file_size;
  uint64_t *file_numbers;
  char *dirname;
} spectrometer_files;

typedef struct spectrometer_output_str {
  FILE *out;
  uint64_t ut_sec;
  int year, month, day;
  char dname[4096];
  char fname[4096];
} spectrometer_output;

extern "C" void process_vector(uint16_t *d_in, spectrometer_data *d, uint64_t t0, spectrometer_output *o, char const *dname_results);
extern "C" spectrometer_data *new_spectrometer_data(int data_length, int spectrum_length);
extern "C" spectrometer_output *new_spectrometer_output();
extern "C" void free_spectrometer_data(spectrometer_data *d);
__global__ void short_to_float(uint16_t *ds, float *df, float *w, int n_spectra, int spectrum_length);
__global__ void square_and_accumulate_sum(cufftComplex *z, float *spectrum);
extern "C" void blackmann_harris( float* pOut, unsigned int num );

#endif
