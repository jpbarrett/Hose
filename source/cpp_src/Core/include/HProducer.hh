#ifndef HProducer_HH__
#define HProducer_HH__

#include <mutex>
#include <thread>

#include "HBufferPool.hh"
#include "HThreadPool.hh"

namespace hose
{

/*
*File: HProducer.hh
*Class: HProducer
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

template< typename XBufferItemType, typename XProducerBufferHandlerPolicy > 
class HProducer: public HThreadPool
{
    public:

        HProducer():
            HThreadPool(),
            fStopProduction(false),
            fThread(),
            fBufferPool(nullptr)
        {};

        virtual ~HProducer(){};

        void SetBufferPool(HBufferPool<XBufferItemType>* buffer_pool){fBufferPool = buffer_pool;};
        HBufferPool<XBufferItemType>* GetBufferPool() {return fBufferPool;};
        const HBufferPool<XBufferItemType>* GetBufferPool() const {return fBufferPool;};

        //start the producer running (in a separate thread in the background)
        void StartProduction()
        {
            //launch the work threads in the thread pool
            Launch();

            //now run the management thread (responsible for generation of work for the thread pool)
            fStopProduction = false;
            fThread = std::thread(&HProducer::ProduceWork,this);
        }

        //stop the producer and join thread
        void StopProduction()
        {
            //signal termination to thread pool
            SignalTerminateOnComplete();

            //signal and stop the management thread
            fStopProduction = true;
            fThread.join();

            //join the thread pool
            Join();
        }

        //stop the producer and join thread
        void ForceStopProduction()
        {
            //kill the thread pool
            ForceTermination();

            //signal and stop the management thread
            fStopProduction = true;
            fThread.join();

            //join the thread pool
            Join();
        }

    protected:

        void ProduceWork()
        {
            XProducerBufferHandlerPolicy bufferHandler;
            HLinearBuffer< XBufferItemType >* buff = nullptr;
            //at some point we need to implement error/exception handling...for now we allow things to fail ungracefully
            while(!fStopProduction) 
            {
                //ask the handler for a buffer
                bufferHandler.ReserveBuffer(fBufferPool, buff);
                if(buff != nullptr)
                {
                    ExecutePreWorkTasks(buff);
                    GenerateWork(buff);
                    ExecutePostWorkTasks(buff);
                    bufferHandler.ReleaseBuffer(fBufferPool, buff);
                }
            }
        }

        virtual void ExecutePreWorkTasks(HLinearBuffer<XBufferItemType>* /*buff*/){};
        virtual void GenerateWork(HLinearBuffer<XBufferItemType>* /*buff*/) = 0;
        virtual void ExecutePostWorkTasks(HLinearBuffer<XBufferItemType>* /*buff*/){};

        bool fStopProduction;
        std::thread fManagementThread;
        HBufferPool<XBufferItemType>* fBufferPool;


};

}

#endif /* end of include guard: HProducer */
