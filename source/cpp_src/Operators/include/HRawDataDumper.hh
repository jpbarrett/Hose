#ifndef HRawDataDumper_HH__
#define HRawDataDumper_HH__

#include <iostream>
#include <ostream>
#include <ios>
#include <fstream>
#include <sstream>
#include <string>

extern "C"
{
    #include "HBasicDefines.h"
}

#include "HLinearBuffer.hh"
#include "HBufferPool.hh"
#include "HConsumer.hh"

namespace hose
{

/*
*File: HRawDataDumper.hh
*Class: HRawDataDumper
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: capture single buffer directly from digitizer/producer and write to disk w/o header
*/

template< typename XBufferItemType >
class HRawDataDumper: public HConsumer< XBufferItemType, HConsumerBufferHandler_Immediate< XBufferItemType > >
{

    public:
        HRawDataDumper():fBufferDumpFrequency(2),fOutputDirectory("./"){}; 
        virtual ~HRawDataDumper(){};

        //frequency at which buffers are dumped to disk...1 is every buffer, 2 is every other, etc.
        void SetBufferDumpFrequency(unsigned int buff_freq){fBufferDumpFrequency = buff_freq;};

        void SetOutputDirectory(std::string output_dir){fOutputDirectory = output_dir;}
        std::string GetOutputDirectory() const {return fOutputDirectory;};

    private:

        virtual void ExecuteThreadTask() override
        {
            //grab buffer and get timestamp, evaluate if enough time has passed
            //if so, then write buffer to raw output file in data directory
            //get a buffer from the buffer handler
            HLinearBuffer< XBufferItemType >* tail = nullptr;
            if( this->fBufferPool->GetConsumerPoolSize( this->GetConsumerID() ) != 0 )
            {
                //grab a buffer to process
                HConsumerBufferPolicyCode buffer_code = this->fBufferHandler.ReserveBuffer(this->fBufferPool, tail, this->GetConsumerID() );

                if(buffer_code == HConsumerBufferPolicyCode::success && tail != nullptr)
                {
                    std::lock_guard<std::mutex> lock( tail->fMutex );
        
                    uint64_t most_recent_sample_index = tail->GetMetaData()->GetLeadingSampleIndex() + tail->GetArraySize();

                    if(most_recent_sample_index > fMostRecentSampleIndex)
                    {
                        //try to grab mutex
                        std::lock_guard<std::mutex> lock2(fMutex);
                        if(most_recent_sample_index > fMostRecentSampleIndex) //double check to make sure the data didn't change on us
                        {
                            //update most recent sample index and the buffer count
                            fMostRecentSampleIndex = most_recent_sample_index;
                            fBufferCount += 1;

                            //if the buffer count has reached the appropriate height, trigger a data dump
                            if(fBufferCount >= fBufferDumpFrequency)
                            {
                                std::stringstream ss;
                                ss << fOutputDirectory;
                                ss << "/";
                                ss <<  tail->GetMetaData()->GetAcquisitionStartSecond();
                                ss << "_";
                                ss <<  tail->GetMetaData()->GetLeadingSampleIndex();
                                ss << "_";
                                ss <<  tail->GetMetaData()->GetSidebandFlag();
                                ss <<  tail->GetMetaData()->GetPolarizationFlag();
                                ss << ".bin";

                                std::ofstream out_file;
                                out_file.open (ss.str().c_str(),  std::ios::out | std::ios::binary);
                                out_file.write( (char*)(tail->GetData()), (std::streamsize) ( tail->GetArrayDimension(0) )*sizeof(XBufferItemType) );
                                out_file.close();

                                //reset
                                fBufferCount = 0;
                            }
                        }
                    }
                    //free the tail for use by other consumer
                    this->fBufferHandler.ReleaseBufferToConsumer(this->fBufferPool, tail, this->GetNextConsumerID() );
                }
            }
        }

        virtual bool WorkPresent() override
        {
            return ( this->fBufferPool->GetConsumerPoolSize( this->GetConsumerID() ) != 0 );
        }

    private:

        //these members are protected by the mutex
        std::mutex fMutex;
        unsigned int fBufferDumpFrequency;
        unsigned int fBufferCount;
        uint64_t fMostRecentSampleIndex;

        std::string fOutputDirectory;


};


}

#endif /* end of include guard: HRawDataDumper */
