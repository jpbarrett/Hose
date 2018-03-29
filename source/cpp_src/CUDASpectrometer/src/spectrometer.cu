/* Project that takes signal and calculates averaged power spectrum using NVIDIA
   CUDA GPGPU programming. 

   Function main() tests empty data sets to check if the kernels 
   (GPU-based algorithms), allocations, and data transfers are working properly.

   The central power of this program is from the kernels below tied together nicely
   in the process_vector() function.

   periodically wake up, 
   scan the directory, 
   read in data from file, 
   and process...

   Juha Vierinen (x@mit.edu)
   Cory Cotter (optimization of square and accumulate sum)
*/
#include "spectrometer.h"
#define MIN(X,Y) (((X) < (Y)) ? (X) : (Y))

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
  convert uint16_t data vector into single precision floating point
  also apply window function *w
 */
__global__ void short_to_float(uint16_t *ds, float *df, float *w, int n_spectra, int spectrum_length)
{
  for(int spec_idx=blockIdx.x; spec_idx < n_spectra ; spec_idx+=N_BLOCKS)
  {
    for(int freq_idx=threadIdx.x; freq_idx < spectrum_length ; freq_idx+=N_THREADS)
    {
      int idx=spec_idx*spectrum_length + freq_idx;
      df[idx] = w[freq_idx]*((float)ds[idx]-32768.0)/65532.0;
    }
  }
}

spectrometer_output *new_spectrometer_output()
{
  spectrometer_output *o;
  o=(spectrometer_output *)malloc(sizeof(spectrometer_output));
  o->out=NULL;
  o->ut_sec=0;
  o->year=0;
  o->month=0;
  o->day=0;
  return(o);
}

int cmpfunc(const void *a, const void *b)
{
  if( (*(int64_t *)a - *(int64_t *)b) < 0)
    return(-1);
  if( (*(int64_t *)a - *(int64_t *)b) > 0)
    return(1);
  else
    return(0);
}

/* 
   figure out how many files in /ram
   
 */
void free_files(spectrometer_files *f)
{
  free(f->file_numbers);
  free(f);
}

spectrometer_files *get_files(char const *dname)
{
  DIR *d;
  spectrometer_files *f;
  struct dirent *dir;
  struct stat st;
  char fname[4096];
  f=(spectrometer_files *)malloc(sizeof(spectrometer_files));
  int size;
  int fsize;
  int n_files;
  uint64_t number;
  n_files=0;
  // out-586888628755720000.bin
  fsize=0;
  d=opendir(dname);
  if(d)
  {
    while((dir=readdir(d)) != NULL)
    {
      if(strlen(dir->d_name) == 26 && dir->d_name[0]=='o')
      {
	sprintf(fname,"%s/%s",dname,dir->d_name);
	stat(fname,&st);
	size=st.st_size;
	if(size > fsize)
	{
	  fsize=size;
	}
	n_files++;
      }
    }
    closedir(d);
  }

  f->n_files=n_files;
  f->file_size=fsize;
  // there might be some new files, pad for extra room
  f->file_numbers=(uint64_t *)malloc(sizeof(uint64_t)*(n_files+1000));
  d=opendir(dname);
  n_files=0;
  if(d)
  {
    while((dir=readdir(d)) != NULL)
    {
      if(strlen(dir->d_name) == 26 && dir->d_name[0]=='o' && n_files < f->n_files)
      {
	sprintf(fname,"%s/%s",dname,dir->d_name);
	stat(fname,&st);
	size=st.st_size;
	if(size==fsize)
	{
	  sscanf(dir->d_name,"out-%"PRIu64".bin",&number);
	  f->file_numbers[n_files]=number;
	  n_files++;
	}
      }
    }
    closedir(d);
  }
  f->n_files=n_files;
  if(f->n_files > 1)
  {
    qsort(f->file_numbers, f->n_files, sizeof(uint64_t), cmpfunc);
  }

  return(f);
}
/*
      {
	printf("reading data %d\n",size/2);
	data_length=size/2;
	n_spectra=data_length/spectrum_length;
	r_in = (uint16_t *)malloc(data_length*sizeof(uint16_t));
	in=(FILE *)fopen(argv[1],"r");
	fread(r_in,sizeof(uint16_t),data_length,in);
	fclose(in);
      }
      else
	exit(0);
    }else{
*/

void day_dirname(spectrometer_output *o, char const *prefix)
{
  struct tm dt;
  time_t t;
  char cmd[4096];
  int year, month, day;

  time(&t);
  gmtime_r(&t, &dt);
  year=dt.tm_year+1900;
  month=dt.tm_mon+1;
  day=dt.tm_mday;
  sprintf(o->dname,"%s/%d.%02d.%02d",prefix, year, month, day);
  if(o->year != year || o->day != day || o->month != month)
  {
    // turns out this operation is _super_ slow. avoid doing it too often.
    sprintf(cmd,"mkdir -p %s",o->dname);
    printf("creating directory %s\n",o->dname);
    system(cmd);
  }
  o->year=year;
  o->month=month;
  o->day=day;
}

void gen_test_signal(uint16_t *r_in, int spectrum_length, int n_spectra)
{
  // test signal
  for(int ti=0; ti<spectrum_length; ti++)
    r_in[ti]=(uint16_t) (sinf(2.0*M_PI*10.0*(float)ti/((float)spectrum_length))*256.0 + 32768.0 );
  
  for(int i=1; i<n_spectra; i++)
    for(int ti=0; ti<spectrum_length; ti++) 
      r_in[i*spectrum_length + ti]=r_in[ti];

#ifdef WRITE_INPUT 
  FILE *out;
  out=fopen("in.bin","w");
  fwrite(r_in,sizeof(uint16_t),10.0*spectrum_length,out);
  fclose(out);
#endif

}

/*
  TODO: this could be double buffered to increase speed:
  - read file in background, while processing the previous one
  - could be done with multithreading
 */
int process(char const *dname, char const *dname_results)
{
    // 32k transform
    int spectrum_length = SPECTRUM_LENGTH;
    //int n_spectra;//=2440; // Limited by the amount of RAM on the GPU.
    int data_length;// = n_spectra*spectrum_length;
    spectrometer_files *f;
    char fname[4096];
    spectrometer_data *d;
    spectrometer_output *o;
    uint16_t *r_in;
    FILE *in;

    f=get_files(dname);
    if(f->n_files == 0 || f->file_size==0)
    {
      printf("waiting for more data...\n");
      sleep(1);
      return(0);
    }

    data_length=f->file_size/2-BLANKING;
    //n_spectra=data_length/spectrum_length;

    // real valued input signal as short ints
    r_in = (uint16_t *)malloc(data_length*sizeof(uint16_t));

#ifdef TEST_SIGNAL
    gen_test_signal(r_in, spectrum_length, n_spectra);
#endif

    d=new_spectrometer_data(data_length,spectrum_length);
    o=new_spectrometer_output();
    
    for(int i=0; i < f->n_files; i++)
    {
#ifdef TIMING_PRINT
      clock_t start, end;
      clock_t r_end;
      start=clock();
#endif
      sprintf(fname,"%s/out-%"PRIu64".bin",dname, f->file_numbers[i]);
      
      if(in=fopen(fname,"r"))
      {
	// blanking, 2 bytes per sample
	fseek(in,BLANKING*2,SEEK_SET);
	size_t rsize=fread(r_in,sizeof(uint16_t),data_length,in);
#ifdef TIMING_PRINT
	r_end=clock();
#endif
	if(rsize == data_length)
	  process_vector(r_in, d, f->file_numbers[i], o, dname_results);	
	else
	  printf("couldn't read file\n");

	fclose(in);
      }
      else
	printf("couldn't open file %s\n", fname);

      // delete file
      sprintf(fname,"rm %s/out-%"PRIu64".bin", dname, f->file_numbers[i]);
      system(fname);
    
#ifdef TIMING_PRINT
      end=clock();
      double dt = ((double) (end-start))/CLOCKS_PER_SEC;
      double r_dt = ((double) (r_end-start))/CLOCKS_PER_SEC;
      
      if(i > 1){
	printf("Time elapsed %1.3f/%1.3f s read %1.3f%% speed ratio %1.3f t %"PRIu64" dt %"PRIu64"\n", dt, (double)(data_length+BLANKING)/SAMPLE_RATE,
	       (r_dt/dt)*100.0,
	       ((double)(data_length+BLANKING)/SAMPLE_RATE)  / dt / 2, //halve the speed ratio for two pol 
	       f->file_numbers[i],f->file_numbers[i]-f->file_numbers[i-1]);
      }
#endif
    }

    free_files(f);
    free_spectrometer_data(d);
    free(r_in);
    return 0;
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

/* This is the primary function in this program, meant to be embedded into other
**  programs. The transmit signal, tx, should be complex conjugated prior to use here.
**  The float types for tx and echo are useful when this function is embedded; the extern
**  "C" is also here for that purpose. Process_echoes sets up and runs the kernels on
**  the GPU, complex_mult, a 1D FFT, and a spectrum accumulation. Host spectrum is not
**  freed, so as to be taken and analyzed.
*/
//    process_vector((float *)z_in, data_length, spectrum, spectrum_length);



extern "C" void process_vector(uint16_t *d_in, spectrometer_data *d, uint64_t t0, spectrometer_output *o, char const *dname_results)
{
    int n_spectra, data_length, spectrum_length;
    //    FILE *out;
    uint64_t ut_sec;
    //    char fname[4096];
    //char dname[4096];

    n_spectra=d->n_spectra;
    data_length=d->data_length;
    spectrum_length=d->spectrum_length;

#ifdef DEBUG_Z_OUT
    // debug out
    cufftComplex *z_out = (cufftComplex *)malloc( n_spectra*(spectrum_length/2 + 1)*sizeof(cufftComplex));
#endif
    // ensure empty device spectrum
    if (cudaMemset(d->d_spectrum, 0, sizeof(float)*(spectrum_length/2 + 1)) != cudaSuccess)
    {
      fprintf(stderr, "Cuda error: Failed to zero device spectrum\n");
        exit(EXIT_FAILURE);
    }

    // copy mem to device
    if (cudaMemcpy(d->ds_in, d_in, sizeof(uint16_t)*data_length, cudaMemcpyHostToDevice) != cudaSuccess)
    {
      fprintf(stderr, "Cuda error: Memory copy failed, tx HtD\n");
      exit(EXIT_FAILURE);
    }
    
    // convert datatype using GPU
    short_to_float<<< N_BLOCKS, N_THREADS >>>(d->ds_in, d->d_in, d->d_window, n_spectra, spectrum_length);

    // cufft kernel execution
    if (cufftExecR2C(d->plan, (float *)d->d_in, (cufftComplex *)d->d_z_out)
	!= CUFFT_SUCCESS)
    {
      fprintf(stderr, "CUFFT error: ExecC2C Forward failed\n");
      exit(EXIT_FAILURE);
    }

    // copying device resultant spectrum to host, now able to be manipulated
    // debug 
#ifdef DEBUG_Z_OUT
    cudaMemcpy(z_out, d->d_z_out, sizeof(cufftComplex)*n_spectra*(spectrum_length/2 + 1 ), cudaMemcpyDeviceToHost);
    out=fopen("z_out.bin","w");
    fwrite(z_out,sizeof(cufftComplex),n_spectra*(spectrum_length/2 + 1),out);
    fclose(out);
#endif

    // this needs to be faster:
    square_and_accumulate_sum<<< 1, N_THREADS >>>(d->d_z_out, d->d_spectrum, n_spectra, spectrum_length/2+1);
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

    ut_sec=t0/SAMPLE_RATE;
    if(o->ut_sec != ut_sec || o->out == NULL)
    {
      // only make directory when necessary
      day_dirname(o, dname_results);
      // write results into one file per second
      sprintf(o->fname,"%s/spec-%"PRIu64".bin",o->dname, ut_sec);
      o->out=fopen(o->fname,"a");
      o->ut_sec=ut_sec;
    }
    fwrite(d->spectrum,sizeof(float),spectrum_length/2+1,o->out);
    fclose(o->out);
    o->out=NULL;

}
