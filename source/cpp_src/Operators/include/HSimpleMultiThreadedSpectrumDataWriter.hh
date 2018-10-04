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

extern "C" 
{
    #include "HSpectrumFile.h"
    #include "HNoisePowerFile.h"
}

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

        void SetExperimentName(std::string exp_name)
        {
            //truncate experiment name if needed
            if(exp_name.size() < HNAME_WIDTH){fExperimentName = exp_name;}
            else{fExperimentName = exp_name.substr(0,HNAME_WIDTH-1);}
        };
        
        void SetSourceName(std::string source_name)
        {
            if(source_name.size() < HNAME_WIDTH){fSourceName = source_name;}
            else{fSourceName = source_name.substr(0,HNAME_WIDTH-1);}
        };
        
        void SetScanName(std::string scan_name)
        {
            if(scan_name.size() < HNAME_WIDTH){fScanName = scan_name;}
            else{fScanName = scan_name.substr(0,HNAME_WIDTH-1);}
        };

        void InitializeOutputDirectory();

        void SetBaseOutputDirectory(std::string output_dir);
        std::string GetBaseOutputDirectory() const {return fBaseOutputDirectory;};

        std::string GetCurrentOutputDirectory() const {return fCurrentOutputDirectory;};

    private:

        virtual void ExecuteThreadTask() override;
        virtual bool WorkPresent() override;
        virtual void Idle() override;

        std::string fBaseOutputDirectory;
        std::string fCurrentOutputDirectory;

        std::string fExperimentName;
        std::string fSourceName;
        std::string fScanName;

};

}

#endif /* end of include guard: HSimpleMultiThreadedSpectrumDataWriter */
