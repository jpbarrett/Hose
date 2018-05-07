#ifndef HBufferMetaData_HH__
#define HBufferMetaData_HH__

#include <string>
#include <vector>
#include <stdint.h>

#include "HDataAccumulation.hh"

/*
*File: HBufferMetaData.hh
*Class: HBufferMetaData
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: place holder class for buffer meta data, should implement an 'extensible' struct 
*/

class HBufferMetaData
{
    public:

        HBufferMetaData():
            fAcquisitionStartSecond(0),
            fSampleIndex(0),
            fSampleRate(0),
            fValidLength(0)
        {};

        virtual ~HBufferMetaData(){};

        virtual std::string GetName() const {return std::string("BasicMetaData");}

        //the integer second (since the epoch) at which the aqcuisition was started
        uint64_t GetAcquisitionStartSecond() const {return fAcquisitionStartSecond;};
        void SetAcquisitionStartSecond(const uint64_t& sec) {fAcquisitionStartSecond = sec;};

        //sample index relative to start of the aqcuisition
        uint64_t GetLeadingSampleIndex() const {return fSampleIndex;};
        void SetLeadingSampleIndex(const uint64_t& index){fSampleIndex = index;};

        //TODO remove me! this is only useful for the px14 internal buffer
        //we really need to implement some sort of extensible struct functionality
        uint64_t GetValidLength() const {return fValidLength;};
        void SetValidLength(const uint64_t& len){fValidLength = len;};

        //sampling rate of the digitizer
        uint64_t GetSampleRate() const {return fSampleRate;};
        void SetSampleRate(const uint64_t& rate) {fSampleRate = rate;};

        //for now we are going to stuff the noise statistics data in here for estimating tsys w/ the noise diode
        //doing otherwise will require some re-architecting of the whole consumer/producer/buffer pool system
        void ClearOnAccumulation(){fOnAccumulations.clear();};
        void AppendOnAccumulation( HDataAccumulation accum){ fOnAccumulations.push_back(accum); };
        void ExtendOnAccumulation( const std::vector< HDataAccumulation >* accum_vec)
        {
            fOnAccumulations.reserve( fOnAccumulations.size() + accum_vec->size() );
            fOnAccumulations.insert( fOnAccumulations.end(), accum_vec->begin(), accum_vec->end() );
        };
        std::vector< HDataAccumulation >* GetOnAccumulations() {return &fOnAccumulations;};

        void ClearOffAccumulation(){fOffAccumulations.clear();};
        void AppendOffAccumulation( HDataAccumulation accum){ fOffAccumulations.push_back(accum); };
        void ExtendOffAccumulation( const std::vector< HDataAccumulation >* accum_vec)
        {
            fOffAccumulations.reserve( fOffAccumulations.size() + accum_vec->size() );
            fOffAccumulations.insert( fOffAccumulations.end(), accum_vec->begin(), accum_vec->end() );
        };
        std::vector< HDataAccumulation >* GetOffAccumulations() {return &fOffAccumulations;};


        HBufferMetaData& operator= (const HBufferMetaData& rhs)
        {
            if( this != &rhs)
            {
                fAcquisitionStartSecond = rhs.fAcquisitionStartSecond;
                fSampleIndex = rhs.fSampleIndex;
                fSampleRate = rhs.fSampleRate;
                fValidLength = rhs.fValidLength;
                fOnAccumulations = rhs.fOnAccumulations;
                fOffAccumulations = rhs.fOffAccumulations;
            }
            return *this;
        }


    private:

        //seconds since epoch, of the start of the aquisition
        uint64_t fAcquisitionStartSecond;

        //index of the leading sample relative to the start of the aquisition
        uint64_t fSampleIndex;

        //sample rate in Hz (integer only, may need to relax this limitation)
        uint64_t fSampleRate;

        uint64_t fValidLength;

        //data statistics (for noise diode)
        std::vector< HDataAccumulation > fOnAccumulations;
        std::vector< HDataAccumulation > fOffAccumulations;
};

#endif /* end of include guard: HBufferMetaData */
