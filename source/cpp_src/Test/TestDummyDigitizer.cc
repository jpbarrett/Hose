#include <iostream>
#include <vector>
#include <memory>


#include "HBufferAllocatorNew.hh"
#include "HDummyDigitizer.hh"
#include "HChunkedRingBuffer.hh"

using namespace hose;


int main(int /*argc*/, char** /*argv*/)
{
    //dummy digitizer
    HDummyDigitizer dummy;
    bool initval = dummy.Initialize();

    //create ring buffer
    HChunkedRingBuffer< HDummyDigitizer::sample_type > ring( dummy.GetAllocator() );

    //allocate space
    const size_t n_chunks = 3;
    const size_t items_per_chunk = 5;
    ring.Allocate(n_chunks, items_per_chunk);

    const unsigned int n_iter = 7;
    
    for(unsigned int j=0; j<n_iter; j++)
    {
        std::cout<<"iter = "<<j<<std::endl;

        std::cout<<"head1 = "<<ring.Head()<<std::endl;
        std::cout<<"tail1 = "<<ring.Tail()<<std::endl;

        dummy.SetBuffer(ring.Head());
        dummy.Acquire();
        dummy.Transfer();
        dummy.Finalize();
        ring.IncrementHead();

        std::cout<<"head2 = "<<ring.Head()<<std::endl;
        std::cout<<"tail2 = "<<ring.Tail()<<std::endl;

        //on even counts, add 1 extra to the acquisition
        if( j%2 == 0)
        {
            dummy.SetBuffer(ring.Head());
            dummy.Acquire();
            dummy.Transfer();
            dummy.Finalize();
            ring.IncrementHead();

            std::cout<<"head3 = "<<ring.Head()<<std::endl;
            std::cout<<"tail3 = "<<ring.Tail()<<std::endl;
        }

        HLinearBuffer< HDummyDigitizer::sample_type >* tail = ring.Tail();

        for(size_t i=0; i<items_per_chunk; i++)
        {
            std::cout<<(*tail)[i]<<", ";
        }
        std::cout<<std::endl;
        ring.IncrementTail();

        std::cout<<"head4 = "<<ring.Head()<<std::endl;
        std::cout<<"tail4 = "<<ring.Tail()<<std::endl;


        //on odd counts add one extra to the print out
        if( j%2 != 0)
        {
            for(size_t i=0; i<items_per_chunk; i++)
            {
                std::cout<<(*tail)[i]<<", ";
            }
            std::cout<<std::endl;
            ring.IncrementTail();

            std::cout<<"head5 = "<<ring.Head()<<std::endl;
            std::cout<<"tail5 = "<<ring.Tail()<<std::endl;


        }




        std::cout<<"------------------"<<std::endl;

    }
    // 
    // dummy.SetBuffer(ring.Head());
    // dummy.Acquire();
    // dummy.Transfer();
    // dummy.Finalize();
    // ring.IncrementHead();
    // 
    // tail = ring.Tail();
    // 
    // for(size_t i=0; i<items_per_chunk; i++)
    // {
    //     std::cout<<(*tail)[i]<<", ";
    // }
    // std::cout<<std::endl;
    // ring.IncrementTail();

    return 0;
}
