#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <thread>
#include <unistd.h>

#include "HPX14Digitizer.hh"
#include "HBufferPool.hh"
#include "HSpectrometerCUDA.hh"

using namespace hose;

using PoolType = HBufferPool< HPX14Digitizer::sample_type >;
uint64_t vector_length = 2000000;
uint64_t nAcq = 100;

void RunAcquireThread(HPX14Digitizer* dummy, PoolType* pool, HSpectrometerCUDA* spec)
{
    //do 15 buffer aquisitions
    unsigned int count = 0;
    while(count < nAcq)
    {
        if(pool->GetProducerPoolSize() != 0)
        {
            HLinearBuffer< HPX14Digitizer::sample_type >* buff = pool->PopProducerBuffer();
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
            HLinearBuffer< HPX14Digitizer::sample_type >* buff = pool->PopConsumerBuffer();
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
    spec->SignalTerminateOnComplete();
};



int main(int /*argc*/, char** /*argv*/)
{
    //dummy digitizer
    HPX14Digitizer dummy;
    bool initval = dummy.Initialize();

    //create pool buffer
    PoolType pool( dummy.GetAllocator() );

    //allocate space
    const size_t n_chunks = nAcq;
    const size_t items_per_chunk = vector_length;
    pool.Allocate(n_chunks, items_per_chunk);

    HSpectrometerCUDA m_spec;

    std::thread acq(RunAcquireThread, &dummy, &pool, &m_spec);
    m_spec.SetNThreads(10);
    m_spec.SetSleepMicroSeconds(1);
    m_spec.SetBufferPool(&pool);
    m_spec.LaunchThreads();

    acq.join();
    m_spec.JoinThreads();

    return 0;
}
