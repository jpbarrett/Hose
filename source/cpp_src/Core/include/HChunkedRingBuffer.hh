#ifndef HChunkedRingBuffer_HH__
#define HChunkedRingBuffer_HH__

#include <type_traits>
#include <cstring>
#include <vector>
#include <mutex>
#include <iostream>
#include <stdexcept>

#include "HLinearBuffer.hh"


namespace hose {

/*
*File: HChunkedRingBuffer.hh
*Class: HChunkedRingBuffer
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date: Sat Feb 10 02:01:00 EST 2018
*Description: ring of data buffers
*/

template< typename XBufferItemType, typename XBufferAllocator >
class HChunkedRingBuffer
{
    static_assert(std::is_same< XBufferItemType, typename XBufferAllocator::value_type >::value, "Buffer item type and buffer allocator value_type must match.");

    public:

        HChunkedRingBuffer(XBufferAllocator* allocator):
            fAllocator(allocator),
            fNChunks(0),
            fNItemsPerChunk(0),
            fTotalItems(0),
            fAllocated(false)
        {};

        HChunkedRingBuffer(XBufferAllocator* allocator, std::size_t n_chunks, std::size_t items_per_chunk):
            fAllocator(allocator),
            fNChunks(n_chunks),
            fNItemsPerChunk(items_per_chunk),
            fTotalItems(n_chunks*items_per_chunk),
            fAllocated(false)
        {
            Allocate(fNChunks, fNItemsPerChunk);
        };

        virtual ~HChunkedRingBuffer()
        {
            Deallocate();
        };

        void Allocate(std::size_t n_chunks, std::size_t items_per_chunk)
        {
            //check that n_chucks is > 2
            //a ring buffer smaller than 2 is not useful, nor possible with
            //this class
            if( n_chunks < 2)
            {
                throw std::runtime_error("HChunkedRingBuffer::Allocate(): Cannot allocate a ring buffer with less than two elements.");
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
                fChunks.push_back( new HLinearBuffer< XBufferItemType >( chunk, fNItemsPerChunk) );
            }

            fAllocated = true;

            //init head and tail
            fHead = fChunks.begin();
            fTail = fChunks.begin();
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

        bool IsEmpty() const
        {
            std::lock_guard<std::mutex> lock(fMutex);
            return is_empty();
        }

        bool IsFull() const
        {
            std::lock_guard<std::mutex> lock(fMutex);
            return is_full();
        }

        //get pointer to the current tail buffer 
        HLinearBuffer< XBufferItemType >* Tail()
        {
            //lock to make sure we are the only thread modifying/querying the position
            //of the tail/head iterators
            std::lock_guard<std::mutex> lock(fMutex);
            //if the buffer is empty, return null
            if( is_empty() ){return nullptr;};
            return *fTail;
        }

        //return current tail and then increment tail pointer
        HLinearBuffer< XBufferItemType >* PopTail()
        {
            std::cout<<"popping tail"<<std::endl;
            //lock to make sure we are the only thread modifying/querying the position
            //of the tail/head iterators
            std::lock_guard<std::mutex> lock(fMutex);
            //increment the tail to the next position, modulo the vector size
            return *( ModuloPostIncrementTail() );
        }

        // //return current tail and then increment tail pointer
        // HLinearBuffer< XBufferItemType >* PopAndLockTail()
        // {
        //     std::cout<<"popping tail"<<std::endl;
        //     //lock to make sure we are the only thread modifying/querying the position
        //     //of the tail/head iterators
        //     std::lock_guard<std::mutex> lock(fMutex);
        //     IterType tail_it = fTail;
        // 
        //     //lock the buffer associated with the tail
        //     if( (*fTail)->fLockStack.size() != 0 )
        //     {
        //         //for now, we just throw a fatal error
        //         //however, we may want to have a policy trait decide what should be done here
        //         //i.e, wait for lock, steal the lock based on priority, etc. 
        //         throw std::runtime_error("HChunkedRingBuffer::PopAndLockHead(): Buffer lock stack not empty!");
        //     }
        //     (*fTail)->fLockStack.push( std::unique_lock<std::mutex>( (*fTail)->fMutex, std) );
        // 
        //     //increment the tail to the next position, modulo the vector size
        //     ModuloPostIncrementTail();
        //     return *(tail_it);
        // }

        //get pointer to the current head buffer
        HLinearBuffer< XBufferItemType >* Head()
        {
            //lock to make sure we are the only thread modifying/querying the position
            //of the tail/head iterators
            std::lock_guard<std::mutex> lock(fMutex);
            return *fHead;
        }

        //return current head and then increment head pointer
        HLinearBuffer< XBufferItemType >* PopHead()
        {
            std::cout<<"popping head"<<std::endl;
            //lock to make sure we are the only thread modifying/querying the position
            //of the tail/head iterators
            std::lock_guard<std::mutex> lock(fMutex);
            //pop the current head and increment

            //if the buffer was full, we also need to increment the tail 
            IterType head_it;
            if( is_full() )
            {
                head_it = ModuloPostIncrementHead();
                std::cout<<"warning, popping head when full"<<std::endl;
                //this may result in data loss, perhaps we should log it
                ModuloPostIncrementTail();
            }
            else
            {
                head_it = ModuloPostIncrementHead();
            }
            return *head_it;
        }
        // 
        // //return current head and then increment head pointer
        // HLinearBuffer< XBufferItemType >* PopAndLockHead()
        // {
        //     //lock ring buffer to make sure we are the only thread that is
        //     //modifying/querying the position of the tail/head iterators
        //     std::lock_guard<std::mutex> lock(fMutex);
        // 
        //     if( (*fHead)->fProducerLockStack.size() != 0 || (*fHead)->fConsumerLockStack.size() != 0 )
        //     {
        //         //for now, we just throw a fatal error
        //         //however, we may want to have a policy trait decide what should be done here
        //         //i.e, wait for lock, steal the lock based on priority, etc. 
        //         throw std::runtime_error("HChunkedRingBuffer::PopAndLockHead(): Buffer lock stack not empty!");
        //     }
        // 
        //     //cache the current head
        //     IterType head_it = fHead;
        //     if( is_full() )
        //     {
        //         //because we have a full buffer, we need to aquire locks
        //         //on both the head and tail buffers before we can increment them
        //         //this forces us to wait for whatever process is busy with
        //         //the current tail
        // 
        //         //buffer lock associated with head gets passed back with the buffer
        //         (*fHead)->fProducerLockStack.push( std::unique_lock<std::mutex>( (*fHead)->fMutex, std::defer_lock) );
        // 
        //         //buffer lock associated with tail gets release at the end of this scope
        //         std::unique_lock<std::mutex> lock_tail( (*fTail)->fMutex, std::defer_lock);
        //      
        //         //now lock both without deadlock
        //         std::lock( (*fHead)->fProducerLockStack.top(), lock_tail);
        // 
        //         std::cout<<"warning, popping head when full"<<std::endl;
        // 
        //         //increment head and tail in lock-step
        //         ModuloPostIncrementHead();
        //         ModuloPostIncrementTail();
        //     }
        //     else
        //     {
        //         if( (*fHead)->fLockStack.size() != 0 )
        //         {
        //             //for now, we just throw a fatal error
        //             //however, we may want to have a policy trait decide what should be done here
        //             //i.e, wait for lock, steal the lock based on priority, etc. 
        //             throw std::runtime_error("HChunkedRingBuffer::PopAndLockHead(): Buffer lock stack not empty!");
        //         }
        //         //not full, just lock and increment head ptr
        //         (*fHead)->fProducerLockStack.push( std::unique_lock<std::mutex>( (*fHead)->fMutex) );
        //         ModuloPostIncrementHead();
        //     }
        //     return *head_it;
        // }

        //increment tail pointer without returning pointer to previous tail
        void IncrementTail()
        {
            std::cout<<"popping tail"<<std::endl;
            //lock to make sure we are the only thread modifying/querying the position
            //of the tail/head iterators
            std::lock_guard<std::mutex> lock(fMutex);
            //increment the tail to the next position, modulo the vector size
            ModuloPostIncrementTail();
        }

        //increment head pointer without returning prointer to previous head
        void IncrementHead()
        {
            std::cout<<"popping head"<<std::endl;
            //lock to make sure we are the only thread modifying/querying the position
            //of the tail/head iterators
            std::lock_guard<std::mutex> lock(fMutex);
            //pop the current head and increment

            //if the buffer was full, we also need to increment the tail 
            if( is_full() )
            {
                IterType head_it = ModuloPostIncrementHead();
                std::cout<<"warning, popping head when full"<<std::endl;
                //this may result in data loss, perhaps we should log it
                ModuloPostIncrementTail();
            }
            else
            {
                IterType head_it = ModuloPostIncrementHead();
            }
        }




    protected:

        using IterType = typename std::vector< HLinearBuffer< XBufferItemType >* >::iterator;
        using ConstIterType = typename std::vector< HLinearBuffer< XBufferItemType >* >::const_iterator;

        bool is_full() const
        {
            //test modulo increment of head to see if it ends up on the tail
            ConstIterType head_it = fHead;
            if(++head_it == fChunks.end()){head_it = fChunks.begin();};
            return (fTail == head_it);
        }

        bool is_empty() const
        {
            return (fTail == fHead);
        }


        IterType ModuloPostIncrementTail()
        {
            IterType tail_it = fTail;
            if(++fTail == fChunks.end()){fTail = fChunks.begin();};
            return tail_it;
        }

        IterType ModuloPostIncrementHead()
        {
            IterType head_it = fHead;
            if(++fHead == fChunks.end()){fHead = fChunks.begin();};
            return head_it;
        }

        //allocator
        XBufferAllocator* fAllocator;

        //data
        std::size_t fNChunks;
        std::size_t fNItemsPerChunk;
        std::size_t fTotalItems;
        bool fAllocated;

        //list of the buffer chunks 
        std::vector< HLinearBuffer< XBufferItemType >* > fChunks;

        //head/tail of the ring buffer
        typename std::vector< HLinearBuffer< XBufferItemType >* >::iterator fHead;
        typename std::vector< HLinearBuffer< XBufferItemType >* >::iterator fTail;

        //head/tail modification mutex
        mutable std::mutex fMutex;

};

}

#endif /* end of include guard: HChunkedRingBuffer */
