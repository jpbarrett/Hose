#ifndef HSimpleMultiThreadedWriter_HH__
#define HSimpleMultiThreadedWriter_HH__

#include <iostream>
#include <ostream>
#include <ios>
#include <fstream>
#include <sstream>
#include <thread>

#include "HLinearBuffer.hh"
#include "HBufferPool.hh"

namespace hose
{

/*
*File: HSimpleMultiThreadedWriter.hh
*Class: HSimpleMultiThreadedWriter
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

template< typename XBufferItemType >
class HSimpleMultiThreadedWriter
{
    public:

        HSimpleMultiThreadedWriter():
            fNThreads(1), 
            fSignalTerminate(false),
            fForceTerminate(false),
            fSleepTime(1),
            fBufferPool(nullptr)    
            {};

        virtual ~HSimpleMultiThreadedWriter(){};

        void SetNThreads(unsigned int n_threads){fNThreads = n_threads;};
        unsigned int GetNThreads() const {return fNThreads;};

        void SetSleepMicroSeconds(unsigned int usec){fSleepTime = usec;};

        //set the buffer pool we are working with
        void SetBufferPool( HBufferPool< XBufferItemType >* buffer_pool ){ fBufferPool = buffer_pool;};

        //create and launch the threads doing the read/write routine
        void LaunchThreads()
        {
            if(fThreads.size() == 0)
            {
                fSignalTerminate = false;
                fForceTerminate = false;
                for(unsigned int i=0; i<fNThreads; i++)
                {
                    fThreads.push_back( std::thread( &HSimpleMultiThreadedWriter::WriteLoop, this ) );
                }
            }
            else
            {
                //error, threads already launched
            }
        }

        //signal to the threads to terminate on completion of work
        void SignalTerminateOnComplete(){fSignalTerminate = true;}

        //force the threads to abandon any remaining work, and terminate immediately
        void ForceTermination(){fForceTerminate = true;}

        //join and destroy threads
        void JoinThreads()
        {
            for(unsigned int i=0; i<fThreads.size(); i++)
            {
                fThreads[i].join();
            }
        }

    private:

        //make file write loop
        void WriteLoop()
        {
            while( !fForceTerminate && (!fSignalTerminate || fBufferPool->GetConsumerPoolSize() != 0 ) )
            {
                HLinearBuffer< XBufferItemType >* tail = nullptr;
                if( fBufferPool->GetConsumerPoolSize() != 0 )
                {
                    //grab a buffer to process
                    tail = fBufferPool->PopConsumerBuffer();
                    if(tail != nullptr)
                    {
                        std::lock_guard<std::mutex> lock( tail->fMutex );

                        std::thread::id this_id = std::this_thread::get_id();
                        std::stringstream ss;
                        ss << "./";
                        ss << tail->GetMetaData()->GetLeadingSampleIndex();
                        ss << ".bin";

                        std::cout<<"sample index = "<<tail->GetMetaData()->GetLeadingSampleIndex()<<std::endl;

                        std::cout<<"sample val = "<<(tail->GetData())[0]<<std::endl;

                        std::ofstream out_file;
                        out_file.open (ss.str().c_str(),  std::ios::out | std::ios::binary);
                        //out_file.open (ss.str().c_str(),  std::ios::out );
                        // for(unsigned int i=0; i< tail->GetArrayDimension(0); i++)
                        // {
                        //     out_file << (tail->GetData())[i];
                        // }
                        out_file.write( (char*)(tail->GetData()), (std::streamsize) ( tail->GetArrayDimension(0) )*sizeof(XBufferItemType) );
                        out_file.close();

                        //free the tail for re-use
                        fBufferPool->PushProducerBuffer( tail );
                    }
                }
                else
                {
                    usleep(fSleepTime);
                }
            }
        }

        unsigned int fNThreads;
        bool fSignalTerminate;
        bool fForceTerminate;

        unsigned int fSleepTime;

        HBufferPool< XBufferItemType >* fBufferPool;

        std::vector< std::thread > fThreads;
};

}

#endif /* end of include guard: HSimpleMultiThreadedWriter */
