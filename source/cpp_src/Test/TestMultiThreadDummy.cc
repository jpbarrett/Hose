#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <thread>
#include <unistd.h>

#include "HBufferAllocatorNew.hh"
#include "HDummyDigitizer.hh"
#include "HBufferPool.hh"
#include "HSimpleMultiThreadedWriter.hh"

using namespace hose;

#define DUMMY_SAMPLE_TYPE int

int main(int /*argc*/, char** /*argv*/)
{
    //dummy digitizer
    HDummyDigitizer< DUMMY_SAMPLE_TYPE > dummy;
    dummy.SetNThreads(2);
    dummy.Initialize();

    //create pool buffer
    HBufferPool< DUMMY_SAMPLE_TYPE > pool( dummy.GetAllocator() );

    //allocate space
    const size_t n_chunks = 3;
    const size_t items_per_chunk = 1024*1024;
    pool.Allocate(n_chunks, items_per_chunk);
    dummy.SetBufferPool(&pool);

    HSimpleMultiThreadedWriter< DUMMY_SAMPLE_TYPE  > m_writer;
    m_writer.SetNThreads(1);
    m_writer.SetBufferPool(&pool);

    std::cout<<"starting consumption"<<std::endl;
    m_writer.StartConsumption();

    std::cout<<"starting production"<<std::endl;
    dummy.StartProduction();
    usleep(100000); //0.1 sec

    std::cout<<"stopping"<<std::endl;
    dummy.StopProduction();

    m_writer.StopConsumption();

    return 0;
}
