#ifndef HConsumer_HH__
#define HConsumer_HH__

#include <mutex>
#include <thread>

#include "HBufferPool.hh"
#include "HThreadPool.hh"

namespace hose
{

/*
*File: HConsumer.hh
*Class: HConsumer
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

template< typename XBufferItemType > 
class HConsumer: public HThreadPool
{
    public:

        HConsumer():
            HThreadPool(),
            fStopConsumption(false),
            fThread(),
            fBufferPool(nullptr)
        {};

        virtual ~HConsumer(){};

        void SetBufferPool(HBufferPool<XBufferItemType>* buffer_pool);
        HBufferPool<XBufferItemType>* GetBufferPool() {return fBufferPool;};
        const HBufferPool<XBufferItemType>* GetBufferPool() const {return fBufferPool;};

        //start the producer running (in a separate thread in the background)
        void Start()
        {
            //launch the work threads in the thread pool
            Launch();

            //now run the management thread (responsible for generation of work for the thread pool)
            fStopConsumption = false;
            fThread = std::thread(&HConsumer::ConsumeWork,this);
        }

        //stop the producer and join thread
        void Stop()
        {
            //signal termination to thread pool
            SignalTerminateOnComplete();

            //signal and stop the management thread
            fStopConsumption = true;
            fThread.join();

            //join the thread pool
            Join();
        }

        //stop the producer and join thread
        void ForceStop()
        {
            //kill the thread pool
            ForceTermination();

            //signal and stop the management thread
            fStopConsumption = true;
            fThread.join();

            //join the thread pool
            Join();
        }

    protected:

        void ProduceWork()
        {
            while(!fStopConsumption)
            {
                ManageWork();
            }
        }

        virtual void ManageWork() = 0;

        bool fStopConsumption;
        std::thread fManagementThread;
        HBufferPool<XBufferItemType>* fBufferPool;


};

}

#endif /* end of include guard: HConsumer */
