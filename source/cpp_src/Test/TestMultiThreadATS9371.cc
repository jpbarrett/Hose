#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <thread>
#include <unistd.h>

#include "HATS9371Digitizer.hh"
#include "HBufferPool.hh"
#include "HBufferAllocatorNew.hh"
#include "HSimpleMultiThreadedWriter.hh"

using namespace hose;

int main(int /*argc*/, char** /*argv*/)
{
    //digi digitizer
    HATS9371Digitizer digi;
    digi.SetNThreads(1);
    digi.SetSidebandFlag('U');
    digi.SetPolarizationFlag('X');
    digi.Initialize();

    //create pool buffer
    HBufferAllocatorNew<HATS9371Digitizer::sample_type>* allocator = new HBufferAllocatorNew<HATS9371Digitizer::sample_type>();
    HBufferPool< HATS9371Digitizer::sample_type > pool( allocator );

    const size_t n_chunks = 100;
    const size_t items_per_chunk = 1024*1024;
    pool.Allocate(n_chunks, items_per_chunk);
    digi.SetBufferPool(&pool);

    HSimpleMultiThreadedWriter< HATS9371Digitizer::sample_type  > m_writer;
    m_writer.SetNThreads(1);
    m_writer.SetBufferPool(&pool);
    m_writer.StartConsumption();

    pool.Initialize();

    std::cout<<"starting acquisition"<<std::endl;
    digi.StartProduction();
    digi.Acquire();

    usleep(500000);

    std::cout<<"stopping acquisition"<<std::endl;
    digi.StopAfterNextBuffer();
    std::cout<<"sleeping for 0.5 sec"<<std::endl;
    usleep(500000);

    std::cout<<"tearing down card"<<std::endl;\
    digi.StopProduction();

    digi.TearDown();

    std::cout<<"stopping writer"<<std::endl;
    m_writer.StopConsumption();

    std::cout<<"finished"<<std::endl;

    return 0;
}
