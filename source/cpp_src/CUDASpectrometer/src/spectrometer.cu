/* Project that takes signal and calculates averaged power spectrum using NVIDIA
   CUDA GPGPU programming. 

   Juha Vierinen (x@mit.edu)
   Cory Cotter (optimization of square and accumulate sum)
   John Barrett (added support for signed int, stripped down code to minimal library)
*/


#include "spectrometer.h"
#define MIN(X,Y) (((X) < (Y)) ? (X) : (Y))

void cuda_alloc_pinned_memory( void** ptr, size_t s)
{
    int code = cudaMallocHost(ptr, s);
    if(code != cudaSuccess)
    {
        //set pointer to null if there is an error
        fprintf(stderr, "Cuda error: cudaMallocHost failed.\n");
        *ptr = NULL;
    }
}

void cuda_free_pinned_memory(void* ptr)
{
    int code = cudaFreeHost(ptr);
    if(code != cudaSuccess)
    {
        fprintf(stderr, "Cuda error: cudaFreeHost failed.\n");
    }
}

extern "C" spectrometer_data *new_spectrometer_data(int data_length, int spectrum_length)
{
  spectrometer_data *d;
  int n_spectra;
  n_spectra=data_length/spectrum_length;

  d=(spectrometer_data *)malloc(sizeof(spectrometer_data));

  // result on the cpu
  d->spectrum = (float *)malloc( (spectrum_length/2+1) * sizeof(float));

  d->data_length=data_length;
  d->spectrum_length=spectrum_length;
  d->n_spectra=n_spectra;

  d->window = (float *)malloc(spectrum_length*sizeof(float));
  blackmann_harris(d->window,spectrum_length);

  // allocating device memory to the above pointers
  // reserve extra for +1 in place transforms
  if (cudaMalloc((void **) &d->d_in, sizeof(cufftComplex)*n_spectra*(spectrum_length/2 + 1)) != cudaSuccess)
  {
    fprintf(stderr, "Cuda error: Failed to allocate input data vector\n");
    exit(EXIT_FAILURE);
  }
  if (cudaMalloc((void **) &d->d_z_out, sizeof(cufftComplex)*n_spectra*(spectrum_length/2 + 1)) != cudaSuccess)
  {
    fprintf(stderr, "Cuda error: Failed to allocate input data vector\n");
    exit(EXIT_FAILURE);
  }

  if (cudaMalloc((void **) &d->d_window, sizeof(float)*spectrum_length) != cudaSuccess)
  {
    fprintf(stderr, "Cuda error: Failed to allocate input data vector\n");
    exit(EXIT_FAILURE);
  }

  // in-place seems to have a bug that causes the end to be garbled.
  //  d->d_z_out =(cufftComplex *) d->d_in;

  if (cudaMalloc((void **) &d->ds_in,sizeof(uint16_t)*data_length) != cudaSuccess)
  {
    fprintf(stderr, "Cuda error: Failed to allocate input data vector\n");
    exit(EXIT_FAILURE);
  }

  if (cudaMalloc((void **) &d->d_spectrum,sizeof(float)*(spectrum_length/2+1))
      != cudaSuccess)
  {
    fprintf(stderr, "Cuda error: Failed to allocate spectrum\n");
    exit(EXIT_FAILURE);
  }

  // initializing 1D FFT plan, this will tell cufft execution how to operate
  // cufft is well optimized and will run with different parameters than above
  //    cufftHandle plan;
  if (cufftPlan1d(&d->plan, spectrum_length, CUFFT_R2C, n_spectra) != CUFFT_SUCCESS) 
  {
    fprintf(stderr, "CUFFT error: Plan creation failed\n");
    exit(EXIT_FAILURE);
  }

  // copy window to device
  if (cudaMemcpy(d->d_window, d->window, sizeof(float)*spectrum_length, cudaMemcpyHostToDevice) != cudaSuccess)
  {
    fprintf(stderr, "Cuda error: Memory copy failed, window function HtD\n");
    exit(EXIT_FAILURE);
  }
  return(d);
}

extern "C" void free_spectrometer_data(spectrometer_data *d)
{
  if (cudaFree(d->d_in) != cudaSuccess) {
    fprintf(stderr, "Cuda error: Failed to free input\n");
    exit(EXIT_FAILURE);
  }
  free(d->window);
  free(d->spectrum);
  if (cudaFree(d->d_window) != cudaSuccess) {
    fprintf(stderr, "Cuda error: Failed to free window function\n");
    exit(EXIT_FAILURE);
  }
  if (cudaFree(d->d_z_out) != cudaSuccess) {
    fprintf(stderr, "Cuda error: Failed to free input\n");
    exit(EXIT_FAILURE);
  }
  if (cudaFree(d->ds_in) != cudaSuccess) {
    fprintf(stderr, "Cuda error: Failed to free input\n");
    exit(EXIT_FAILURE);
  }

  if (cudaFree(d->d_spectrum) != cudaSuccess) {
    fprintf(stderr, "Cuda error: Failed to free spec\n");
    exit(EXIT_FAILURE);
  }
  if (cufftDestroy(d->plan) != CUFFT_SUCCESS) {
    fprintf(stderr, "CUFFT error: Failed to destroy plan\n");
    exit(EXIT_FAILURE);
  }
  free(d);
}


extern "C" spectrometer_data_s *new_spectrometer_data_s(int data_length, int spectrum_length)
{
  spectrometer_data_s *d;
  int n_spectra;
  n_spectra=data_length/spectrum_length;

  unsigned long available, total;
  cudaMemGetInfo(&available, &total);
    printf("gpu mem: avail and total = %lu and %lu \n", available, total);


  d=(spectrometer_data_s *)malloc(sizeof(spectrometer_data_s));

  // result on the cpu
  d->spectrum = (float *)malloc( (spectrum_length/2+1) * sizeof(float));

  d->data_length=data_length;
  d->spectrum_length=spectrum_length;
  d->n_spectra=n_spectra;

  d->window = (float *)malloc(spectrum_length*sizeof(float));
  blackmann_harris(d->window,spectrum_length);

  // allocating device memory to the above pointers
  // reserve extra for +1 in place transforms
    unsigned long want =  sizeof(cufftComplex)*n_spectra*(spectrum_length/2 + 1);
    printf("required gpu buff size = %lu \n", want);


  int code = cudaMalloc( (void **) &d->d_in, sizeof(cufftComplex)*n_spectra*(spectrum_length/2 + 1) );
  if( code != cudaSuccess)
  {
    printf("code = %d\n", code);
    fprintf(stderr, "Cuda error: Failed to allocate input data vector\n");
    exit(EXIT_FAILURE);
  }

  cudaMemGetInfo(&available, &total);
    printf("gpu mem: avail and total = %lu and %lu\n", available, total);


  code = cudaMalloc((void **) &d->d_z_out, sizeof(cufftComplex)*n_spectra*(spectrum_length/2 + 1));
  if ( code != cudaSuccess)
  {
    printf("code = %d\n", code);
    fprintf(stderr, "Cuda error: Failed to allocate input data vector\n");
    exit(EXIT_FAILURE);
  }

  cudaMemGetInfo(&available, &total);
    printf("gpu mem: avail and total = %lu and %lu\n", available, total);


    code = cudaMalloc((void **) &d->d_window, sizeof(float)*spectrum_length);
  if (code != cudaSuccess)
  {
    printf("code = %d\n", code);
    fprintf(stderr, "Cuda error: Failed to allocate input data vector\n");
    exit(EXIT_FAILURE);
  }

  cudaMemGetInfo(&available, &total);
    printf("gpu mem: avail and total = %lu and %lu\n", available, total);


  // in-place seems to have a bug that causes the end to be garbled.
  //  d->d_z_out =(cufftComplex *) d->d_in;

    code = cudaMalloc((void **) &d->ds_in,sizeof(int16_t)*data_length);
  if (code != cudaSuccess)
  {
    printf("code = %d\n", code);
    fprintf(stderr, "Cuda error: Failed to allocate input data vector\n");
    exit(EXIT_FAILURE);
  }

  cudaMemGetInfo(&available, &total);
    printf("gpu mem: avail and total = %lu and %lu\n", available, total);


  if (cudaMalloc((void **) &d->d_spectrum,sizeof(float)*(spectrum_length/2+1))
      != cudaSuccess)
  {
    fprintf(stderr, "Cuda error: Failed to allocate spectrum\n");
    exit(EXIT_FAILURE);
  }

  cudaMemGetInfo(&available, &total);
    printf("gpu mem: avail and total = %lu and %lu\n", available, total);


  // initializing 1D FFT plan, this will tell cufft execution how to operate
  // cufft is well optimized and will run with different parameters than above
  //    cufftHandle plan;
    
  if (cufftPlan1d(&d->plan, spectrum_length, CUFFT_R2C, n_spectra) != CUFFT_SUCCESS)
  {
    fprintf(stderr, "CUFFT error: Plan creation failed\n");
    fprintf(stderr, "spec len, n spec = %d, %d", spectrum_length, n_spectra);
    exit(EXIT_FAILURE);
  }

  // copy window to device
  if (cudaMemcpy(d->d_window, d->window, sizeof(float)*spectrum_length, cudaMemcpyHostToDevice) != cudaSuccess)
  {
    fprintf(stderr, "Cuda error: Memory copy failed, window function HtD\n");
    exit(EXIT_FAILURE);
  }
  return(d);
}

extern "C" void free_spectrometer_data_s(spectrometer_data_s *d)
{
  if (cudaFree(d->d_in) != cudaSuccess) {
    fprintf(stderr, "Cuda error: Failed to free input\n");
    exit(EXIT_FAILURE);
  }
  free(d->window);
  free(d->spectrum);
  if (cudaFree(d->d_window) != cudaSuccess) {
    fprintf(stderr, "Cuda error: Failed to free window function\n");
    exit(EXIT_FAILURE);
  }
  if (cudaFree(d->d_z_out) != cudaSuccess) {
    fprintf(stderr, "Cuda error: Failed to free input\n");
    exit(EXIT_FAILURE);
  }
  if (cudaFree(d->ds_in) != cudaSuccess) {
    fprintf(stderr, "Cuda error: Failed to free input\n");
    exit(EXIT_FAILURE);
  }

  if (cudaFree(d->d_spectrum) != cudaSuccess) {
    fprintf(stderr, "Cuda error: Failed to free spec\n");
    exit(EXIT_FAILURE);
  }
  if (cufftDestroy(d->plan) != CUFFT_SUCCESS) {
    fprintf(stderr, "CUFFT error: Failed to destroy plan\n");
    exit(EXIT_FAILURE);
  }
  free(d);
}


/*
  create Blackmann-Harris window function
 */
void blackmann_harris(float* pOut, unsigned int num)
{
  const float a0      = 0.35875f;
  const float a1      = 0.48829f;
  const float a2      = 0.14128f;
  const float a3      = 0.01168f;

  unsigned int idx    = 0;
  while( idx < num )
  {
    pOut[idx]   = a0 - (a1 * cosf( (2.0f * M_PI * idx) / (num - 1) )) + (a2 * cosf( (4.0f * M_PI * idx) / (num - 1) )) - (a3 * cosf( (6.0f * M_PI * idx) / (num - 1) ));
    idx++;
  }
}

/*
  average spectra
 */
__global__ void square_and_accumulate_sum(const cufftComplex* d_in,
					  float* d_out,
					  const unsigned n_spectra,
					  const unsigned spectrum_length)
{
    unsigned idx = threadIdx.x + blockDim.x*blockIdx.x;
    while (idx < spectrum_length)
    {
        float result = 0.0;
        for (int i = 0; i < n_spectra; i++)
        {
            int d_idx = i * spectrum_length + idx;
            result += d_in[d_idx].x * d_in[d_idx].x + d_in[d_idx].y * d_in[d_idx].y;
        }
        d_out[idx] = result;
        idx += gridDim.x * blockDim.x;
    }
}

/*
  convert int16_t data vector into single precision floating point (original range is -32768 to 32767)
  also apply window function *w
 */
__global__ void short_to_float_s(int16_t *ds, float *df, float *w, int n_spectra, int spectrum_length)
{
  for(int spec_idx=blockIdx.x; spec_idx < n_spectra ; spec_idx+=N_BLOCKS_S)
  {
    for(int freq_idx=threadIdx.x; freq_idx < spectrum_length ; freq_idx+=N_THREADS_S)
    {
      //map signed shorts into range [-0.5,0.5)
      int idx=spec_idx*spectrum_length + freq_idx;
      df[idx] = w[freq_idx]*( (float)ds[idx] ) / 65535.0;
    }
  }
}

/*
  convert uint16_t data vector into single precision floating point (original range is 0 to 65535)
  also apply window function *w
 */
__global__ void short_to_float(uint16_t *ds, float *df, float *w, int n_spectra, int spectrum_length)
{
  for(int spec_idx=blockIdx.x; spec_idx < n_spectra ; spec_idx+=N_BLOCKS)
  {
    for(int freq_idx=threadIdx.x; freq_idx < spectrum_length ; freq_idx+=N_THREADS)
    {
      int idx=spec_idx*spectrum_length + freq_idx;
      //map unsigned shorts in to range [-0.5,0.5)
      df[idx] = w[freq_idx]*( (float)ds[idx] - 32768.0) / 65535.0;
    }
  }
}

/*
 transform and average signed data
*/

extern "C" void process_vector_no_output_s(int16_t *d_in, spectrometer_data_s *d)
{
    int n_spectra, data_length, spectrum_length;

    n_spectra=d->n_spectra;
    data_length=d->data_length;
    spectrum_length=d->spectrum_length;


    // ensure empty device spectrum
    if (cudaMemsetAsync(d->d_spectrum, 0, sizeof(float)*(spectrum_length/2 + 1)) != cudaSuccess)
    {
      fprintf(stderr, "Cuda error: Failed to zero device spectrum\n");
        exit(EXIT_FAILURE);
    }

    // copy mem to device
    if (cudaMemcpyAsync(d->ds_in, d_in, sizeof(int16_t)*data_length, cudaMemcpyHostToDevice) != cudaSuccess)
    {
      fprintf(stderr, "Cuda error: Memory copy failed\n");
      exit(EXIT_FAILURE);
    }

    // convert datatype using GPU
    short_to_float_s<<< N_BLOCKS_S, N_THREADS_S >>>(d->ds_in, d->d_in, d->d_window, n_spectra, spectrum_length);

    // cufft kernel execution
    if (cufftExecR2C(d->plan, (float *)d->d_in, (cufftComplex *)d->d_z_out)
	!= CUFFT_SUCCESS)
    {
      fprintf(stderr, "CUFFT error: ExecC2C Forward failed\n");
      exit(EXIT_FAILURE);
    }

    // this needs to be faster:
    square_and_accumulate_sum<<< 1, N_THREADS_S >>>(d->d_z_out, d->d_spectrum, n_spectra, spectrum_length/2+1);
    if (cudaGetLastError() != cudaSuccess) {
       fprintf(stderr, "Cuda error: Kernel failure, square_and_accumulate_sum\n");
       exit(EXIT_FAILURE);
    }

    // copying device resultant spectrum to host, now able to be manipulated
    if (cudaMemcpy(d->spectrum, d->d_spectrum, sizeof(float) * spectrum_length/2,
        cudaMemcpyDeviceToHost) != cudaSuccess)
    {
        fprintf(stderr, "Cuda error: Memory copy failed, spectrum DtH\n");
        exit(EXIT_FAILURE);
    }

}


/*
 transform and average unsigned data
*/

extern "C" void process_vector_no_output(uint16_t *d_in, spectrometer_data *d)
{
    int n_spectra, data_length, spectrum_length;

    n_spectra=d->n_spectra;
    data_length=d->data_length;
    spectrum_length=d->spectrum_length;


    // ensure empty device spectrum
    if (cudaMemset(d->d_spectrum, 0, sizeof(float)*(spectrum_length/2 + 1)) != cudaSuccess)
    {
      fprintf(stderr, "Cuda error: Failed to zero device spectrum\n");
        exit(EXIT_FAILURE);
    }

    // copy mem to device
    if (cudaMemcpy(d->ds_in, d_in, sizeof(uint16_t)*data_length, cudaMemcpyHostToDevice) != cudaSuccess)
    {
      fprintf(stderr, "Cuda error: Memory copy failed: data len = %d\n", data_length);
      exit(EXIT_FAILURE);
    }

    // // convert datatype using GPU
    short_to_float<<< N_BLOCKS_S, N_THREADS_S >>>(d->ds_in, d->d_in, d->d_window, n_spectra, spectrum_length);

    // cufft kernel execution
    if (cufftExecR2C(d->plan, (float *)d->d_in, (cufftComplex *)d->d_z_out)
	!= CUFFT_SUCCESS)
    {
      fprintf(stderr, "CUFFT error: ExecC2C Forward failed\n");
      exit(EXIT_FAILURE);
    }

    // this needs to be faster:
    square_and_accumulate_sum<<< 1, N_THREADS_S >>>(d->d_z_out, d->d_spectrum, n_spectra, spectrum_length/2+1);
    if (cudaGetLastError() != cudaSuccess) {
       fprintf(stderr, "Cuda error: Kernel failure, square_and_accumulate_sum\n");
       exit(EXIT_FAILURE);
    }

    // copying device resultant spectrum to host, now able to be manipulated
    if (cudaMemcpy(d->spectrum, d->d_spectrum, sizeof(float) * spectrum_length/2,
        cudaMemcpyDeviceToHost) != cudaSuccess)
    {
        fprintf(stderr, "Cuda error: Memory copy failed, spectrum DtH\n");
        exit(EXIT_FAILURE);
    }

}
