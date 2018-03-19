#include <iostream>
#include <vector>
#include <mutex>
#include <memory>
#include <fstream>
#include <sstream>
#include <thread>
#include <unistd.h>

#include<ctime>


#include "HBufferAllocatorNew.hh"
#include "HDummyDigitizer.hh"
#include "HBufferPool.hh"

#include "digital_rf.h"

using namespace hose;

using RingType = HBufferPool< HDummyDigitizer::sample_type >;

bool AcquisitionRunning;

//std::mutex DRFMutex;

/* global writing parameters */
uint64_t sample_rate_numerator = 1000; /* 1000 Hz sample rate - typically MUCH faster */
uint64_t sample_rate_denominator = 1;
uint64_t subdir_cadence = 4; /* Number of seconds per subdirectory - typically longer */
uint64_t millseconds_per_file = 1000; /* Each subdirectory will have up to 4 1000 ms files */
int compression_level = 1; /* low level of compression */
int checksum = 0; /* no checksum */
int is_complex = 0; /* real values */
int is_continuous = 0; /* continuous data written */
int num_subchannels = 1; /* only one subchannel */
int marching_periods = 0; /* no marching periods when writing */
char uuid[100] = "Fake UUID - use a better one!";
uint64_t vector_length = 1000; /* number of samples written for each call - typically MUCH longer */
Digital_rf_write_object * data_object = NULL; /* main object created by init */


void RunAcquireThread(HDummyDigitizer* dummy, RingType* ring)
{
    //do 15 buffer aquisitions
    unsigned int count = 0;
    while(count < 15)
    {
        if(ring->GetProducerPoolSize() != 0)
        {
            HLinearBuffer< HDummyDigitizer::sample_type >* buff = ring->PopProducerBuffer();
            if(buff != nullptr)
            {
                dummy->SetBuffer(buff);
                dummy->Acquire();
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
            HLinearBuffer< HDummyDigitizer::sample_type >* buff = ring->PopConsumerBuffer();
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
    HLinearBuffer< HDummyDigitizer::sample_type >* tail = nullptr;
    while( AcquisitionRunning || ring->GetConsumerPoolSize() != 0)
    {
        //std::cout<<"about to check for empty ring"<<std::endl;
        if( ring->GetConsumerPoolSize() != 0 )
        {
            tail = ring->PopConsumerBuffer();
            if(tail != nullptr)
            {
                //lock the buffer
                std::lock_guard<std::mutex> buffer_lock( tail->fMutex );

                //write the buffer to a file
                result = digital_rf_write_hdf5(data_object, tail->GetMetaData()->GetLeadingSampleIndex(), tail->GetData(), vector_length);
                if (result){exit(-1);}

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
    HDummyDigitizer dummy;
    bool initval = dummy.Initialize();

    //create ring buffer
    RingType ring( dummy.GetAllocator() );

    /* init */
    std::time_t t = std::time(0);  //current unix time
    uint64_t global_start_index = (uint64_t)(t * (long double)sample_rate_numerator/sample_rate_denominator) + 1; /* should represent 2014-03-09 12:30:30  and 10 milliseconds*/

    data_object = digital_rf_create_write_hdf5( "./junk" , H5T_NATIVE_SHORT, subdir_cadence, millseconds_per_file,
        global_start_index, sample_rate_numerator, sample_rate_denominator, uuid, compression_level, checksum, is_complex, num_subchannels,
        is_continuous, marching_periods);
    if (!data_object){exit(-1);}

    //allocate space
    const size_t n_chunks = 16;
    const size_t items_per_chunk = vector_length;
    ring.Allocate(n_chunks, items_per_chunk);

    AcquisitionRunning = true;

    std::thread acq(RunAcquireThread, &dummy, &ring);
    std::thread writer1(RunWriterThread, &ring); //only one thread allowed when using digitial_rf
    //std::thread writer2(RunWriterThread, &ring); 

    acq.join();
    writer1.join();
    //writer2.join();

    /* close */
    digital_rf_close_write_hdf5(data_object);


    return 0;
}
