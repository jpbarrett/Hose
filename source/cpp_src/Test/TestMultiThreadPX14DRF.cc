#include <iostream>
#include <ostream>
#include <ios>
#include <vector>
#include <mutex>
#include <memory>
#include <fstream>
#include <sstream>
#include <thread>
#include <unistd.h>

#include<ctime>


#include "HBufferAllocatorNew.hh"
#include "HPX14Digitizer.hh"
#include "HBufferPool.hh"
#include "HSimpleMultiThreadedWriter.hh"

#include "digital_rf.h"

using namespace hose;

using RingType = HBufferPool< HPX14Digitizer::sample_type >;

bool AcquisitionRunning;

//std::mutex DRFMutex;

/* global writing parameters */
uint64_t sample_rate_numerator = 200000000; /* 200 MHz sample rate */
uint64_t sample_rate_denominator = 1;
uint64_t subdir_cadence = 1; /* Number of seconds per subdirectory */
uint64_t millseconds_per_file = 10; /* Each subdirectory will have up to 100 x 10 ms files */
int compression_level = 1; /* low level of compression */
int checksum = 0; /* no checksum */
int is_complex = 0; /* real values */
int is_continuous = 0; /* continuous data written */
int num_subchannels = 1; /* only one subchannel */
int marching_periods = 0; /* no marching periods when writing */
char uuid[100] = "Fake UUID - use a better one!";
uint64_t vector_length = 2000000; /* number of samples written for each call - typically MUCH longer */
Digital_rf_write_object * data_object = NULL; /* main object created by init */


void RunAcquireThread(HPX14Digitizer* dummy, RingType* ring)
{
    std::cout<<"in aquire thread"<<std::endl;
    //do 15 buffer aquisitions
    unsigned int count = 0;
    while(count < 100)
    {
        std::cout<<"producer pool size = "<<ring->GetProducerPoolSize()<<std::endl;
        if(ring->GetProducerPoolSize() != 0)
        {
            std::cout<<"getting a buffer"<<std::endl;
            HLinearBuffer< HPX14Digitizer::sample_type >* buff = ring->PopProducerBuffer();
            if(buff != nullptr)
            {
                dummy->SetBuffer(buff);
                if(count == 0)
                {
                    dummy->Acquire();
                }
                dummy->Transfer();
                dummy->Finalize();
                ring->PushConsumerBuffer(dummy->GetBuffer());
                count++;
            }
        }
        else
        {
            //steal a buffer from the consumer pool..however, this requires dropping
            //a previous aquisition, so we don't increment the count
            HLinearBuffer< HPX14Digitizer::sample_type >* buff = ring->PopConsumerBuffer();
            if(buff != nullptr)
            {
                dummy->SetBuffer(buff);
                dummy->Acquire();
                dummy->Transfer();
                dummy->Finalize();
                ring->PushConsumerBuffer(dummy->GetBuffer());
            }
        }

    }
    AcquisitionRunning = false;
};

void RunWriterThread(RingType* ring)
{
	uint64_t vector_leading_edge_index = 0; /* index of the sample being written starting at zero with the first sample recorded */

	int i, result;

    unsigned int count = 0;
    HLinearBuffer< HPX14Digitizer::sample_type >* tail = nullptr;
    while( AcquisitionRunning || ring->GetConsumerPoolSize() != 0)
    {
        //std::cout<<"about to check for empty ring"<<std::endl;
        // if( ring->GetConsumerPoolSize() != 0 )
        // {
        //     std::cout<<"grabbing consumer buffer"<<std::endl;
        //     tail = ring->PopConsumerBuffer();
        //     if(tail != nullptr)
        //     {
        //         //lock the buffer
        //         std::lock_guard<std::mutex> buffer_lock( tail->fMutex );
        // 
        //         //lock the digital rf object...things are pretty much going to be running serially b/c of this
        // 
        //         //write the buffer to a file
        //         result = digital_rf_write_hdf5(data_object, tail->GetLeadingSampleIndex(), tail->GetData(), tail->GetArrayDimension(0));
        //         if (result){exit(-1);}
        // 
        //         ring->PushProducerBuffer( tail );
        //     }
        //     
        // }
        if( ring->GetConsumerPoolSize() != 0 )
        {
            tail = ring->PopConsumerBuffer();
            if(tail != nullptr)
            {
                std::cout<<"grabbing consumer buffer"<<std::endl;
                std::lock_guard<std::mutex> lock( tail->fMutex );

                std::thread::id this_id = std::this_thread::get_id();
                std::stringstream ss;
                ss << "./";
                ss << tail->GetLeadingSampleIndex();
                ss << ".bin";
                count++;

                //std::cout<<"constructing file:"<<ss.str()<<std::endl;
                std::ofstream myfile;
                myfile.open (ss.str().c_str(),  std::ios::out | std::ios::binary);
                myfile.write( (char*)(tail->GetData()), (std::streamsize) ( tail->GetArrayDimension(0) )*sizeof(HPX14Digitizer::sample_type) );
                // for(size_t i=0; i< tail->GetArrayDimension(0); i++)
                // {
                //     myfile << (*tail)[i];
                // }
                myfile.close();

                ring->PushProducerBuffer( tail );
            }
        }
        else
        {
            usleep(1);
        }
    }
    
};


int main(int /*argc*/, char** /*argv*/)
{
    //dummy digitizer
    HPX14Digitizer dummy;
    bool initval = dummy.Initialize();

    //create ring buffer
    std::cout<<"creating ring buff"<<std::endl;
    RingType ring( dummy.GetAllocator() );

    /* init */
    std::time_t t = std::time(0);  //current unix time
    uint64_t global_start_index = (uint64_t)(t * (long double)sample_rate_numerator/sample_rate_denominator) + 1; /* should represent 2014-03-09 12:30:30  and 10 milliseconds*/

    // std::cout<<"creating drf obj"<<std::endl;
    // data_object = digital_rf_create_write_hdf5( "./junk" , H5T_NATIVE_SHORT, subdir_cadence, millseconds_per_file,
    //     global_start_index, sample_rate_numerator, sample_rate_denominator, uuid, compression_level, checksum, is_complex, num_subchannels,
    //     is_continuous, marching_periods);
    // if (!data_object){exit(-1);}

    //allocate space
    const size_t n_chunks = 100;
    const size_t items_per_chunk = vector_length;
    ring.Allocate(n_chunks, items_per_chunk);

    AcquisitionRunning = true;

    std::cout<<"starting aquire thread"<<std::endl;
    std::thread acq(RunAcquireThread, &dummy, &ring);
    std::cout<<"starting writer thread"<<std::endl;
    std::thread writer1(RunWriterThread, &ring); //only one thread allowed when using digitial_rf
    std::thread writer2(RunWriterThread, &ring); 
    std::thread writer3(RunWriterThread, &ring); 
    std::thread writer4(RunWriterThread, &ring); 
    std::thread writer5(RunWriterThread, &ring); 
    std::thread writer6(RunWriterThread, &ring); 
    std::thread writer7(RunWriterThread, &ring); 

    acq.join();
    writer1.join();
    writer2.join();
    writer3.join();
    writer4.join();
    writer5.join();
    writer6.join();
    writer7.join();
    // 
    // /* close */
    // digital_rf_close_write_hdf5(data_object);


    return 0;
}
