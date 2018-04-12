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
    HPX14Digitizer< HPX14Digitizer::sample_type > px14;
    px14.Initialize();

    //create pool buffer
    HBufferPool< HPX14Digitizer::sample_type > pool( px14.GetAllocator() );

    //allocate space
    const size_t n_chunks = 100;
    const size_t items_per_chunk = 1024*1024;
    pool.Allocate(n_chunks, items_per_chunk);
    px14.SetBufferPool(&pool);

    HSimpleMultiThreadedWriter< HPX14Digitizer::sample_type  > m_writer;
    m_writer.SetNThreads(1); 
    m_writer.SetSleepMicroSeconds(1);
    m_writer.SetBufferPool(&pool);
    m_writer.LaunchThreads();

    px14.StartProduction();

    //wait 
    //usleep(5000);
    usleep(100000); //0.1 sec

    px14.StopProduction();

    m_writer.SignalTerminateOnComplete();

    m_writer.JoinThreads();

    return 0;
}






// void RunAcquireThread(HPX14Digitizer* px14, PoolType* pool, HSimpleMultiThreadedWriter< HPX14Digitizer::sample_type >* writer)
// {
//     //do 15 buffer aquisitions
//     unsigned int count = 0;
//     while(count < nAcq)
//     {
//         if(pool->GetProducerPoolSize() != 0)
//         {
//             HLinearBuffer< HPX14Digitizer::sample_type >* buff = pool->PopProducerBuffer();
//             if(buff != nullptr)
//             {
//                 px14->SetBuffer(buff);
//                 if(count == 0){ px14->Acquire();};
//                 px14->Transfer();
//                 px14->Finalize();
//                 pool->PushConsumerBuffer(px14->GetBuffer());
//                 count++;
//             }
//         }
//         else
//         {
//             //steal a buffer from the consumer pool..however, this requires dropping
//             //a previous aquisition, so we don't increment the count
//             HLinearBuffer< HPX14Digitizer::sample_type >* buff = pool->PopConsumerBuffer();
//             if(buff != nullptr)
//             {
//                 px14->SetBuffer(buff);
//                 px14->Acquire();
//                 px14->Transfer();
//                 px14->Finalize();
//                 pool->PushConsumerBuffer(px14->GetBuffer());
//             }
//         }
//     }
//     writer->SignalTerminateOnComplete();
// };
// 
// 
// 
// int main(int /*argc*/, char** /*argv*/)
// {
//     //px14 digitizer
//     HPX14Digitizer px14;
//     bool initval = px14.Initialize();
// 
//     //create pool buffer
//     PoolType pool( px14.GetAllocator() );
// 
//     //allocate space
//     const size_t n_chunks = nAcq;
//     const size_t items_per_chunk = vector_length;
//     pool.Allocate(n_chunks, items_per_chunk);
// 
//     HSimpleMultiThreadedWriter< HPX14Digitizer::sample_type > m_writer;
// 
//     std::thread acq(RunAcquireThread, &px14, &pool, &m_writer);
//     m_writer.SetNThreads(10);
//     m_writer.SetSleepMicroSeconds(1);
//     m_writer.SetBufferPool(&pool);
//     m_writer.LaunchThreads();
// 
//     acq.join();
//     m_writer.JoinThreads();
// 
//     return 0;
// }
