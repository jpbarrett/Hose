#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <thread>
#include <unistd.h>

#include "HDummyDigitizer.hh"
#include "HBufferPool.hh"

#include "HPX14DigitizerSimulator.hh"
#include "HSpectrometerCUDA.hh"
#include "HSpectrumAverager.hh"

#include "HSpectrometerCUDA.hh"
#include "HSimpleMultiThreadedSpectrumDataWriter.hh"

#define DIGITIZER_TYPE HPX14DigitizerSimulator
#define SPECTROMETER_TYPE HSpectrometerCUDA
#define SPECTRUM_TYPE spectrometer_data
#define AVERAGER_TYPE HSpectrumAverager
#define N_DIGITIZER_THREADS 1
#define N_DIGITIZER_POOL_SIZE 4
#define N_SPECTROMETER_POOL_SIZE 4
#define N_SPECTROMETER_THREADS 1
#define N_NOISE_POWER_POOL_SIZE 10
#define N_NOISE_POWER_THREADS 1
#define DUMP_FREQ 1
#define N_AVE_BUFFERS 2
#define SPEC_AVE_POOL_SIZE 4
#define FFT_SIZE 131072
#define SPECTRUM_LENGTH 131072
#define SAMPLE_TYPE uint16_t
#define N_AVERAGES 256


#include "HBufferPool.hh"
#include "HBufferAllocatorNew.hh"
#include "HCudaHostBufferAllocator.hh"
#include "HBufferAllocatorSpectrometerDataCUDA.hh"
#include "HSwitchedPowerCalculator.hh"
#include "HDataAccumulationWriter.hh"
#include "HSimpleMultiThreadedSpectrumDataWriter.hh"
#include "HAveragedMultiThreadedSpectrumDataWriter.hh"
#include "HRawDataDumper.hh"

using namespace hose;


using PoolType = HBufferPool< uint16_t >;

int main(int /*argc*/, char** /*argv*/)
{

    std::cout<<"Initializing..."<<std::endl;


    //objects
    DIGITIZER_TYPE* fDigitizer;
    HCudaHostBufferAllocator< SAMPLE_TYPE >* fCUDABufferAllocator;
    HBufferAllocatorSpectrometerDataCUDA< SPECTRUM_TYPE >* fSpectrometerBufferAllocator;
    SPECTROMETER_TYPE* fSpectrometer;
    HRawDataDumper< SAMPLE_TYPE >* fDumper;
    HBufferPool< SAMPLE_TYPE >* fDigitizerSourcePool;
    HBufferPool< SPECTRUM_TYPE >* fSpectrometerSinkPool;
    HBufferAllocatorNew< float >* fSpectrumAveragingBufferAllocator;
    HBufferPool< float >* fSpectrumAveragingBufferPool;
    AVERAGER_TYPE* fSpectrumAverager;
    HAveragedMultiThreadedSpectrumDataWriter* fAveragedSpectrumWriter;


    //create digitizer
    fDigitizer = new DIGITIZER_TYPE();
    fDigitizer->SetNThreads(N_DIGITIZER_THREADS);
    bool digitizer_init_success = fDigitizer->Initialize();

    if(digitizer_init_success)
    {
        //create source buffer pool
        fCUDABufferAllocator = new HCudaHostBufferAllocator< SAMPLE_TYPE >();
        fDigitizerSourcePool = new HBufferPool< SAMPLE_TYPE  >( fCUDABufferAllocator );
        fDigitizerSourcePool->Allocate(N_DIGITIZER_POOL_SIZE, N_AVERAGES*FFT_SIZE);
        fDigitizer->SetBufferPool(fDigitizerSourcePool);

        //TODO fill these in with real values!
        fDigitizer->SetSidebandFlag('U');
        fDigitizer->SetPolarizationFlag('X');

        //create spectrometer data pool
        fSpectrometerBufferAllocator = new HBufferAllocatorSpectrometerDataCUDA<SPECTRUM_TYPE>();
        fSpectrometerBufferAllocator->SetSampleArrayLength(N_AVERAGES*FFT_SIZE);
        fSpectrometerBufferAllocator->SetSpectrumLength(FFT_SIZE);
        fSpectrometerSinkPool = new HBufferPool< SPECTRUM_TYPE >( fSpectrometerBufferAllocator );
        fSpectrometerSinkPool->Allocate(N_SPECTROMETER_POOL_SIZE, 1);

        //create spectrometer
        fSpectrometer = new SPECTROMETER_TYPE(FFT_SIZE, N_AVERAGES);
        fSpectrometer->SetNThreads(N_SPECTROMETER_THREADS);
        fSpectrometer->SetSourceBufferPool(fDigitizerSourcePool);
        fSpectrometer->SetSinkBufferPool(fSpectrometerSinkPool);

        //create post-spectrometer data pool for averaging
        fSpectrumAveragingBufferAllocator = new HBufferAllocatorNew< float >();
        fSpectrumAveragingBufferPool = new HBufferPool< float >(fSpectrumAveragingBufferAllocator);
        fSpectrumAveragingBufferPool->Allocate(SPEC_AVE_POOL_SIZE, FFT_SIZE/2+1); //create a work space of buffers

        fSpectrumAverager = new AVERAGER_TYPE(FFT_SIZE/2+1, N_AVE_BUFFERS); //we average over 12 buffers
        fSpectrumAverager->SetNThreads(1); //ONE THREAD ONLY!
        fSpectrumAverager->SetSourceBufferPool(fSpectrometerSinkPool);
        fSpectrumAverager->SetSinkBufferPool(fSpectrumAveragingBufferPool);

        fAveragedSpectrumWriter = new HAveragedMultiThreadedSpectrumDataWriter();
        fAveragedSpectrumWriter->SetBufferPool(fSpectrumAveragingBufferPool);
        fAveragedSpectrumWriter->SetNThreads(1);

        //create an itermittent raw data dumper
        fDumper = new HRawDataDumper< SAMPLE_TYPE >();
        fDumper->SetBufferPool(fDigitizerSourcePool);
        fDumper->SetBufferDumpFrequency(DUMP_FREQ);
        fDumper->SetNThreads(1);

        fDigitizerSourcePool->Initialize();
        fSpectrometerSinkPool->Initialize();
        fSpectrumAveragingBufferPool->Initialize();

    }

    std::string expName = "Dummy";
    std::string sourceName = "Dummy";
    std::string scanName = "Dummy";


    fDumper->SetExperimentName(expName);
    fDumper->SetSourceName(sourceName);
    fDumper->SetScanName(scanName);
    fDumper->InitializeOutputDirectory();

    fAveragedSpectrumWriter->SetExperimentName(expName);
    fAveragedSpectrumWriter->SetSourceName(sourceName);
    fAveragedSpectrumWriter->SetScanName(scanName);
    fAveragedSpectrumWriter->InitializeOutputDirectory();


    fAveragedSpectrumWriter->StartConsumption();

    // NUMA node0 CPU(s):     0-7,16-23
    // NUMA node1 CPU(s):     8-15,24-31

    unsigned int core_id = 8;
    for(unsigned int i=0; i<1; i++)
    {
        fAveragedSpectrumWriter->AssociateThreadWithSingleProcessor(i, core_id++);
    };

    fDumper->StartConsumption();

    fSpectrumAverager->StartConsumptionProduction();
    for(unsigned int i=0; i<1; i++)
    {
        fSpectrumAverager->AssociateThreadWithSingleProcessor(i, core_id++);
    };

    fSpectrometer->StartConsumptionProduction();
    core_id = 24;
    for(size_t i=0; i<N_SPECTROMETER_THREADS; i++)
    {
        fSpectrometer->AssociateThreadWithSingleProcessor(i, core_id++);
    };

    fDigitizer->StartProduction();
    for(size_t i=0; i<N_DIGITIZER_THREADS; i++)
    {
        fDigitizer->AssociateThreadWithSingleProcessor(i, core_id++);
    };


    fDigitizer->Acquire();

    for(unsigned int i=0; i<10; i++)
    {
        std::cout<<"Est. time remaining: "<<1*(10-i)<<"s"<<std::endl;
        sleep(1);
    }

    fDigitizer->StopAfterNextBuffer();
    sleep(1);
    fSpectrometer->StopConsumptionProduction();
    sleep(1);
    fAveragedSpectrumWriter->StopConsumption();
    sleep(1);
    fDumper->StopConsumption();
    fSpectrumAverager->StopConsumptionProduction();
    fDigitizer->StopProduction();


    delete fDigitizer;
    delete fSpectrometer;
    delete fDumper;
    delete fSpectrumAverager;
    delete fAveragedSpectrumWriter;

    delete fDigitizerSourcePool;
    delete fSpectrometerSinkPool;
    delete fSpectrumAveragingBufferPool;

    delete fCUDABufferAllocator;
    delete fSpectrometerBufferAllocator;
    delete fSpectrumAveragingBufferAllocator;

    std::cout<<"Done."<<std::endl;

    return 0;
}
