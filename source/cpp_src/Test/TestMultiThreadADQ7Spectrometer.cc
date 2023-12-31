#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <thread>
#include <unistd.h>

#include "HADQ7Digitizer.hh"
#include "HBufferPool.hh"
#include "HSpectrometerCUDA.hh"
#include "HCudaHostBufferAllocator.hh"
#include "HBufferAllocatorSpectrometerDataCUDA.hh"
#include "HSimpleMultiThreadedSpectrumDataWriter.hh"

using namespace hose;

#define FAKE_SPECTRUM_LENGTH 131072

using PoolType = HBufferPool< SAMPLE_TYPE >;

int main(int /*argc*/, char** /*argv*/)
{
    size_t n_ave = 256;
    size_t vector_length = FAKE_SPECTRUM_LENGTH*n_ave;
    // size_t vector_length = 1024*n_ave;
    size_t nAcq = 1;
    unsigned int n_dropped = 0;

    //digitizer
    HADQ7Digitizer dummy;
    dummy.SetNThreads(2);
    bool initval = dummy.Initialize();

    std::cout<<"initval = "<<initval<<std::endl;
    std::cout<<"done card init"<<std::endl;

    std::cout<<"allocating cuda host buffs"<<std::endl;
    //create source buffer pool
    HCudaHostBufferAllocator< SAMPLE_TYPE >* balloc = new HCudaHostBufferAllocator<  SAMPLE_TYPE >();
    HBufferPool< SAMPLE_TYPE >* source_pool = new HBufferPool< SAMPLE_TYPE >( balloc );

    const size_t source_n_chunks = 32;
    const size_t source_items_per_chunk = vector_length;
    source_pool->Allocate(source_n_chunks, source_items_per_chunk);
    std::cout<<"done"<<std::endl;

    dummy.SetBufferPool(source_pool);

    std::cout<<"allocating cuda dev buffs"<<std::endl;
    //create sink buffer pool
    HBufferAllocatorSpectrometerDataCUDA< spectrometer_data >* sdata_alloc = new HBufferAllocatorSpectrometerDataCUDA< spectrometer_data >();
    sdata_alloc->SetSampleArrayLength(vector_length);
    sdata_alloc->SetSpectrumLength(FAKE_SPECTRUM_LENGTH);

    HBufferPool< spectrometer_data >* sink_pool = new HBufferPool< spectrometer_data >( sdata_alloc );
    const size_t sink_n_chunks = 16;
    const size_t sink_items_per_chunk = 1; //THERE CAN BE ONLY ONE!!!
    sink_pool->Allocate(sink_n_chunks, sink_items_per_chunk);
    std::cout<<"done"<<std::endl;

    HSpectrometerCUDA m_spec(FAKE_SPECTRUM_LENGTH, n_ave);
    m_spec.SetNThreads(3);
    m_spec.SetSourceBufferPool(source_pool);
    m_spec.SetSinkBufferPool(sink_pool);

    std::cout<<"sampling freq = "<<dummy.GetSamplingFrequency()<<std::endl;
    // m_spec.SetSamplingFrequency( dummy.GetSamplingFrequency() );
    // m_spec.SetSwitchingFrequency( 80.0 );
    // m_spec.SetBlankingPeriod( 20.0*(1.0/dummy.GetSamplingFrequency()) );

    //file writing consumer to drain the spectrum data buffers
    HSimpleMultiThreadedSpectrumDataWriter spec_writer;
    spec_writer.SetExperimentName("test");
    spec_writer.SetSourceName("none");
    spec_writer.SetScanName("test1");
    spec_writer.InitializeOutputDirectory();
    spec_writer.SetBufferPool(sink_pool);
    spec_writer.SetNThreads(1);


    std::cout<<"starting"<<std::endl;
    spec_writer.StartConsumption();

    for(unsigned int i=0; i<1; i++)
    {
        spec_writer.AssociateThreadWithSingleProcessor(i, i+6);
    };

    m_spec.StartConsumptionProduction();
    for(unsigned int i=0; i<3; i++)
    {
        m_spec.AssociateThreadWithSingleProcessor(i, i+2);
    };

    dummy.StartProduction();
    dummy.Acquire();


    //repeatedly stop and start acquisition to mimic multiple recordings
    // for(unsigned int i=0; i<1000; i++)
    // {
    //     //wait
    //     sleep(i%2 +1);
    //     std::cout<<"stopping acquire"<<std::endl;
    //     dummy.StopAfterNextBuffer();
    //     sleep(1);
    //     std::cout<<"restarting acquire"<<std::endl;
    //     dummy.Acquire();
    // }

    std::cout<<"sleeping for 60 sec"<<std::endl;
    sleep(60);
    std::cout<<"stopping digitizer"<<std::endl;
    dummy.StopAfterNextBuffer();
    dummy.StopProduction();

    std::cout<<"stopping spec"<<std::endl;
    m_spec.StopConsumptionProduction();

    sleep(1);

    std::cout<<"stopping writer"<<std::endl;
    spec_writer.StopConsumption();

    sleep(2);

    delete source_pool;
    delete sink_pool;
    delete balloc;
    delete sdata_alloc;

    return 0;
}
