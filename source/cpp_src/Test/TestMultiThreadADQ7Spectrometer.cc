#include <iostream>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <thread>
#include <unistd.h>
#include <getopt.h>

#include "HADQ7Digitizer.hh"
#include "HBufferPool.hh"
#include "HSpectrometerCUDASigned.hh"
#include "HCudaHostBufferAllocator.hh"

using namespace hose;

using PoolType = HBufferPool< HADQ7Digitizer::sample_type >;
uint64_t nAcq = 200;

unsigned int nBuffersDropped = 0;

void RunAcquireThread(HADQ7Digitizer* dummy, PoolType* pool, HSpectrometerCUDASigned* spec)
{
    unsigned int count = 0;
    bool buff_overflow = false;
    while(count < nAcq)
    {
        if(pool->GetProducerPoolSize() != 0)
        {
            HLinearBuffer< HADQ7Digitizer::sample_type >* buff = pool->PopProducerBuffer();
            if(buff != nullptr)
            {
                dummy->SetBuffer(buff);
                if(count == 0 || buff_overflow)
                {
                    dummy->Acquire();
                    buff_overflow = false;
                };

                dummy->Transfer();
                int err_code = dummy->Finalize();
                if(err_code != 0)
                {
                    std::cout<<"Card buffer overflow error, temporarily stopping acquire."<<std::endl;
                    dummy->Stop();
                    buff_overflow = true;
                    //put garbage buffer back
                    pool->PushProducerBuffer(dummy->GetBuffer());
                    //sleep for a while
                    nBuffersDropped++;
                    usleep(500000);
                }
                else 
                {
                    pool->PushConsumerBuffer(dummy->GetBuffer());
                    count++;
                    if(count % 100 == 0){std::cout<<"count = "<<count<<std::endl;}
                }
            }
        }
        else
        {
            std::cout<<"Ring buffer overflow error, temporarily stopping acquire."<<std::endl;
            dummy->Stop();
            buff_overflow = true;
            //put garbage buffer back
            pool->PushProducerBuffer(dummy->GetBuffer());
            nBuffersDropped++;

            //wait until buffer pool is replenished 
            while(pool->GetConsumerPoolSize() != 0)
            {
                usleep(10);
            }


            /*
            //steal a buffer from the consumer pool..however, this requires dropping
            //a previous aquisition, so we don't increment the count
            HLinearBuffer< HADQ7Digitizer::sample_type >* buff = pool->PopConsumerBuffer();
            if(buff != nullptr)
            {
                nBuffersDropped++;
                dummy->SetBuffer(buff);
                dummy->Transfer();
                dummy->Finalize();
                pool->PushConsumerBuffer(dummy->GetBuffer());
                count++;
                if(count % 100 == 0)
                {
                    std::cout<<"stolen! count = "<<count<<std::endl;
                }
            }
            */


        }
    }

    dummy->Stop();

    spec->SignalTerminateOnComplete();
};



int main(int argc, char** argv)
{
    std::string usage =
    "\n"
    "Usage: TestMultiThreadADQ7Spectrometer <options>\n"
    "\n"
    "Acquire data via the ADQ7 digitizer and computer average spectra.\n"
    "\tAvailable options:\n"
    "\t -h, --help               (shows this message and exits)\n"
    "\t -s  --sample-skip        (set the decimation factor, reduces sample rate by this factor)\n"
    "\t -n, --n-spectra          (number of spectra to acquire)\n"
    "\t -m, --n-averages         (number of spectra to average together)\n"
    "\t -c, --chunks             (number of pre-allocated buffers)\n"
    "\t -a, --acquire-threads    (number of acquisition threads)\n"
    "\t -p, --process-threads (number of processing threads)\n"
    "\t -o, --output-directory   (output directory to store data) \n"
    ;

    //set defaults
    unsigned int sample_skip = 8;
    unsigned int n_spectra = 200;
    unsigned int n_chunks = 80;
    unsigned int n_averages = 1024;
    unsigned int acq_threads = 4;
    unsigned int proc_threads = 4;
    std::string o_dir =  DATA_INSTALL_DIR;

    static struct option longOptions[] =
    {
        {"help", no_argument, 0, 'h'},
        {"sample-skip", required_argument, 0, 's'},
        {"n-spectra", required_argument, 0, 'n'},
        {"n-averages", required_argument, 0, 'm'},
        {"chunks", required_argument, 0, 'c'},
        {"acquire-threads", required_argument, 0, 'a'},
        {"process-threads", required_argument, 0, 'p'},
        {"output-directory", required_argument, 0, 'o'},
    };

    static const char *optString = "hs:n:m:c:a:p:o:";

    while(1)
    {
        char optId = getopt_long(argc, argv,optString, longOptions, NULL);
        if(optId == -1) break;
        switch(optId)
        {
            case('h'): // help
            std::cout<<usage<<std::endl;
            return 0;
            case('s'):
            sample_skip = atoi(optarg);
            break;
            case('n'):
            n_spectra = atoi(optarg);
            break;
            case('m'):
            n_averages = atoi(optarg);
            break;
            case('c'):
            n_chunks = atoi(optarg);
            break;
            case('a'):
            acq_threads = atoi(optarg);
            break;
            case('p'):
            proc_threads = atoi(optarg);
            break;
            case('o'):
            o_dir = std::string(optarg);
            default:
                std::cout<<usage<<std::endl;
            return 1;
        }
    }

    nAcq = n_spectra;

    //dummy digitizer
    HADQ7Digitizer dummy;
    dummy.SetNThreads(acq_threads);
    dummy.SetDecimationFactor(sample_skip);
    bool initval = dummy.Initialize();

    //create buffer pool
    HCudaHostBufferAllocator< HADQ7Digitizer::sample_type >* balloc = new HCudaHostBufferAllocator<  HADQ7Digitizer::sample_type >();
    PoolType* pool = new PoolType( balloc );

    //allocate space
    const size_t items_per_chunk = SPECTRUM_LENGTH_S*n_averages;
    pool->Allocate(n_chunks, items_per_chunk);

    HSpectrometerCUDASigned m_spec;
    m_spec.SetDataLength(items_per_chunk);
    m_spec.SetOutputDirectory(o_dir);
    m_spec.SetNThreads(proc_threads);
    m_spec.SetBufferPool(pool);
    m_spec.LaunchThreads();

    //give some time to set up the gpu
    sleep(5);

    //launch aquire thread
    std::thread acq(RunAcquireThread, &dummy, pool, &m_spec);

    acq.join();
    m_spec.JoinThreads();
    dummy.TearDown();

    std::cout<<"n buffers dropped = "<<nBuffersDropped<<std::endl;

    delete pool;
    delete balloc;

    return 0;
}
