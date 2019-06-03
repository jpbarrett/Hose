#ifndef HSimpleMultiThreadedSpectrumDataWriterSigned_HH__
#define HSimpleMultiThreadedSpectrumDataWriterSigned_HH__

#include <iostream>
#include <ostream>
#include <ios>
#include <fstream>
#include <sstream>
#include <thread>

#include "HLinearBuffer.hh"
#include "HBufferPool.hh"
#include "HConsumer.hh"
#include "HDirectoryWriter.hh"

#include "spectrometer.h"

extern "C"
{
    #include "HBasicDefines.h"
    #include "HSpectrumFile.h"
}

namespace hose
{

/*
*File: HSimpleMultiThreadedSpectrumDataWriterSigned.hh
*Class: HSimpleMultiThreadedSpectrumDataWriterSigned
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

class HSimpleMultiThreadedSpectrumDataWriterSigned: public HConsumer< spectrometer_data_s, HConsumerBufferHandler_Immediate< spectrometer_data_s > >, public HDirectoryWriter
{
    public:

        HSimpleMultiThreadedSpectrumDataWriterSigned();
        virtual ~HSimpleMultiThreadedSpectrumDataWriterSigned();

    private:

        virtual void ExecuteThreadTask() override;
        virtual bool WorkPresent() override;
        virtual void Idle() override;


};

}

#endif /* end of include guard: HSimpleMultiThreadedSpectrumDataWriterSigned */
