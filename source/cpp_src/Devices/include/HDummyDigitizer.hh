#ifndef HDummyDigitizer_HH__
#define HDummyDigitizer_HH__

#include <random>
#include <limits>
#include <queue>
#include <stdint.h>

#include "HDigitizer.hh"

namespace hose {

/*
*File: HDummyDigitizer.hh
*Class: HDummyDigitizer
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: Test harness for digitizer interface, generates random numbers
*/

class HDummyDigitizer: public HDigitizer< uint16_t, HDummyDigitizer >
{
    public:
        HDummyDigitizer();
        virtual ~HDummyDigitizer();

    protected:

        friend class HDigitizer< uint16_t, HDummyDigitizer >;

        //required by digitizer interface
        bool InitializeImpl();
        void AcquireImpl();
        void TransferImpl();
        int FinalizeImpl();
        void StopImpl();
        void TearDownImpl();

        void fill_function();

        //random number generator
        std::random_device* fRandom;
        std::mt19937* fGenerator;
        std::uniform_int_distribution<uint16_t>* fUniformDistribution;

        //'samples' counter
        uint64_t fCounter;
        std::time_t fAcquisitionStartTime;


};

}

#endif /* end of include guard: HDummyDigitizer */
