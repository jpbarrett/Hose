#include <stdio.h>
#include "spectrometer.h"
#include <cmath>
#include <random>
#include <inttypes.h>
#include <chrono>
#include <iostream>
int main(int argc, char **argv)
{
  printf("running a benchmark\n");
  int veclen=2097152*64;
  int fftlen=2097152;
  spectrometer_data* sd = new_spectrometer_data(veclen, fftlen, 0);
  int16_t *noise; 
  std::random_device rd{};
  std::mt19937 gen{rd()};
  
  
  std::normal_distribution d{0.0,1.0};
  auto random_short = [&d, &gen]{return std::round(512*d(gen));};
  
  noise=(int16_t *)malloc(sizeof(int16_t)*veclen);
  
  for(int i=0; i<veclen; i++)
  {
	noise[i]=random_short();
  }
  FILE *o=fopen("noise.bin","wb");
  fwrite(noise,sizeof(int16_t),veclen,o);
  fclose(o);

  process_vector_no_output_(noise, sd, 0);
  o=fopen("spec.bin","wb");  
  fwrite(sd->spectrum,sizeof(float),sd->spectrum_length/2+1,o);
  fclose(o);
  
  typedef std::chrono::high_resolution_clock Time;
  typedef std::chrono::milliseconds ms;
  typedef std::chrono::duration<float> fsec;
  auto t0 = Time::now();
  
  

  
  long n_reps=10;
  long n_samples = n_reps*veclen;
  for(int i=0; i<n_reps; i++)
  {
    process_vector_no_output_(noise, sd, 0);
  }

  
  auto t1 = Time::now();
  fsec fs = t1 - t0;
  ms td = std::chrono::duration_cast<ms>(fs);
  double samps_per_sec=(double)n_samples/fs.count();
  std::cout << samps_per_sec << "samps per second\n";
  std::cout << fs.count() << "s\n";
  std::cout << td.count() << "ms\n";
}
