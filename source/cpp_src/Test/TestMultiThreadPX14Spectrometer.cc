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
#include "HCudaHostBufferAllocator.hh"
#include "HBufferAllocatorSpectrometerDataCUDA.hh"
#include "HSimpleMultiThreadedSpectrumDataWriter.hh"
#include "HPeriodicPowerCalculator.hh"

using namespace hose;

#define FAKE_SPECTRUM_LENGTH 16384

using PoolType = HBufferPool< uint16_t >;

int main(int /*argc*/, char** /*argv*/)
{
    size_t n_ave = 64;
    size_t vector_length = FAKE_SPECTRUM_LENGTH*n_ave;
    // size_t vector_length = 1024*n_ave;
    size_t nAcq = 1;
    unsigned int n_dropped = 0;

    //digitizer
    HPX14Digitizer dummy;
    bool initval = dummy.Initialize();

    std::cout<<"initval = "<<initval<<std::endl;
    std::cout<<"done card init"<<std::endl;

    std::cout<<"allocating cuda host buffs"<<std::endl;
    //create source buffer pool
    HCudaHostBufferAllocator< uint16_t >* balloc = new HCudaHostBufferAllocator<  uint16_t >();
    HBufferPool< uint16_t >* source_pool = new HBufferPool< uint16_t >( dummy.GetAllocator() );

    const size_t source_n_chunks = 32;
    const size_t source_items_per_chunk = vector_length;
    source_pool->Allocate(source_n_chunks, source_items_per_chunk);
    std::cout<<"done"<<std::endl;

    dummy.SetBufferPool(source_pool);

    std::cout<<"allocating cuda dev buffs"<<std::endl;
    //create sink buffer pool
    HBufferAllocatorSpectrometerDataCUDA< spectrometer_data >* sdata_alloc = new HBufferAllocatorSpectrometerDataCUDA<spectrometer_data>();
    sdata_alloc->SetSampleArrayLength(vector_length);
    sdata_alloc->SetSpectrumLength(FAKE_SPECTRUM_LENGTH);

    HBufferPool< spectrometer_data >* sink_pool = new HBufferPool< spectrometer_data >( sdata_alloc );
    const size_t sink_n_chunks = 16;
    const size_t sink_items_per_chunk = 1; //THERE CAN BE ONLY ONE!!!
    sink_pool->Allocate(sink_n_chunks, sink_items_per_chunk);
    std::cout<<"done"<<std::endl;

    HSpectrometerCUDA m_spec(FAKE_SPECTRUM_LENGTH, n_ave);
    m_spec.SetNThreads(1);
    for(unsigned int i=0; i<4; i++)
    {
        m_spec.AssociateThreadWithSingleProcessor(i, i+1);
    };

    m_spec.SetSourceBufferPool(source_pool);
    m_spec.SetSinkBufferPool(sink_pool);

    std::cout<<"sampling freq = "<<dummy.GetSamplingFrequency()<<std::endl;
    m_spec.GetPowerCalculator()->SetSamplingFrequency( dummy.GetSamplingFrequency() );
    m_spec.GetPowerCalculator()->SetSwitchingFrequency( 80.0 );
    m_spec.GetPowerCalculator()->SetBlankingPeriod( 20.0*(1.0/dummy.GetSamplingFrequency()) );

    //file writing consumer to drain the spectrum data buffers
    HSimpleMultiThreadedSpectrumDataWriter spec_writer;
    spec_writer.SetBufferPool(sink_pool);
    spec_writer.SetNThreads(1);
    for(unsigned int i=0; i<4; i++)
    {
        spec_writer.AssociateThreadWithSingleProcessor(i, i+5);
    };

    std::cout<<"starting"<<std::endl;
    spec_writer.StartConsumption();
    m_spec.StartConsumptionProduction();
    dummy.StartProduction();

    //wait 
    usleep(100000000);

    std::cout<<"stopping"<<std::endl;
    dummy.StopProduction();
    m_spec.StopConsumptionProduction();
    spec_writer.StopConsumption();

    sleep(2);

    delete source_pool;
    delete sink_pool;
    delete balloc;
    delete sdata_alloc;

    return 0;
}




// using namespace hose;
// 
// using PoolType = HBufferPool< HPX14Digitizer::sample_type >;
// uint64_t vector_length = 2000000;
// uint64_t nAcq = 100;
// 
// void RunAcquireThread(HPX14Digitizer* dummy, PoolType* pool, HSpectrometerCUDA* spec)
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
//                 dummy->SetBuffer(buff);
//                 if(count == 0){ dummy->Acquire();};
//                 dummy->Transfer();
//                 dummy->Finalize();
//                 pool->PushConsumerBuffer(dummy->GetBuffer());
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
//                 dummy->SetBuffer(buff);
//                 dummy->Acquire();
//                 dummy->Transfer();
//                 dummy->Finalize();
//                 pool->PushConsumerBuffer(dummy->GetBuffer());
//             }
//         }
//     }
//     spec->SignalTerminateOnComplete();
// };
// 
// 
// 
// int main(int /*argc*/, char** /*argv*/)
// {
//     //dummy digitizer
//     HPX14Digitizer dummy;
//     bool initval = dummy.Initialize();
// 
//     //create pool buffer
//     PoolType pool( dummy.GetAllocator() );
// 
//     //allocate space
//     const size_t n_chunks = nAcq;
//     const size_t items_per_chunk = vector_length;
//     pool.Allocate(n_chunks, items_per_chunk);
// 
//     HSpectrometerCUDA m_spec;
// 
//     std::thread acq(RunAcquireThread, &dummy, &pool, &m_spec);
//     m_spec.SetNThreads(10);
//     m_spec.SetSleepMicroSeconds(1);
//     m_spec.SetBufferPool(&pool);
//     m_spec.LaunchThreads();
// 
//     acq.join();
//     m_spec.JoinThreads();
// 
//     return 0;
// }
