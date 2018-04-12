#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <thread>
#include <unistd.h>

#include "HPX14Digitizer.hh"
#include "HBufferPool.hh"
#include "HSimpleMultiThreadedWriter.hh"

using namespace hose;

int main(int /*argc*/, char** /*argv*/)
{
    //px14 digitizer
    HPX14Digitizer px14;
    px14.Initialize();

    //create pool buffer
    HBufferPool< HPX14Digitizer::sample_type > pool( px14.GetAllocator() );

    //allocate space
    const size_t n_chunks = 100;
    const size_t items_per_chunk = 1024*1024;
    pool.Allocate(n_chunks, items_per_chunk);
    px14.SetBufferPool(&pool);

    HSimpleMultiThreadedWriter< HPX14Digitizer::sample_type  > m_writer;
    m_writer.SetNThreads(2);
    m_writer.SetBufferPool(&pool);
    m_writer.StartConsumption();

    px14.StartProduction();

    //wait 
    //usleep(5000);
    usleep(100000); //0.1 sec

    px14.StopProduction();


    m_writer.StopConsumption();

    return 0;
}
