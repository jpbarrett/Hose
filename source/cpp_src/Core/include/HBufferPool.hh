#ifndef HBufferPool_HH__
#define HBufferPool_HH__

#include <type_traits>
#include <cstring>
#include <vector>
#include <queue>
#include <mutex>
#include <iostream>
#include <stdexcept>

#include "HLinearBuffer.hh"
#include "HBufferAllocatorBase.hh"

namespace hose {

/*
*File: HBufferPool.hh
*Class: HBufferPool
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date: Sat Feb 10 02:01:00 EST 2018
*Description: ring of data buffers
*/

template< typename XBufferItemType >
class HBufferPool
{
    public:

        HBufferPool(HBufferAllocatorBase< XBufferItemType >* allocator):
            fAllocator(allocator),
            fNChunks(0),
            fNItemsPerChunk(0),
            fTotalItems(0),
            fAllocated(false)
        {};

        HBufferPool(HBufferAllocatorBase< XBufferItemType >* allocator, std::size_t n_chunks, std::size_t items_per_chunk):
            fAllocator(allocator),
            fNChunks(n_chunks),
            fNItemsPerChunk(items_per_chunk),
            fTotalItems(n_chunks*items_per_chunk),
            fAllocated(false)
        {
            Allocate(fNChunks, fNItemsPerChunk);
        };

        virtual ~HBufferPool()
        {
            Deallocate();
        };

        void Allocate(std::size_t n_chunks, std::size_t items_per_chunk)
        {
            if( n_chunks < 1)
            {
                throw std::runtime_error("HBufferPool::Allocate(): Cannot allocate a ring buffer with less than one element.");
            }

            std::lock_guard<std::mutex> lock(fMutex);
            if(fAllocated){Deallocate();}

            fNChunks = n_chunks;
            fNItemsPerChunk = items_per_chunk;
            fTotalItems = fNChunks*fNItemsPerChunk;

            //for each chunk, call the allocator and create a linear buffer
            fChunks.resize(0);
            for(size_t i=0; i<fNChunks; i++ )
            {
                auto chunk = fAllocator->allocate( fNItemsPerChunk ); //TODO, deal with exceptions
                std::cout<<"chunk ptr = "<<chunk<<std::endl;
                if(chunk != nullptr)
                {
                    fChunks.push_back( new HLinearBuffer< XBufferItemType >( chunk, fNItemsPerChunk) );
                }
            }

            fAllocated = true;

            for(size_t i=0; i<fNChunks; i++ )
            {
                fProducerQueue.push( fChunks[i] );
            }

        }

        void Deallocate()
        {
            std::lock_guard<std::mutex> lock(fMutex);
            if(fAllocated)
            {
                for(size_t i=0; i<fChunks.size(); i++)
                {
                    fAllocator->deallocate( fChunks[i]->GetData(), fNItemsPerChunk);
                }
                fChunks.resize(0);
            }
            fAllocated = false;
        }

        bool IsAllocated() const { return fAllocated; }

        size_t GetConsumerPoolSize() const
        {
            std::lock_guard<std::mutex> lock(fMutex);
            return fConsumerQueue.size();
        }

        size_t GetProducerPoolSize() const 
        {
            std::lock_guard<std::mutex> lock(fMutex);
            return fProducerQueue.size();
        }

        //pop a buffer off of the producer queue for use by a producer 
        HLinearBuffer< XBufferItemType >* PopProducerBuffer()
        {
            //lock the buffer pool, so more than one thread cannot grab the same buffer
            std::lock_guard<std::mutex> lock(fMutex);
            HLinearBuffer< XBufferItemType >* buff = nullptr;
            if(fProducerQueue.size() != 0 )
            {
                buff = fProducerQueue.front();
                fProducerQueue.pop();
            }
            return buff;
        }

        //return a buffer to the producer queue
        void PushProducerBuffer(HLinearBuffer< XBufferItemType >* buff)
        {
            //for now, we do not check if the buff is an actual member of this pool
            //TODO: determine if this check is needed

            //lock the buffer pool, so more than one thread modify the queue
            std::lock_guard<std::mutex> lock(fMutex);
            fProducerQueue.push(buff);
        }


        //pop a buffer off of the consumer queue for use by a consumer
        HLinearBuffer< XBufferItemType >* PopConsumerBuffer()
        {
            //lock the buffer pool, so more than one thread cannot grab the same buffer
            std::lock_guard<std::mutex> lock(fMutex);
            HLinearBuffer< XBufferItemType >* buff = nullptr;
            if(fConsumerQueue.size() != 0 )
            {
                buff = fConsumerQueue.front();
                fConsumerQueue.pop();
            }
            return buff;
        }

        //return a buffer to the consumer queue
        void PushConsumerBuffer(HLinearBuffer< XBufferItemType >* buff)
        {
            //for now, we do not check if the buff is an actual member of this pool
            //TODO: determine if this check is needed

            //lock the buffer pool, so more than one thread modify the queue
            std::lock_guard<std::mutex> lock(fMutex);
            fConsumerQueue.push(buff);
        }

    protected:

        //allocator
        HBufferAllocatorBase< XBufferItemType >* fAllocator;

        //data
        std::size_t fNChunks;
        std::size_t fNItemsPerChunk;
        std::size_t fTotalItems;
        bool fAllocated;

        //complete list of the buffer chunks 
        std::vector< HLinearBuffer< XBufferItemType >* > fChunks;

        //FIFO queue's of the buffers, for producer/consumer use
        std::queue< HLinearBuffer< XBufferItemType >* > fProducerQueue;
        std::queue< HLinearBuffer< XBufferItemType >* > fConsumerQueue;

        //modification mutex
        mutable std::mutex fMutex;

};

}

#endif /* end of include guard: HBufferPool */
