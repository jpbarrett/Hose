#include "HSpectrometerCUDA.hh"

namespace hose
{

HSpectrometerCUDA::HSpectrometerCUDA():
    fNThreads(1), 
    fSignalTerminate(false),
    fForceTerminate(false),
    fSleepTime(1),
    fBufferPool(nullptr)
{

}

HSpectrometerCUDA::~HSpectrometerCUDA(){};


void 
HSpectrometerCUDA::LaunchThreads()
{
    //allocate workspace for each thread
    if(fThreads.size() == 0)
    {
        fSignalTerminate = false;
        fForceTerminate = false;
        for(unsigned int i=0; i<fNThreads; i++)
        {
            fThreads.push_back( std::thread( &HSpectrometerCUDA::ProcessLoop, this ) );
        }
    }
    else
    {
        //error, threads already launched
    }
}



void 
HSpectrometerCUDA::JoinThreads()
{
    for(unsigned int i=0; i<fThreads.size(); i++)
    {
        fThreads[i].join();
    }
}

//the data processing loop for each thread
void 
HSpectrometerCUDA::ProcessLoop()
{
//     //initialize the thread workspace
//     bool initialized = false;
//     int spectrum_length = SPECTRUM_LENGTH;
//     int data_length = 0;
//     spectrometer_data* sdata = nullptr;
//     spectrometer_output* out_data = nullptr;
// 
//     while( !fForceTerminate && (!fSignalTerminate || fBufferPool->GetConsumerPoolSize() != 0 ) )
//     {
//         HLinearBuffer< uint16_t >* tail = nullptr;
//         if( fBufferPool->GetConsumerPoolSize() != 0 )
//         {
//             //grab a buffer to process
//             tail = fBufferPool->PopConsumerBuffer();
//             if(tail != nullptr)
//             {
//                 std::lock_guard<std::mutex> lock( tail->fMutex );
// 
//                 if(!initialized)
//                 {
//                     data_length = tail->GetArrayDimension(0);
//                     sdata = new_spectrometer_data(data_length, spectrum_length);
//                     out_data = new_spectrometer_output();
//                     initialized = true;
//                 }
// 
//                 std::thread::id this_id = std::this_thread::get_id();
//                 std::stringstream ss;
//                 // ss << "./";
//                 ss << "thread: ";
//                 ss << this_id;
//                 ss << " is processing samples indexed @ ";
//                 ss << tail->GetMetaData()->GetLeadingSampleIndex();
//                 // ss << ".bin";
// 
//                 //call Juha's process_vector routine
//                 std::cout<<ss.str()<<std::endl;
//                 process_vector(tail->GetData(), sdata, tail->GetMetaData()->GetLeadingSampleIndex(), out_data, "./");
// 
//                 //free the tail for re-use
//                 fBufferPool->PushProducerBuffer( tail );
//             }
//         }
//         else
//         {
//             usleep(fSleepTime);
//         }
//     }
// 
//     free_spectrometer_data(sdata);
}



}
