#ifndef HProducerBufferHandlerPolicy_HH__
#define HProducerBufferHandlerPolicy_HH__

#include <chrono>
#include <thread>


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

//enum of return codes
enum class HProducerBufferPolicyCode
{
    unset = 0x0,
    fail = 0x1, //failed to reserve buffer
    success = 0x8, //successfully reserved (or released) a buffer
    stolen = 0x9, //successfully stole a buffer
    timeout_fail = 0x3 , //failed to reserve a buffer after timing-out
    flushed = 0xA, //flushed buffer queue, and then successfully reserved a buffer
    forced_flushed = 0xC //forcefully flushed buffer queue, then successfully reserved a buffer
};

//base class for releasing buffers (common to all handler policies)
template< typename XBufferItemType >
class HProducerBufferReleaser
{
    public:
        HProducerBufferReleaser(){;};
        virtual ~HProducerBufferReleaser(){;};

        HProducerBufferPolicyCode ReleaseBufferToProducer(HBufferPool<XBufferItemType>* pool, HLinearBuffer<XBufferItemType>*& buffer)
        {
            pool->PushProducerBuffer(buffer);
            return HProducerBufferPolicyCode::success;
        }

        HProducerBufferPolicyCode ReleaseBufferToConsumer(HBufferPool<XBufferItemType>* pool, HLinearBuffer<XBufferItemType>*& buffer)
        {
            pool->PushConsumerBuffer(buffer);
            return HProducerBufferPolicyCode::success;
        }
};


//get a buffer immediately, if no buffer is available fail and return nullptr
template< typename XBufferItemType >
class HProducerBufferHandler_Immediate: public HProducerBufferReleaser< XBufferItemType >
{
    public:
        HProducerBufferHandler_Immediate(){;};
        virtual ~HProducerBufferHandler_Immediate(){;};

        HProducerBufferPolicyCode ReserveBuffer(HBufferPool<XBufferItemType>* pool, HLinearBuffer<XBufferItemType>*& buffer)
        {
            if(pool->GetProducerPoolSize() != 0)
            {
                buffer = pool->PopProducerBuffer();
                return HProducerBufferPolicyCode::success;
            }
            else
            {   
                buffer = nullptr;
                return HProducerBufferPolicyCode::fail;
            }
        }
};

//wait indefinitely until a buffer is available
template< typename XBufferItemType >
class HProducerBufferHandler_Wait: public HProducerBufferReleaser< XBufferItemType >
{
    public:
        HProducerBufferHandler_Wait(){;};
        virtual ~HProducerBufferHandler_Wait(){;};

        HProducerBufferPolicyCode ReserveBuffer(HBufferPool<XBufferItemType>* pool, HLinearBuffer<XBufferItemType>*& buffer)
        {
            while(true)
            {
                if(pool->GetProducerPoolSize() != 0)
                {
                    buffer = pool->PopProducerBuffer();
                    return HProducerBufferPolicyCode::success;
                }
            };
        }

};


//steal a (unconsumed) consumer buffer to give to the producer
template< typename XBufferItemType >
class HProducerBufferHandler_Steal: public HProducerBufferReleaser< XBufferItemType >
{
    public:
        HProducerBufferHandler_Steal(){;};
        virtual ~HProducerBufferHandler_Steal(){;};

        HProducerBufferPolicyCode ReserveBuffer(HBufferPool<XBufferItemType>* pool, HLinearBuffer<XBufferItemType>*& buffer)
        {
            if(pool->GetProducerPoolSize() != 0)
            {
                buffer = pool->PopProducerBuffer();
                return HProducerBufferPolicyCode::success;
            }
            else
            {   
                if(pool->GetConsumerPoolSize() != 0)
                {
                    buffer = pool->PopConsumerBuffer();
                    return HProducerBufferPolicyCode::stolen;
                }
                else
                {
                    buffer = nullptr;
                    return HProducerBufferPolicyCode::fail;
                }
            }
        }

};

//wait indefinitely for all consumer buffers to be freed for production, before returning the next buffer
template< typename XBufferItemType >
class HProducerBufferHandler_Flush: public HProducerBufferReleaser< XBufferItemType >
{
    public:
        HProducerBufferHandler_Flush():fSleepDurationNanoSeconds(500){;};
        virtual ~HProducerBufferHandler_Flush(){;};

        void SetSleepDurationNanoSeconds(unsigned int ns){fSleepDurationNanoSeconds = ns;};
        unsigned int GetSleepDurationNanoSeconds() const {return fSleepDurationNanoSeconds;};

        HProducerBufferPolicyCode ReserveBuffer(HBufferPool<XBufferItemType>* pool, HLinearBuffer<XBufferItemType>*& buffer)
        {
            if(pool->GetProducerPoolSize() != 0)
            {
                buffer = pool->PopProducerBuffer();
                return HProducerBufferPolicyCode::success;
            }
            else
            {   
                //wait for the consumer buffer pool to become empty
                while( pool->GetConsumerPoolSize() != 0 )
                {
                    //sleep for the specified duration if it is non-zero
                    if(fSleepDurationNanoSeconds != 0)
                    {
                        std::this_thread::sleep_for(std::chrono::nanoseconds(fSleepDurationNanoSeconds));
                    }
                };

                //producer pool should be full now, so grab buffer
                if(pool->GetProducerPoolSize() != 0)
                {
                    buffer = pool->PopProducerBuffer();
                    return HProducerBufferPolicyCode::flushed;
                }
                else
                {   
                    buffer = nullptr;
                    return HProducerBufferPolicyCode::fail;
                }
            }
        }

    protected: 

        unsigned int fSleepDurationNanoSeconds;

};

//forcefully release all un-reserved consumer buffers for production, then return the next buffer
template< typename XBufferItemType >
class HProducerBufferHandler_ForceFlush: public HProducerBufferReleaser< XBufferItemType >
{
    public:
        HProducerBufferHandler_ForceFlush(){;};
        virtual ~HProducerBufferHandler_ForceFlush(){;};

        HProducerBufferPolicyCode ReserveBuffer(HBufferPool<XBufferItemType>* pool, HLinearBuffer<XBufferItemType>*& buffer)
        {
            if(pool->GetProducerPoolSize() != 0)
            {
                buffer = pool->PopProducerBuffer();
                return HProducerBufferPolicyCode::success;
            }
            else
            {
                //flush out all of the consumer buffers back into the producer pool w/o consuming them
                int count = 0;
                while( pool->GetConsumerPoolSize() != 0 )
                {
                    HLinearBuffer< XBufferItemType >* temp_buffer = pool->PopConsumerBuffer();
                    pool->PushProducerBuffer(temp_buffer);
                    count++;
                };

                std::cout<<"FLUSHED "<<count<<" buffers!!!!!!!!"<<std::endl;

                //producer pool should be full now, so grab buffer
                if(pool->GetProducerPoolSize() != 0)
                {
                    buffer = pool->PopProducerBuffer();
                    return HProducerBufferPolicyCode::forced_flushed;
                }
                else
                {   
                    buffer = nullptr;
                    return HProducerBufferPolicyCode::fail;
                }
            }
        }
};


//wait for buffer until time-out is reached, then fail
template< typename XBufferItemType >
class HProducerBufferHandler_WaitWithTimeout: public HProducerBufferReleaser< XBufferItemType >
{
    public:
        HProducerBufferHandler_WaitWithTimeout():fNAttempts(100),fSleepDurationNanoSeconds(500){;};
        virtual ~HProducerBufferHandler_WaitWithTimeout(){;};

        //total time-out wait time will be fNAttempts*fSleepDurationNanoSeconds
        void SetNAttempts(unsigned int n){fNAttempts = n;};
        unsigned int GetNAttempts() const {return fNAttempts;};

        void SetSleepDurationNanoSeconds(unsigned int ns){fSleepDurationNanoSeconds = ns;};
        unsigned int GetSleepDurationNanoSeconds() const {return fSleepDurationNanoSeconds;};

        HProducerBufferPolicyCode ReserveBuffer(HBufferPool<XBufferItemType>* pool, HLinearBuffer<XBufferItemType>* buffer)
        {
            if(pool->GetProducerPoolSize() != 0)
            {
                buffer = pool->PopProducerBuffer();
                return HProducerBufferPolicyCode::success;
            }
            else
            {   
                //wait for the consumer buffer pool to become empty
                unsigned int count = 0;
                while( pool->GetProducerPoolSize() != 0 && count < fNAttempts)
                {
                    //sleep for the specified duration if it is non-zero
                    if(fSleepDurationNanoSeconds != 0)
                    {
                        std::this_thread::sleep_for(std::chrono::nanoseconds(fSleepDurationNanoSeconds));
                    }

                    if(pool->GetProducerPoolSize() != 0)
                    {
                        buffer = pool->PopProducerBuffer();
                        return HProducerBufferPolicyCode::success;
                    }

                    count++;
                };

                //producer pool should be full now, so grab buffer
                if(pool->GetProducerPoolSize() != 0)
                {
                    buffer = pool->PopProducerBuffer();
                    return HProducerBufferPolicyCode::success;
                }
                else
                {   
                    buffer = nullptr;
                    return HProducerBufferPolicyCode::fail;
                }
            }
        }

    protected: 

        unsigned int fNAttempts;
        unsigned int fSleepDurationNanoSeconds;
};

}


#endif /* end of include guard: HProducerBufferHandlerPolicy */
