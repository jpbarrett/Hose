#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <thread>
#include <unistd.h>

#include "HDummyDigitizer.hh"
#include "HBufferPool.hh"
#include "HSpectrometerCUDASigned.hh"
#include "HCudaHostBufferAllocator.hh"

using namespace hose;

using PoolType = HBufferPool< short >;
uint64_t vector_length = SPECTRUM_LENGTH_S*128;
uint64_t nAcq = 1;
unsigned int n_dropped = 0;

int main(int /*argc*/, char** /*argv*/)
{

    //dummy digitizer
    HDummyDigitizer< short > dummy;
    dummy.Initialize();

    //create pool buffer
    //create buffer pool
    HCudaHostBufferAllocator< short >* balloc = new HCudaHostBufferAllocator<  short >();
    HBufferPool< short >* pool = new HBufferPool< short >( balloc );

    //allocate space
    const size_t n_chunks = 10;
    const size_t items_per_chunk = vector_length;
    pool->Allocate(n_chunks, items_per_chunk);
    dummy.SetBufferPool(pool);

    HSpectrometerCUDASigned m_spec;
    m_spec.SetDataLength(items_per_chunk);
    std::cout<<"data install dir = "<<DATA_INSTALL_DIR<<std::endl;
    m_spec.SetOutputDirectory(DATA_INSTALL_DIR);
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
