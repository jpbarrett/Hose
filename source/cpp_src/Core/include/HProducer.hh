#ifndef HProducer_HH__
#define HProducer_HH__

#include <mutex>
#include <thread>

#include "HBufferPool.hh"
#include "HThreadPool.hh"
#include "HProducerBufferHandlerPolicy.hh"

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

template< typename XBufferItemType, typename XProducerBufferHandlerPolicyType > 
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
            ConfigureBufferHandler(fBufferHandler);
            ExecutePreProductionTasks();

            while(!fStopProduction) 
            {
                //prepare things as needed (reserve buffer, etc)
                ExecutePreWorkTasks(status, buffer);

                //spawn off work associated with this buffer
                GenerateWork(status, buffer);

                //do post-work tasks (release the buffer, etc)
                ExecutePostWorkTasks(status, buffer);
            }

            ExecutePostProductionTasks();

        }

        virtual void ExecutePreProductionTasks(){};
        virtual void ExecutePostProductionTasks();

        virtual void ConfigureBufferHandler(XProducerBufferHandlerPolicyType& /*handler*/){};
        virtual void ExecutePreWorkTasks(){};
        virtual void GenerateWork(){};
        virtual void ExecutePostWorkTasks(){};

        bool fStopProduction;
        std::thread fManagementThread;
        HBufferPool<XBufferItemType>* fBufferPool;
        XProducerBufferHandlerPolicyType fBufferHandler;


};

}

#endif /* end of include guard: HProducer */
