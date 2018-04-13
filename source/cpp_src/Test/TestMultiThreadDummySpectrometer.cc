#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <thread>
#include <unistd.h>

#include "HDummyDigitizer.hh"
#include "HBufferPool.hh"
#include "HSpectrometerCUDA.hh"
#include "HCudaHostBufferAllocator.hh"
#include "HBufferAllocatorSpectrometerDataCUDA.hh"

using namespace hose;

using PoolType = HBufferPool< uint16_t >;



int main(int /*argc*/, char** /*argv*/)
{
    size_t vector_length = SPECTRUM_LENGTH_S*128;
    size_t nAcq = 1;
    unsigned int n_dropped = 0;

    //dummy digitizer
    HDummyDigitizer< uint16_t > dummy;
    dummy.Initialize();

    //create source buffer pool
    HCudaHostBufferAllocator< uint16_t >* balloc = new HCudaHostBufferAllocator<  uint16_t >();
    HBufferPool< uint16_t >* source_pool = new HBufferPool< uint16_t >( balloc );

    const size_t source_n_chunks = 10;
    const size_t source_items_per_chunk = vector_length;
    source_pool->Allocate(source_n_chunks, source_items_per_chunk);
    dummy.SetBufferPool(source_pool);

    //create sink buffer pool
    HBufferAllocatorSpectrometerDataCUDA< spectrometer_data >* sdata_alloc = new HBufferAllocatorSpectrometerDataCUDA<spectrometer_data>();
    sdata_alloc->SetSampleArrayLength(vector_length);
    sdata_alloc->SetSpectrumLength(SPECTRUM_LENGTH);

    HBufferPool< spectrometer_data >* sink_pool = new HBufferPool< spectrometer_data >( sdata_alloc );
    const size_t sink_n_chunks = 11;
    const size_t sink_items_per_chunk = 1; //THERE CAN BE ONLY ONE!!!
    sink_pool->Allocate(sink_n_chunks, sink_items_per_chunk);

    HSpectrometerCUDA m_spec;
    m_spec.SetNThreads(1);

    m_spec.SetSourceBufferPool(source_pool);
    m_spec.SetSinkBufferPool(sink_pool);

    std::cout<<"starting"<<std::endl;
    m_spec.StartConsumptionProduction();
    dummy.StartProduction();

    //wait 
    usleep(1000000); //0.1 sec

    std::cout<<"stopping"<<std::endl;
    dummy.StopProduction();
    m_spec.StopConsumptionProduction();

    sleep(2);

    delete source_pool;
    delete sink_pool;
    delete balloc;
    delete sdata_alloc;

    return 0;
}
