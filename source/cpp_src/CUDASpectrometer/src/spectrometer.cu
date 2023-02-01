/* Project that takes signal and calculates averaged power spectrum using NVIDIA
   CUDA GPGPU programming.

   Juha Vierinen (x@mit.edu)
   Cory Cotter (optimization of square and accumulate sum)
   John Barrett (added support for signed/unsigned int, stripped down code to minimal library) 2019
   John Barrett (added noise power calculation, re-org to clean up code) 2021
*/


#include "spectrometer.h"
#include "noise_statistics_mbp_reduce.h"


#define BOXCAR_WIN 0
#define BLACKMAN_HARRIS_WIN 1
#define HANN_WIN 2

/*
  create box-car (effectively no) window function
 */
void boxcar(float* pOut, unsigned int num)
{

  unsigned int idx    = 0;
  while( idx < num )
  {
    pOut[idx] = 1.0;
    idx++;
  }
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
  create a Hann window function
 */
void hann_window(float* pOut, unsigned int num)
{
  unsigned int idx    = 0;
  while( idx < num )
  {
    pOut[idx] = 0.5 + 0.5*( cosf( (2.0f * M_PI * idx) / (num - 1) ) );
    idx++;
  }
}

/*
create a Hamming window function
*/
void hamming_window(float* pOut, unsigned int num)
{
    unsigned int idx    = 0;
    while( idx < num )
    {
      pOut[idx] = 0.53836 + 0.46164*( cosf( (2.0f * M_PI * idx) / (num - 1) ) );
      idx++;
    }
}



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

void wrapped_cuda_malloc(void** ptr, size_t s)
{
    int code =cudaMalloc(ptr, s);
    if( code != cudaSuccess)
    {
        printf("code = %d\n", code);
        fprintf(stderr, "Cuda error: Failed to allocate input data vector\n");
        exit(EXIT_FAILURE);
    }
}

void print_cuda_meminfo()
{
    unsigned long available, total;
    cudaMemGetInfo(&available, &total);
    printf("gpu mem: avail and total = %lu and %lu\n", available, total);
}

void wrapped_cuda_free(void* ptr)
{
    if (cudaFree(ptr) != cudaSuccess)
    {
        fprintf(stderr, "Cuda error: Failed to free input\n");
        exit(EXIT_FAILURE);
    }
}

extern "C" spectrometer_data *new_spectrometer_data(int data_length, int spectrum_length, int window_flag=BLACKMAN_HARRIS_WIN)
{
    spectrometer_data *d;
    int n_spectra;
    n_spectra = data_length/spectrum_length;
    print_cuda_meminfo();

    d = (spectrometer_data *) malloc(sizeof(spectrometer_data));

    // result on the cpu
    d->spectrum = (float*) malloc( (spectrum_length/2+1) * sizeof(float));
    d->data_length = data_length;
    d->spectrum_length = spectrum_length;
    d->n_spectra = n_spectra;
    d->window = (float*) malloc(spectrum_length*sizeof(float));

    if(window_flag == BOXCAR_WIN)
    {
        boxcar(d->window,spectrum_length);
    }

    if(window_flag == BLACKMAN_HARRIS_WIN)
    {
        blackmann_harris(d->window,spectrum_length);
    }
    
    if(window_flag == HANN_WIN)
    {
        hann_window(d->window,spectrum_length);
    }

    // allocating device memory to the above pointers
    // reserve extra for +1 in place transforms
    unsigned long want =  sizeof(cufftComplex)*n_spectra*(spectrum_length/2 + 1);
    printf("required gpu buff size = %lu \n", want);

    //allocate space for FFT input
    wrapped_cuda_malloc( (void **) &d->d_in, sizeof(cufftComplex)*n_spectra*(spectrum_length/2 + 1) );

    print_cuda_meminfo();
    //space for the noise power data accumulation
    wrapped_cuda_malloc( (void **) &d->d_out, sizeof(float)*N_THREADS );
    wrapped_cuda_malloc( (void **) &d->d_out2, sizeof(float)*N_THREADS );
    wrapped_cuda_malloc( (void **) &d->f_out, sizeof(float) );
    wrapped_cuda_malloc( (void **) &d->f_out2, sizeof(float) );
    print_cuda_meminfo();

    //allocate space for the x-formed spectra output
    // in-place seems to have a bug that causes the end to be garbled.
    //  d->d_z_out =(cufftComplex *) d->d_in;
    wrapped_cuda_malloc( (void **) &d->d_z_out, sizeof(cufftComplex)*n_spectra*(spectrum_length/2 + 1));
    print_cuda_meminfo();

    //allocate space for BH-window weights
    //note that we do not need multiple copies of the window weights
    //(we could replace this with pointer to a shared buffer)
    wrapped_cuda_malloc( (void **) &d->d_window, sizeof(float)*spectrum_length);
    print_cuda_meminfo();

    //allocate space to store the digitizer samples
    wrapped_cuda_malloc( (void **) &d->ds_in,sizeof(SAMPLE_TYPE)*data_length );
    print_cuda_meminfo();

    //allocate space for the power spectrum
    wrapped_cuda_malloc( (void **) &d->d_spectrum,sizeof(float)*(spectrum_length/2+1) );
    print_cuda_meminfo();

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
        fprintf(stderr, "Cuda error: Memory copy failed, window function: host to device.\n");
        exit(EXIT_FAILURE);
    }
    return(d);
}

extern "C" void free_spectrometer_data(spectrometer_data *d)
{
    free(d->window);
    free(d->spectrum);
    wrapped_cuda_free(d->d_in);
    wrapped_cuda_free(d->d_window);
    wrapped_cuda_free(d->d_z_out);
    wrapped_cuda_free(d->ds_in);
    wrapped_cuda_free(d->d_out);
    wrapped_cuda_free(d->d_out2);
    wrapped_cuda_free(d->f_out);
    wrapped_cuda_free(d->f_out2);
    wrapped_cuda_free(d->d_spectrum);

    if (cufftDestroy(d->plan) != CUFFT_SUCCESS)
    {
        fprintf(stderr, "CUFFT error: Failed to destroy plan\n");
        exit(EXIT_FAILURE);
    }

    free(d);
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
__global__ void short_to_float_s(int16_t *ds, float *df, int n_spectra, int spectrum_length)
{
    for(int spec_idx=blockIdx.x; spec_idx < n_spectra ; spec_idx+=N_BLOCKS)
    {
        for(int freq_idx=threadIdx.x; freq_idx < spectrum_length ; freq_idx+=N_THREADS)
        {
            //map signed shorts into range [-0.5,0.5)
            //ADQ7DC range is 1Vpp (-0.5V to +0.5V)
            int idx=spec_idx*spectrum_length + freq_idx;
            df[idx] = ( (float)ds[idx] ) / 65535.0;
        }
    }
}

/*
  convert uint16_t data vector into single precision floating point (original range is 0 to 65535)
  also apply window function *w
 */
__global__ void short_to_float(uint16_t *ds, float *df, int n_spectra, int spectrum_length)
{
    for(int spec_idx=blockIdx.x; spec_idx < n_spectra ; spec_idx+=N_BLOCKS)
    {
        for(int freq_idx=threadIdx.x; freq_idx < spectrum_length ; freq_idx+=N_THREADS)
        {
            int idx=spec_idx*spectrum_length + freq_idx;
            //map unsigned shorts in to range [-0.5,0.5)
            df[idx] = ( (float)ds[idx] - 32768.0) / 65535.0;
        }
    }
}


__global__ void apply_weights(float *df, float *w, int n_spectra, int spectrum_length)
{
    for(int spec_idx=blockIdx.x; spec_idx < n_spectra ; spec_idx+=N_BLOCKS)
    {
        for(int freq_idx=threadIdx.x; freq_idx < spectrum_length ; freq_idx+=N_THREADS)
        {
            int idx=spec_idx*spectrum_length + freq_idx;
            df[idx] *= w[freq_idx];
        }
    }
}


/*
 transform and average signed data
*/

void process_vector_no_output(SAMPLE_TYPE *d_in, spectrometer_data *d)
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

    // ensure empty noise data acuumulation buffers
    if (cudaMemsetAsync(d->d_out, 0, sizeof(float)*N_THREADS) != cudaSuccess)
    {
      fprintf(stderr, "Cuda error: Failed to zero device spectrum\n");
        exit(EXIT_FAILURE);
    }

    // ensure empty noise data acuumulation buffers
    if (cudaMemsetAsync(d->d_out2, 0, sizeof(float)*N_THREADS) != cudaSuccess)
    {
      fprintf(stderr, "Cuda error: Failed to zero device spectrum\n");
        exit(EXIT_FAILURE);
    }

    // copy mem to device
    if (cudaMemcpyAsync(d->ds_in, d_in, sizeof(SAMPLE_TYPE)*data_length, cudaMemcpyHostToDevice) != cudaSuccess)
    {
      fprintf(stderr, "Cuda error: Memory copy failed\n");
      exit(EXIT_FAILURE);
    }

    //convert datatype using GPU
    #ifdef HOSE_USE_ADQ7
    //convert signed ints to floats
    short_to_float_s<<< N_BLOCKS, N_THREADS >>>(d->ds_in, d->d_in, n_spectra, spectrum_length);
    #else
    //convert unsigned ints to floats
    short_to_float<<< N_BLOCKS, N_THREADS >>>(d->ds_in, d->d_in, n_spectra, spectrum_length);
    #endif

    //do the first pass parallel reduction of the data for the noise statistics
    cuda_noise_statistics_mbp_reduce1<<< N_BLOCKS, N_THREADS>>>(d->d_in, d->d_out, d->d_out2, data_length);

    //do the second pass parallel reduction of the data for the noise statistics
    cuda_noise_statistics_mbp_reduce2<<<1, N_THREADS>>>(d->d_out, d->d_out2, d->f_out, d->f_out2, N_THREADS);

    cudaDeviceSynchronize();

    //copy the <x> and <x^2> values back to the host
    cudaMemcpy(&d->sum, d->f_out, sizeof(float), cudaMemcpyDeviceToHost );
    cudaMemcpy(&d->sum2, d->f_out2, sizeof(float), cudaMemcpyDeviceToHost );


    //have to apply the Blackman-Harris window function to the data now
    apply_weights<<< N_BLOCKS, N_THREADS >>>(d->d_in, d->d_window, n_spectra, spectrum_length);

    // cufft kernel execution
    if (cufftExecR2C(d->plan, (float *)d->d_in, (cufftComplex *)d->d_z_out)	!= CUFFT_SUCCESS)
    {
        fprintf(stderr, "CUFFT error: ExecC2C Forward failed\n");
        exit(EXIT_FAILURE);
    }

    // this needs to be faster:
    square_and_accumulate_sum<<< 1, N_THREADS >>>(d->d_z_out, d->d_spectrum, n_spectra, spectrum_length/2+1);
    if (cudaGetLastError() != cudaSuccess)
    {
        fprintf(stderr, "Cuda error: Kernel failure, square_and_accumulate_sum\n");
        exit(EXIT_FAILURE);
    }

    // copying device resultant spectrum to host, now able to be manipulated
    if (cudaMemcpy(d->spectrum, d->d_spectrum, sizeof(float) * spectrum_length/2, cudaMemcpyDeviceToHost) != cudaSuccess)
    {
        fprintf(stderr, "Cuda error: Memory copy failed, spectrum DtH\n");
        exit(EXIT_FAILURE);
    }

}
