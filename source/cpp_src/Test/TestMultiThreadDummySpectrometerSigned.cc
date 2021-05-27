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

using namespace hose;

#define SPECTRUM_LENGTH 131072

using PoolType = HBufferPool< SAMPLE_TYPE >;
uint64_t vector_length = SPECTRUM_LENGTH*128;
uint64_t nAcq = 1;
unsigned int n_dropped = 0;

int main(int /*argc*/, char** /*argv*/)
{

    //dummy digitizer
    HDummyDigitizer< SAMPLE_TYPE > dummy;
    dummy.Initialize();

    //create pool buffer
    //create buffer pool
    HCudaHostBufferAllocator< SAMPLE_TYPE >* balloc = new HCudaHostBufferAllocator<  SAMPLE_TYPE >();
    HBufferPool< SAMPLE_TYPE >* pool = new HBufferPool< SAMPLE_TYPE >( balloc );

    //allocate space
    const size_t n_chunks = 10;
    const size_t items_per_chunk = vector_length;
    pool->Allocate(n_chunks, items_per_chunk);
    dummy.SetBufferPool(pool);

    HSpectrometerCUDA m_spec;
    m_spec.SetDataLength(items_per_chunk);
    std::cout<<"data install dir = "<<STR2(DATA_INSTALL_DIR)<<std::endl;
    m_spec.SetOutputDirectory(STR2(DATA_INSTALL_DIR));
    m_spec.SetNThreads(1);
    m_spec.SetBufferPool(pool);
    m_spec.LaunchThreads();

    dummy.StartProduction();

    //wait
    usleep(1000000); //0.1 sec

    dummy.StopProduction();

    m_spec.SignalTerminateOnComplete();
    m_spec.JoinThreads();

    delete pool;
    delete balloc;

    return 0;
}
