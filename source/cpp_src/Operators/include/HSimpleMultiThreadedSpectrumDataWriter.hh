#ifndef HSimpleMultiThreadedSpectrumDataWriter_HH__
#define HSimpleMultiThreadedSpectrumDataWriter_HH__

#include <iostream>
#include <ostream>
#include <ios>
#include <fstream>
#include <sstream>
#include <thread>

#include "HLinearBuffer.hh"
#include "HBufferPool.hh"
#include "HConsumer.hh"
#include "HSpectrumObject.hh"

#include "spectrometer.h"

namespace hose
{

/*
*File: HSimpleMultiThreadedSpectrumDataWriter.hh
*Class: HSimpleMultiThreadedSpectrumDataWriter
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

class HSimpleMultiThreadedSpectrumDataWriter: public HConsumer< spectrometer_data, HConsumerBufferHandler_WaitWithTimeout< spectrometer_data > >
{
    public:

        HSimpleMultiThreadedSpectrumDataWriter();
        virtual ~HSimpleMultiThreadedSpectrumDataWriter();

        void SetOutputDirectory(std::string output_dir);
        std::string GetOutputDirectory() const {return fOutputDirectory;};

    private:

        virtual void ExecuteThreadTask() override;
        virtual bool WorkPresent() override;
        virtual void Idle() override;

        std::string fOutputDirectory;

};

}

#endif /* end of include guard: HSimpleMultiThreadedSpectrumDataWriter */
