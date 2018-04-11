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




void RunAcquireThread(HDummyDigitizer< short >* dummy, PoolType* pool, HSpectrometerCUDASigned* spec)
{
    unsigned int count = 0;
    while(count < nAcq)
    {
        if(pool->GetProducerPoolSize() != 0)
        {
            HLinearBuffer< short >* buff = pool->PopProducerBuffer();
            if(buff != nullptr)
            {
                dummy->SetBuffer(buff);
                if(count == 0){ dummy->Acquire();};
                dummy->Transfer();
                dummy->Finalize();
                pool->PushConsumerBuffer(dummy->GetBuffer());
                count++;
                if(count % 100 == 0){std::cout<<"count = "<<count<<std::endl;}
            }
        }
        else
        {
            //steal a buffer from the consumer pool..however, this requires dropping
            //a previous aquisition, so we don't increment the count
            HLinearBuffer< short >* buff = pool->PopConsumerBuffer();
            if(buff != nullptr)
            {
                dummy->SetBuffer(buff);
                dummy->Acquire();
                dummy->Transfer();
                dummy->Finalize();
                pool->PushConsumerBuffer(dummy->GetBuffer());
                count++;
                n_dropped++;
                if(count % 100 == 0)
                {
                    std::cout<<"stolen! count = "<<count<<std::endl;
                }
            }
        }
    }
    spec->SignalTerminateOnComplete();
};






int main(int /*argc*/, char** /*argv*/)
{

    //dummy digitizer
    HDummyDigitizer< short > dummy;
    dummy.Initialize();

    //create buffer pool
    HCudaHostBufferAllocator< short >* balloc = new HCudaHostBufferAllocator<  short >();
    PoolType* pool = new PoolType( balloc );

    //allocate space
    const size_t n_chunks = 10;
    const size_t items_per_chunk = vector_length;
    pool->Allocate(n_chunks, items_per_chunk);

    HSpectrometerCUDASigned m_spec;
    m_spec.SetDataLength(items_per_chunk);
    std::cout<<"data install dir = "<<DATA_INSTALL_DIR<<std::endl;
    m_spec.SetOutputDirectory(DATA_INSTALL_DIR);
    m_spec.SetNThreads(1);
    m_spec.SetBufferPool(pool);
    m_spec.LaunchThreads();

    std::thread acq(RunAcquireThread, &dummy, pool, &m_spec);

    acq.join();
    m_spec.JoinThreads();

    std::cout<<"number of dropped buffers = "<<n_dropped<<std::endl;

    delete pool;
    delete balloc;

    return 0;
}
