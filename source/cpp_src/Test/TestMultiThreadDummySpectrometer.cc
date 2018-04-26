#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <thread>
#include <unistd.h>

#include "HDummyDigitizer.hh"
#include "HBufferPool.hh"
#include "HSpectrometerCUDA.hh"
#include "HCudaHostBufferAllocator.hh"
#include "HBufferAllocatorSpectrometerDataCUDA.hh"
#include "HSimpleMultiThreadedSpectrumDataWriter.hh"
#include "HPeriodicPowerCalculator.hh"

using namespace hose;


using PoolType = HBufferPool< uint16_t >;

int main(int /*argc*/, char** /*argv*/)
{
    size_t n_ave = 128;
    size_t vector_length = SPECTRUM_LENGTH_S*n_ave;
    size_t nAcq = 1;
    unsigned int n_dropped = 0;

    //dummy digitizer
    HDummyDigitizer< uint16_t > dummy;
    dummy.Initialize();

    //create source buffer pool
    HCudaHostBufferAllocator< uint16_t >* balloc = new HCudaHostBufferAllocator<  uint16_t >();
    HBufferPool< uint16_t >* source_pool = new HBufferPool< uint16_t >( balloc );

    const size_t source_n_chunks = 10;
    const size_t source_items_per_chunk = vector_length;
    source_pool->Allocate(source_n_chunks, source_items_per_chunk);
    dummy.SetBufferPool(source_pool);

    //power calculator
    HPeriodicPowerCalculator< uint16_t > power_calc;
    power_calc.SetSamplingFrequency( dummy.GetSamplingFrequency() );
    power_calc.SetSwitchingFrequency( 80.0 );
    power_calc.SetBlankingPeriod( 20.0*(1.0/dummy.GetSamplingFrequency()) );
    power_calc.SetBufferPool(source_pool);

    //create sink buffer pool
    HBufferAllocatorSpectrometerDataCUDA< spectrometer_data >* sdata_alloc = new HBufferAllocatorSpectrometerDataCUDA<spectrometer_data>();
    sdata_alloc->SetSampleArrayLength(vector_length);
    sdata_alloc->SetSpectrumLength(SPECTRUM_LENGTH);

    HBufferPool< spectrometer_data >* sink_pool = new HBufferPool< spectrometer_data >( sdata_alloc );
    const size_t sink_n_chunks = 4;
    const size_t sink_items_per_chunk = 1; //THERE CAN BE ONLY ONE!!!
    sink_pool->Allocate(sink_n_chunks, sink_items_per_chunk);

    HSpectrometerCUDA m_spec(SPECTRUM_LENGTH, n_ave);
    m_spec.SetNThreads(1);
    m_spec.SetSourceBufferPool(source_pool);
    m_spec.SetSinkBufferPool(sink_pool);

    //TODO add a file writing consumer to drain the spectrum data buffers
    HSimpleMultiThreadedSpectrumDataWriter spec_writer;
    spec_writer.SetBufferPool(sink_pool);
    spec_writer.SetNThreads(2);

    std::cout<<"starting"<<std::endl;
    spec_writer.StartConsumption();
    power_calc.StartConsumption();
    m_spec.StartConsumptionProduction();
    dummy.StartProduction();

    //wait 
    usleep(10000000); //0.1 sec

    std::cout<<"stopping"<<std::endl;
    dummy.StopProduction();
    power_calc.StopConsumption();
    m_spec.StopConsumptionProduction();
    spec_writer.StopConsumption();

    sleep(2);

    delete source_pool;
    delete sink_pool;
    delete balloc;
    delete sdata_alloc;

    return 0;
}
