#ifndef HBufferMetaData_HH__
#define HBufferMetaData_HH__

#include <string>
#include <stdint.h>

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
            fAquisitionStartSecond(0),
            fSampleIndex(0),
            fSampleRate(0)
        {};

        virtual ~HBufferMetaData(){};

        virtual std::string GetName() const {return std::string("BasicMetaData");}

        //the integer second (since the epoch) at which the aqcuisition was started
        uint64_t GetAquisitionStartSecond() const {return fAquisitionStartSecond;};
        void SetAquisitionStartSecond(const uint64_t& sec) {fAquisitionStartSecond = sec;};

        //sample index relative to start of the aqcuisition
        uint64_t GetLeadingSampleIndex() const {return fSampleIndex;};
        void SetLeadingSampleIndex(const uint64_t& index){fSampleIndex = index;};

        uint64_t GetSampleRate() const {return fSampleRate;};
        void SetSampleRate(const uint64_t& rate) {fSampleRate = rate;};

        //for now we are going to stuff the noise-diode tsys data in here
        //doing otherwise will require some re-architecting of the whole consumer/producer/buffer pool system
        void SetOnAccumulation(double on_accum){fOnAccum = on_accum;};
        double GetOnAccumulation() const { return fOnAccum;};
        void SetOffAccumulation(double off_accum){fOffAccum = off_accum;};
        double GetOffAccumulation() const { return fOffAccum;};

        void SetOnSquaredAccumulation(double on_saccum){fOnSquaredAccum = on_saccum;};
        double GetOnSquaredAccumulation() const { return fOnSquaredAccum;};
        void SetOffSquaredAccumulation(double off_saccum){fOffSquaredAccum = off_saccum;};
        double GetOffSquaredAccumulation() const { return fOffSquaredAccum;};

        void SetOnCount(double on_count){fOnCount = on_count;};
        double GetOnCount() const { return fOnCount;};
        void SetOffCount(double off_count){fOffCount = off_count;};
        double GetOffCount() const { return fOffCount;};

    private:

        //seconds since epoch, of the start of the aquisition
        uint64_t fAquisitionStartSecond;

        //index of the leading sample relative to the start of the aquisition
        uint64_t fSampleIndex;

        //sample rate in Hz (integer only, may need to relax this limitation)
        uint64_t fSampleRate;

        //power cal data
        double fOnAccum;
        double fOffAccum;

        double fOnSquaredAccum;
        double fOffSquaredAccum;

        double fOnCount;
        double fOffCount;


};

#endif /* end of include guard: HBufferMetaData */
