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

class HSimpleMultiThreadedSpectrumDataWriter: public HConsumer< spectrometer_data, HConsumerBufferHandler_Immediate< spectrometer_data > >
{
    public:

        HSimpleMultiThreadedSpectrumDataWriter();
        virtual ~HSimpleMultiThreadedSpectrumDataWriter();

        void SetExperimentName(std::string exp_name);
        void SetSourceName(std::string source_name);
        void SetScanName(std::string scan_name)

        void InitializeOutputDirectory();

        void SetBaseOutputDirectory(std::string output_dir);
        std::string GetBaseOutputDirectory() const {return fBaseOutputDirectory;};

    private:

        virtual void ExecuteThreadTask() override;
        virtual bool WorkPresent() override;
        virtual void Idle() override;

        std::string fBaseOutputDirectory;

        std::string fExperimentName;
        std::string fSourceName;
        std::string fScanName;

};

}

#endif /* end of include guard: HSimpleMultiThreadedSpectrumDataWriter */
