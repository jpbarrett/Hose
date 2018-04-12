#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <thread>
#include <unistd.h>

#include "HADQ7Digitizer.hh"
#include "HBufferPool.hh"
#include "HSimpleMultiThreadedWriter.hh"

using namespace hose;

using PoolType = HBufferPool< HADQ7Digitizer::sample_type >;
uint64_t vector_length = 100000000;
uint64_t nAcq = 32;

void RunAcquireThread(HADQ7Digitizer* dummy, PoolType* pool, HSimpleMultiThreadedWriter< HADQ7Digitizer::sample_type >* writer)
{
    //do buffer aquisitions
    unsigned int count = 0;
    while(count < nAcq)
    {
        if(pool->GetProducerPoolSize() != 0)
        {
            HLinearBuffer< HADQ7Digitizer::sample_type >* buff = pool->PopProducerBuffer();
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
            HLinearBuffer< HADQ7Digitizer::sample_type >* buff = pool->PopConsumerBuffer();
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
};



int main(int /*argc*/, char** /*argv*/)
{
    //dummy digitizer
    HADQ7Digitizer dummy;
    bool initval = dummy.Initialize();

    //create pool buffer
    PoolType pool( dummy.GetAllocator() );

    //allocate space
    const size_t n_chunks = nAcq;
    const size_t items_per_chunk = vector_length;
    pool.Allocate(n_chunks, items_per_chunk);

    HSimpleMultiThreadedWriter< HADQ7Digitizer::sample_type > m_writer;

    std::thread acq(RunAcquireThread, &dummy, &pool, &m_writer);
    m_writer.SetNThreads(2);
    m_writer.SetBufferPool(&pool);
    m_writer.StartConsumption();
    
    acq.join();

    m_writer.StopConsumption();

    return 0;
}
