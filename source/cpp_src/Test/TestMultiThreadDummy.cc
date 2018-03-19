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

using PoolType = HBufferPool< HDummyDigitizer::sample_type >;

void RunAcquireThread(HDummyDigitizer* dummy, PoolType* pool, HSimpleMultiThreadedWriter< HDummyDigitizer::sample_type >* writer)
{
    //do 15 buffer aquisitions
    unsigned int count = 0;
    while(count < 15)
    {
        if(pool->GetProducerPoolSize() != 0)
        {
            HLinearBuffer< HDummyDigitizer::sample_type >* buff = pool->PopProducerBuffer();
            if(buff != nullptr)
            {
                dummy->SetBuffer(buff);
                if(count == 0){ dummy->Acquire();};
                dummy->Transfer();
                dummy->Finalize();
                pool->PushConsumerBuffer(dummy->GetBuffer());
                count++;
            }
        }
        else
        {
            //steal a buffer from the consumer pool..however, this requires dropping
            //a previous aquisition, so we don't increment the count
            HLinearBuffer< HDummyDigitizer::sample_type >* buff = pool->PopConsumerBuffer();
            if(buff != nullptr)
            {
                dummy->SetBuffer(buff);
                dummy->Acquire();
                dummy->Transfer();
                dummy->Finalize();
                pool->PushConsumerBuffer(dummy->GetBuffer());
            }
        }
    }
    writer->SignalTerminateOnComplete();
};


int main(int /*argc*/, char** /*argv*/)
{
    //dummy digitizer
    HDummyDigitizer dummy;
    bool initval = dummy.Initialize();

    //create pool buffer
    PoolType pool( dummy.GetAllocator() );

    //allocate space
    const size_t n_chunks = 3;
    const size_t items_per_chunk = 1000;
    pool.Allocate(n_chunks, items_per_chunk);

    HSimpleMultiThreadedWriter< HDummyDigitizer::sample_type > m_writer;

    std::thread acq(RunAcquireThread, &dummy, &pool, &m_writer);
    m_writer.SetNThreads(5);
    m_writer.SetSleepMicroSeconds(1);
    m_writer.SetBufferPool(&pool);
    m_writer.LaunchThreads();

    acq.join();
    m_writer.JoinThreads();

    return 0;
}
