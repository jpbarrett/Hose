#include "HSpectrometerCUDASigned.hh"
#include "HSpectrumObject.hh"

#include <fstream>
#include <pthread.h>

namespace hose
{

HSpectrometerCUDASigned::HSpectrometerCUDASigned():
    fDataLength(1),
    fNThreads(1),
    fSignalTerminate(false),
    fForceTerminate(false),
    fSleepTime(1),
    fBufferPool(nullptr),
    fOutputDirectory( STR2(DATA_INSTALL_DIR) )
{

}

HSpectrometerCUDASigned::~HSpectrometerCUDASigned(){};


void
HSpectrometerCUDASigned::LaunchThreads()
{
    unsigned int num_cpus = std::thread::hardware_concurrency();
    //allocate workspace for each thread
    if(fThreads.size() == 0)
    {
        fSignalTerminate = false;
        fForceTerminate = false;
        for(unsigned int i=0; i<fNThreads; i++)
        {
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET((i+8)%num_cpus, &cpuset);
            // std::cout<<"setting spec thread #"<<i<<" to cpu #"<<(i+8)%num_cpus<<std::endl;
            fThreads.push_back( std::thread( &HSpectrometerCUDASigned::ProcessLoop, this ) );
            int rc = pthread_setaffinity_np(fThreads.back().native_handle(), sizeof(cpu_set_t), &cpuset);
            if(rc != 0)
            {
                std::cout<<"Error, could not set thread affinity"<<std::endl;
            }
        }
    }
    else
    {
        std::exit(1);
        //error, threads already launched
    }

}



void
HSpectrometerCUDASigned::JoinThreads()
{
    for(unsigned int i=0; i<fThreads.size(); i++)
    {
        fThreads[i].join();
    }
}

//the data processing loop for each thread
void
HSpectrometerCUDASigned::ProcessLoop()
{
    //initialize the thread workspace
    int spectrum_length = SPECTRUM_LENGTH_S;
    int data_length = fDataLength;
    spectrometer_data_s* sdata = nullptr;

    std::cout<<"data length = "<<data_length<<" and spec len = "<<spectrum_length<<std::endl;
    sdata = new_spectrometer_data_s(data_length, spectrum_length);

    while( !fForceTerminate && (!fSignalTerminate || fBufferPool->GetConsumerPoolSize() != 0 ) )
    {
        HLinearBuffer< int16_t >* tail = nullptr;
        if( fBufferPool->GetConsumerPoolSize() != 0 )
        {
            //grab a buffer to process
            tail = fBufferPool->PopConsumerBuffer();
            if(tail != nullptr)
            {
                std::lock_guard<std::mutex> lock( tail->fMutex );

                if(tail->GetMetaData()->GetLeadingSampleIndex() % ( 100*tail->GetArrayDimension(0) ) == 0 )
                {
                    std::thread::id this_id = std::this_thread::get_id();
                    std::stringstream ss;
                    ss << "./";
                    ss << "thread: ";
                    ss << this_id;
                    ss << " is processing samples indexed @ ";
                    ss << tail->GetMetaData()->GetLeadingSampleIndex();
                    std::cout<<ss.str()<<std::endl;
                }

                // //print the first couple samples 
                std::size_t tmp = 0;
                std::cout<<"sample @ "<<tmp<<" = "<<(tail->GetData())[tmp]<<std::endl; tmp++;

                //call Juha's process_vector routine
                process_vector_no_output_s(tail->GetData(), sdata);

                //append the averaged spectrum to file associated with this thread
                //std::thread::id this_id = std::this_thread::get_id();
                std::stringstream ss;
                //ss << fOutputDirectory;
                ss << DATA_INSTALL_DIR;
                ss << "/";
                ss << tail->GetMetaData()->GetAcquisitionStartSecond();
                ss << "/";
                ss << tail->GetMetaData()->GetAcquisitionStartSecond();
                ss << "_";
                ss << tail->GetMetaData()->GetLeadingSampleIndex();
                ss << ".bin";

                HSpectrumObject< float > spec_data;
                spec_data.SetStartTime( tail->GetMetaData()->GetAcquisitionStartSecond() );
                spec_data.SetSampleRate( tail->GetMetaData()->GetSampleRate() );
                spec_data.SetLeadingSampleIndex( tail->GetMetaData()->GetLeadingSampleIndex() );
                spec_data.SetSampleLength(tail->GetArrayDimension(0));
                spec_data.SetNAverages( tail->GetArrayDimension(0)/SPECTRUM_LENGTH_S );

                //free the tail for re-use
                fBufferPool->PushProducerBuffer( tail );

                spec_data.SetSpectrumLength(spectrum_length/2+1); //Fix naming of this
                spec_data.SetSpectrumData(sdata->spectrum);
                std::cout<<"file name = "<<ss.str()<<std::endl;
                spec_data.WriteToFile(ss.str());
                spec_data.ReleaseSpectrumData();


                
            }
        }
    }

    if(sdata != nullptr)
    {
        free_spectrometer_data_s(sdata);
    }
}



}
