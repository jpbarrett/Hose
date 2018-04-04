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
enum class ProducerBufferPolicyCode
{
    fail, //failed to reserve buffer
    success, //successfully reserved (or released) a buffer
    stolen, //successfully stole a buffer
    timeout_fail, //failed to reserve a buffer after timing-out
    flushed, //flushed buffer queue, and then successfully reserved a buffer
    forced_flushed //forcefully flushed buffer queue, then successfully reserved a buffer
}

//base class for releasing buffers (common to all handler policies)
template< XBufferItemType >
class HProducerBufferReleaser
{
    public:
        HProducerBufferReleaser(){;};
        virtual ~HProducerBufferReleaser(){;};

        ProducerBufferPolicyCode ReleaseBufferToProducer(HBufferPool<XBufferItemType>* pool, HLinearBuffer<XBufferItemType>* buffer)
        {
            pool->PushProducerBuffer(buffer);
            return ProducerBufferPolicyCode::success;
        }

        ProducerBufferPolicyCode ReleaseBufferToConsumer(HBufferPool<XBufferItemType>* pool, HLinearBuffer<XBufferItemType>* buffer)
        {
            pool->PushConsumerBuffer(buffer);
            return ProducerBufferPolicyCode::success;
        }
};


//get a buffer immediately, if no buffer is available fail and return nullptr
template< XBufferItemType >
class HProducerBufferHander_Immediate: HProducerBufferReleaser< XBufferItemType >
{
    public:
        HProducerBufferHander_Immediate(){;};
        virtual ~HProducerBufferHander_Immediate(){;};

        ProducerBufferPolicyCode ReserveBuffer(HBufferPool<XBufferItemType>* pool, HLinearBuffer<XBufferItemType>* buffer)
        {
            if(pool->GetProducerPoolSize() != 0)
            {
                buffer = pool->PopProducerBuffer();
                return ProducerBufferPolicyCode::success;
            }
            else
            {   
                buffer = nullptr;
                return ProducerBufferPolicyCode::fail;
            }
        }
};

//wait indefinitely until a buffer is available
template< XBufferItemType >
class HProducerBufferHander_Wait: HProducerBufferReleaser< XBufferItemType >
{
    public:
        HProducerBufferHander_Wait(){;};
        virtual ~HProducerBufferHander_Wait(){;};

        ProducerBufferPolicyCode ReserveBuffer(HBufferPool<XBufferItemType>* pool, HLinearBuffer<XBufferItemType>* buffer)
        {
            while(true)
            {
                if(pool->GetProducerPoolSize() != 0)
                {
                    buffer = pool->PopProducerBuffer();
                    return ProducerBufferPolicyCode::success;
                }
            };
        }

};


//steal a (unconsumed) consumer buffer to give to the producer
template< XBufferItemType >
class HProducerBufferHander_Steal: HProducerBufferReleaser< XBufferItemType >
{
    public:
        HProducerBufferHander_Steal(){;};
        virtual ~HProducerBufferHander_Steal(){;};

        ProducerBufferPolicyCode ReserveBuffer(HBufferPool<XBufferItemType>* pool, HLinearBuffer<XBufferItemType>* buffer)
        {
            if(pool->GetProducerPoolSize() != 0)
            {
                buffer = pool->PopProducerBuffer();
                return ProducerBufferPolicyCode::success;
            }
            else
            {   
                if(pool->GetConsumerPoolSize() != 0)
                {
                    buffer = pool->PopConsumerBuffer();
                    return ProducerBufferPolicyCode::stolen;
                }
                else
                {
                    buffer = nullptr;
                    return ProducerBufferPolicyCode::fail;
                }
            }
        }

};

//wait indefinitely for all consumer buffers to be freed for production, before returning the next buffer
template< XBufferItemType >
class HProducerBufferHander_Flush: HProducerBufferReleaser< XBufferItemType >
{
    public:
        HProducerBufferHander_Flush():fDurationNanoSeconds(500){;};
        virtual ~HProducerBufferHander_Flush(){;};

        void SetSleepDurationNanoSeconds(unsigned int ns){fDurationNanoSeconds = ns;};
        unsigned int GetSleepDurationNanoSeconds() const {return fDurationNanoSeconds};

        ProducerBufferPolicyCode ReserveBuffer(HBufferPool<XBufferItemType>* pool, HLinearBuffer<XBufferItemType>* buffer)
        {
            if(pool->GetProducerPoolSize() != 0)
            {
                buffer = pool->PopProducerBuffer();
                return ProducerBufferPolicyCode::success;
            }
            else
            {   
                //wait for the consumer buffer pool to become empty
                while( pool->GetConsumerPoolSize() != 0 )
                {
                    //sleep for the specified duration if it is non-zero
                    if(fDurationNanoSeconds != 0)
                    {
                        std::this_thread::sleep_for(std::chrono::nanoseconds(fDurationNanoSeconds));
                    }
                };

                //producer pool should be full now, so grab buffer
                if(pool->GetProducerPoolSize() != 0)
                {
                    buffer = pool->PopProducerBuffer();
                    return ProducerBufferPolicyCode::flushed;
                }
                else
                {   
                    buffer = nullptr;
                    return ProducerBufferPolicyCode::fail;
                }
            }
        }

    protected: 

        unsigned int fDurationNanoSeconds;

};

//forcefully release all un-reserved consumer buffers for production, then return the next buffer
template< XBufferItemType >
class HProducerBufferHander_ForceFlush: HProducerBufferReleaser< XBufferItemType >
{
    public:
        HProducerBufferHander_ForceFlush(){;};
        virtual ~HProducerBufferHander_ForceFlush(){;};

        ProducerBufferPolicyCode ReserveBuffer(HBufferPool<XBufferItemType>* pool, HLinearBuffer<XBufferItemType>* buffer)
        {
            if(pool->GetProducerPoolSize() != 0)
            {
                buffer = pool->PopProducerBuffer();
                return ProducerBufferPolicyCode::success;
            }
            else
            {
                //flush out all of the consumer buffers back into the producer pool w/o consuming them
                while( pool->GetConsumerPoolSize() != 0 )
                {
                    HLinearBuffer< XBufferItemType > temp_buffer = pool->PopConsumerBuffer();
                    pool->PushProducerBuffer(temp_buffer);
                };

                //producer pool should be full now, so grab buffer
                if(pool->GetProducerPoolSize() != 0)
                {
                    buffer = pool->PopProducerBuffer();
                    return ProducerBufferPolicyCode::forced_flushed;
                }
                else
                {   
                    buffer = nullptr;
                    return ProducerBufferPolicyCode::fail;
                }
            }
        }
};


//wait for buffer until time-out is reached, then fail
template< XBufferItemType >
class HProducerBufferHander_WaitWithTimeout: HProducerBufferReleaser< XBufferItemType >
{
    public:
        HProducerBufferHander_WaitWithTimeout():fNAttempts(100),fDurationNanoSeconds(500){;};
        virtual ~HProducerBufferHander_WaitWithTimeout(){;};

        //total time-out wait time will be fNAttempts*fDurationNanoSeconds
        void SetNAttempts(unsigned int n){fNAttempts = n;};
        unsigned int GetNAttempts() const {return fNAttempts;};

        void SetSleepDurationNanoSeconds(unsigned int ns){fDurationNanoSeconds = ns;};
        unsigned int GetSleepDurationNanoSeconds() const {return fDurationNanoSeconds};

        ProducerBufferPolicyCode ReserveBuffer(HBufferPool<XBufferItemType>* pool, HLinearBuffer<XBufferItemType>* buffer)
        {
            if(pool->GetProducerPoolSize() != 0)
            {
                buffer = pool->PopProducerBuffer();
                return ProducerBufferPolicyCode::success;
            }
            else
            {   
                //wait for the consumer buffer pool to become empty
                unsigned int count = 0;
                while( pool->GetProducerPoolSize() != 0 && count < fNAttempts)
                {
                    //sleep for the specified duration if it is non-zero
                    if(fDurationNanoSeconds != 0)
                    {
                        std::this_thread::sleep_for(std::chrono::nanoseconds(fDurationNanoSeconds));
                    }

                    if(pool->GetProducerPoolSize() != 0)
                    {
                        buffer = pool->PopProducerBuffer();
                        return ProducerBufferPolicyCode::success;
                    }

                    count++;
                };

                //producer pool should be full now, so grab buffer
                if(pool->GetProducerPoolSize() != 0)
                {
                    buffer = pool->PopProducerBuffer();
                    return ProducerBufferPolicyCode::success;
                }
                else
                {   
                    buffer = nullptr;
                    return ProducerBufferPolicyCode::fail;
                }
            }
        }

    protected: 

        unsigned int fNAttempts;
        unsigned int fDurationNanoSeconds;
};

}


#endif /* end of include guard: HProducerBufferHandlerPolicy */
