#ifndef HProducerBufferHandlerPolicy_HH__
#define HProducerBufferHandlerPolicy_HH__

#include "HBufferPool.hh"
#include "HLinearBuffer.hh"

namespace hose
{

/*
*File: HProducerBufferHandlerPolicy.hh
*Class: HProducerBufferHandlerPolicy
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/


//if no buffer is available, fail and return nullptr
template< XBufferItemType >
struct HProducerBufferHander_Fail;

//wait indefinitely until a buffer is available
template< XBufferItemType >
struct HProducerBufferHander_Wait;

//wait for buffer until time-out is reached
template< XBufferItemType >
struct HProducerBufferHander_WaitWithTimeout

//steal a consumer buffer to use for production
template< XBufferItemType >
struct HProducerBufferHander_Steal;

//wait for all consumer buffers to be freed for production, before returning the next buffer
template< XBufferItemType >
struct HProducerBufferHander_Flush;

//forcefully release all un-reserved consumer buffers for production, then return the next buffer
template< XBufferItemType >
struct HProducerBufferHander_ForceFlush;





template< XBufferItemType >
struct HProducerBufferHander_WaitWithTimeout
{
    public:

        void ReserveBuffer(HBufferPool<XBufferItemType>* pool, HLinearBuffer< XBufferItemType >* buff)
        {
            while(pool->GetProducerPoolSize() == 0)
            {
                //count up to until we reach the timeout
            }

            if(pool->GetProducerPoolSize() != 0)
            {
                return = pool->PopProducerBuffer();
            }
            else
            {   
                return nullptr;
            }
        }

        void ReleaseBuffer(HBufferPool<XBufferItemType>* pool, HLinearBuffer< XBufferItemType >* buff)
        {

        }


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

        void ReleaseBuffer(int& /*err_code*/, HLinearBuffer<XBufferItemType>* buff)
        {
            fBufferPool->PushConsumerBuffer(buff);
        }


    private:

        HBufferPool<XBufferItemType>* fBufferPool;

};


class HClearAndWaitForBuffer
{
    public:
        HProducerBufferHandlerPolicy(HBufferPool<XBufferItemType>* pool ):fBufferPool(pool){};
        virtual ~HProducerBufferHandlerPolicy();

        HLinearBuffer<XBufferItemType>* ReserveBuffer(int& error_code);

        void ReleaseBuffer(int& /*err_code*/, HLinearBuffer<XBufferItemType>* buff)
        {
            fBufferPool->PushConsumerBuffer(buff);
        }


    private:

        HBufferPool<XBufferItemType>* fBufferPool;

};

class HStealBuffer
{
    public:
        HProducerBufferHandlerPolicy(HBufferPool<XBufferItemType>* pool ):fBufferPool(pool){};
        virtual ~HProducerBufferHandlerPolicy();

        HLinearBuffer<XBufferItemType>* ReserveBuffer(int& error_code)
        {

        }


        void ReleaseBuffer(int& /*err_code*/, HLinearBuffer<XBufferItemType>* buff)
        {
            fBufferPool->PushConsumerBuffer(buff);
        }

    private:

        HBufferPool<XBufferItemType>* fBufferPool;

};



}


#endif /* end of include guard: HProducerBufferHandlerPolicy */
