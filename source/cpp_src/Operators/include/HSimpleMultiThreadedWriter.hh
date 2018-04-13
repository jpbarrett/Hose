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
#include "HConsumer.hh"


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
class HSimpleMultiThreadedWriter: public HConsumer< XBufferItemType, HConsumerBufferHandler_Immediate< XBufferItemType> >
{
    public:

        HSimpleMultiThreadedWriter(){};
        virtual ~HSimpleMultiThreadedWriter(){};

    private:

        virtual void ExecuteThreadTask()
        {
            //get a buffer from the buffer handler
            HLinearBuffer< XBufferItemType >* tail = nullptr;
            if( this->fBufferPool->GetConsumerPoolSize() != 0 )
            {
                //grab a buffer to process
                HConsumerBufferPolicyCode buffer_code = this->fBufferHandler.ReserveBuffer(this->fBufferPool, tail);

                if(buffer_code == HConsumerBufferPolicyCode::success && tail != nullptr)
                {
                    std::lock_guard<std::mutex> lock( tail->fMutex );

                    std::stringstream ss;
                    ss << "./";
                    ss << tail->GetMetaData()->GetLeadingSampleIndex();
                    ss << ".bin";

                    std::cout<<"sample index = "<<tail->GetMetaData()->GetLeadingSampleIndex()<<std::endl;
                    std::cout<<"sample val = "<<(tail->GetData())[0]<<std::endl;

                    std::ofstream out_file;
                    out_file.open (ss.str().c_str(),  std::ios::out | std::ios::binary);
                    out_file.write( (char*)(tail->GetData()), (std::streamsize) ( tail->GetArrayDimension(0) )*sizeof(XBufferItemType) );
                    out_file.close();

                    //free the tail for re-use
                    this->fBufferHandler.ReleaseBufferToProducer(this->fBufferPool, tail);
                }
            }
        }
        
        virtual bool WorkPresent() override
        {
            return ( this->fBufferPool->GetConsumerPoolSize() != 0 );
        }

        virtual void Idle() override
        {
            usleep(10);
        }


};

}

#endif /* end of include guard: HSimpleMultiThreadedWriter */
