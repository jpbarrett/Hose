#ifndef HConsumerProducer_HH__
#define HConsumerProducer_HH__

#include <mutex>
#include <thread>

#include "HBufferPool.hh"
#include "HThreadPool.hh"
#include "HConsumerBufferHandlerPolicy.hh"
#include "HProducerBufferHandlerPolicy.hh"

namespace hose
{

/*
*File: HConsumerProducer.hh
*Class: HConsumerProducer
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:fManagementThread
*Description:
*/

template< typename XSourceBufferItemType, typename XSinkBufferItemType, typename XProducerSinkBufferHandlerPolicyType, typename XConsumerSourceBufferHandlerPolicyType> 
class HConsumerProducer: public HThreadPool
{
    public:

        HConsumerProducer():
            HThreadPool(),
            fStopConsumptionProduction(false),
            fManagementThread(),
            fSourceBufferPool(nullptr),
            fSinkBufferPool(nullptr)
        {};

        virtual ~HConsumerProducer(){};

        void SetSourceBufferPool(HBufferPool<XSourceBufferItemType>* buffer_pool){fSourceBufferPool = buffer_pool;};
        HBufferPool<XSourceBufferItemType>* GetSourceBufferPool() {return fSourceBufferPool;};
        const HBufferPool<XSourceBufferItemType>* GetSourceBufferPool() const {return fSourceBufferPool;};

        void SetSinkBufferPool(HBufferPool<XSinkBufferItemType>* buffer_pool){fSinkBufferPool = buffer_pool;};
        HBufferPool<XSinkBufferItemType>* GetSinkBufferPool() {return fSinkBufferPool;};
        const HBufferPool<XSinkBufferItemType>* GetSinkBufferPool() const {return fSinkBufferPool;};

        //start the producer running (in a separate thread in the background)
        void StartConsumptionProduction()
        {
            //launch the work threads in the thread pool
            Launch();

            //now run the management thread (responsible for generation of work for the thread pool)
            fStopConsumptionProduction = false;
            fManagementThread = std::thread(&HConsumerProducer::ConsumeProduceWork,this);
        }

        //stop the producer and join thread
        void StopConsumptionProduction()
        {
            //signal termination to thread pool
            std::cout<<"sending terminate signal"<<std::endl;
            SignalTerminateOnComplete();
            usleep(10000);
            std::cout<<"term signal = "<<fSignalTerminate<<std::endl;

            //signal and stop the management thread

            std::cout<<"waiting to join management thread"<<std::endl;
            fStopConsumptionProduction = true;
            fManagementThread.join();

            std::cout<<"manager joined"<<std::endl;


            std::cout<<"waiting for work threads"<<std::endl;
            //join the thread pool
            Join();

            std::cout<<"consumer/producer has stopped"<<std::endl;
        }

        //stop the producer and join thread
        void ForceStopConsumptionProduction()
        {
            //kill the thread pool
            ForceTermination();
            usleep(10000);

            //signal and stop the management thread
            fStopConsumptionProduction = true;
            fManagementThread.join();

            //join the thread pool
            Join();
        }

    protected:

        void ConsumeProduceWork()
        {
            // ConfigureSinkBufferHandler(fSinkBufferHandler);
            // ConfigureSourceBufferHandler(fSourceBufferHandler);

            ExecutePreConsumptionProductionTasks();

            while(!fStopConsumptionProduction) 
            {
                //prepare things as needed (reserve buffer, etc)
                ExecutePreWorkTasks();

                //spawn off work associated with this buffer
                DoWork();

                //do post-work tasks (release the buffer, etc)
                ExecutePostWorkTasks();
            }

            ExecutePostConsumptionProductionTasks();
        }

        virtual void ExecutePreConsumptionProductionTasks(){};
        virtual void ExecutePostConsumptionProductionTasks(){};

        // virtual void ConfigureSinkBufferHandler(XProducerSinkBufferHandlerPolicyType& /*handler*/){};
        // virtual void ConfigureSourceBufferHandler(XConsumerSourceBufferHandlerPolicyType& /*handler*/){};
        virtual void ExecutePreWorkTasks(){};
        virtual void DoWork(){}; //override if thread tasks are not independent
        virtual void ExecutePostWorkTasks(){};

        volatile bool fStopConsumptionProduction;
        std::thread fManagementThread;
        HBufferPool<XSourceBufferItemType>* fSourceBufferPool;
        HBufferPool<XSinkBufferItemType>* fSinkBufferPool;
        XProducerSinkBufferHandlerPolicyType fSourceBufferHandler;
        XConsumerSourceBufferHandlerPolicyType fSinkBufferHandler;


};

}

#endif /* end of include guard: HConsumerProducer */
