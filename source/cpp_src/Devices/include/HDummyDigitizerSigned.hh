#ifndef HDummyDigitizerSigned_HH__
#define HDummyDigitizerSigned_HH__

#include <random>
#include <limits>
#include <queue>
#include <stdint.h>

#include "HDigitizer.hh"

namespace hose {

/*
*File: HDummyDigitizerSigned.hh
*Class: HDummyDigitizerSigned
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: Test harness for digitizer interface, generates random numbers
*/

class HDummyDigitizerSigned: public HDigitizer< int16_t, HDummyDigitizerSigned >
{
    public:
        HDummyDigitizerSigned();
        virtual ~HDummyDigitizerSigned();

    protected:

        friend class HDigitizer< int16_t, HDummyDigitizerSigned >;

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
        std::uniform_int_distribution<int16_t>* fUniformDistribution;

        //'samples' counter and aquire time stamp
        uint64_t fCounter;
        std::time_t fAcquisitionStartTime;

};

}

#endif /* end of include guard: HDummyDigitizerSigned */
