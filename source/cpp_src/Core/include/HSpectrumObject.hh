#ifndef HSpectrumObject_HH__
#define HSpectrumObject_HH__

#include <stdint.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include <istream>

/*
*File: HSpectrumObject.hh
*Class: HSpectrumObject
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: quick and dirty I/O wrapper
*/

namespace hose
{


template< typename XSpectrumType >
class HSpectrumObject
{
    public:

        HSpectrumObject():
            fStartTime(0),
            fSampleRate(0),
            fLeadingSampleIndex(0),
            fSampleLength(0),
            fNAverages(0),
            fSpectrumLength(0),
            fDataOwned(true),
            fSpectrumData(nullptr)
            {};

        //copy constuctor
        HSpectrumObject( const HSpectrumObject& copy):
            fStartTime(0),
            fSampleRate(0),
            fLeadingSampleIndex(0),
            fSampleLength(0),
            fNAverages(0),
            fSpectrumLength(0),
            fDataOwned(true),
            fSpectrumData(nullptr)
        {
            fStartTime = copy.fStartTime;
            fSampleRate = copy.fSampleRate;
            fLeadingSampleIndex = copy.fLeadingSampleIndex;
            fSampleLength = copy.fSampleLength;
            fNAverages = copy.fNAverages;
            fSpectrumLength = copy.fSpectrumLength;
            if(copy.fDataOwned)
            {
                AllocateSpectrum();
                for(size_t i=0; i<copy.fSpectrumLength; i++)
                {
                    fSpectrumData[i] = copy.fSpectrumData[i];
                }
            }
            else
            {
                fDataOwned = false;
                fSpectrumData = copy.fSpectrumData;
            }
        }

        virtual ~HSpectrumObject()
        {
            if(fDataOwned){delete[] fSpectrumData; fSpectrumData = nullptr;};
        }

        uint64_t GetStartTime() const {return fStartTime;};
        uint64_t GetSampleRate() const {return fSampleRate;};
        uint64_t GetLeadingSampleIndex() const {return fLeadingSampleIndex;};
        size_t GetSampleLength() const {return fSampleLength;};
        size_t GetNAverages() const {return fNAverages;};
        size_t GetSpectrumLength() const {return fSpectrumLength;};
        XSpectrumType* GetSpectrumData() {return fSpectrumData;};
        const XSpectrumType* GetSpectrumData() const {return fSpectrumData;};

        //allocate spectrum space (assumes fSpectrumLength is set)
        void AllocateSpectrum()
        {
            if(fSpectrumData != nullptr && fDataOwned)
            {
                delete[] fSpectrumData;
                fSpectrumData = nullptr;
            }

            if(fSpectrumData == nullptr)
            {
                fSpectrumData = new XSpectrumType[fSpectrumLength];
                fDataOwned = true;
            }
            else 
            {
                std::cout<<"error, tried to allocate spectrum space on top of non-owned data"<<std::endl;
                std::exit(1);
            }
        }

        void DeallocateSpectrum()
        {
            if(fDataOwned)
            {
                delete[] fSpectrumData;
                fSpectrumData = nullptr;
            }
            else 
            {
                std::cout<<"error, tried to deallocate non-owned spectrum space"<<std::endl;
                std::exit(1);
            }
        }

        void SetStartTime(uint64_t st){fStartTime = st; };
        void SetSampleRate(uint64_t rate){fSampleRate = rate;};
        void SetLeadingSampleIndex(uint64_t n){fLeadingSampleIndex = n;};
        void SetSampleLength(size_t n){fSampleLength = n;};
        void SetNAverages(size_t n){fNAverages = n;};
        void SetSpectrumLength(size_t spec_length){fSpectrumLength = spec_length;};

        //only use for output, to point to externally allocated array
        void SetSpectrumData(XSpectrumType* array){fSpectrumData = array; fDataOwned = false;};
        void ReleaseSpectrumData(){fSpectrumData = nullptr; fDataOwned = true;};

        void WriteToFile(std::string filename)
        {
            std::ofstream outfile;
            outfile.open(filename.c_str(), std::ios::out | std::ios::binary);
            outfile << *this;
            // outfile.write( (const char*) &fStartTime, sizeof(uint64_t) );
            // outfile.write( (const char*) &fSampleRate, sizeof(uint64_t) );
            // outfile.write( (const char*) &fLeadingSampleIndex, sizeof(uint64_t) );
            // outfile.write( (const char*) &fSampleLength, sizeof(size_t) );
            // outfile.write( (const char*) &fNAverages, sizeof(size_t) );
            // outfile.write( (const char*) &fSpectrumLength, sizeof(size_t) );
            // outfile.write( (const char*) fSpectrumData, sizeof(XSpectrumType)*fSpectrumLength );
            outfile.close();
        }

        void ReadFromFile(std::string filename)
        {
            std::ifstream infile;
            infile.open(filename.c_str(), std::ifstream::binary);

            infile >> *this;

            // infile.read( (char*) &fStartTime, sizeof(uint64_t) );
            // infile.read( (char*) &fSampleRate, sizeof(uint64_t) );
            // infile.read( (char*) &fLeadingSampleIndex, sizeof(uint64_t) );
            // infile.read( (char*) &fSampleLength, sizeof(size_t) );
            // infile.read( (char*) &fNAverages, sizeof(size_t) );
            // infile.read( (char*) &fSpectrumLength, sizeof(size_t) );

            // //allocate some spectrum space
            // ReleaseSpectrumData();
            // AllocateSpectrum();
            // infile.read( (char*) fSpectrumData, sizeof(XSpectrumType)*fSpectrumLength );
            infile.close();
        }

    protected:

        //data
        uint64_t fStartTime; //acquisition start time in seconds since epoch
        uint64_t fSampleRate; //may need to accomodate doubles?
        uint64_t fLeadingSampleIndex; //sample index since start of the acquisition
        size_t fSampleLength; //total number of samples used to compute the spectrum
        size_t fNAverages; //if spectral averaging was used, how many averaging periods were used
        size_t fSpectrumLength; //number of spectral points

        //indicates if we need to delete spectrum data on destruction
        bool fDataOwned;

        //pointer to spectrum data array, not owned if passed through SetSpectrumData()
        XSpectrumType* fSpectrumData;
};

template< typename XSpectrumType, typename XStreamType >
XStreamType& operator>>(XStreamType& s, HSpectrumObject<XSpectrumType>& aData)
{
    uint64_t start_time; 
    uint64_t sample_rate;
    uint64_t leading_sample_index; 
    size_t sample_length; 
    size_t n_averages; 
    size_t spectrum_length;

    s >> start_time;
    s >> sample_rate;
    s >> leading_sample_index;
    s >> sample_length;
    s >> n_averages;
    s >> spectrum_length;

    aData.SetStartTime(start_time);
    aData.SetSampleRate(sample_rate);
    aData.SetLeadingSampleIndex(leading_sample_index);
    aData.SetSampleLength(sample_length);
    aData.SetNAverages(n_averages);
    aData.SetSpectrumLength(spectrum_length);

    aData.AllocateSpectrum();

    XSpectrumType* spec = aData.GetSpectrumData();
    for(size_t i=0; i<spectrum_length; i++)
    {
        s >> spec[i];
    }

    return s;
}

template< typename XSpectrumType, typename XStreamType >
XStreamType& operator<<(XStreamType& s, const HSpectrumObject<XSpectrumType>& aData)
{
    s << aData.GetStartTime() << ' ';
    s << aData.GetSampleRate() << ' ';
    s << aData.GetLeadingSampleIndex() << ' ';
    s << aData.GetSampleLength() << ' ';
    s << aData.GetNAverages() << ' ';
    s << aData.GetSpectrumLength() << ' ';

    const XSpectrumType* spec = aData.GetSpectrumData();
    size_t n = aData.GetSpectrumLength();
    for(size_t i=0; i<n; i++)
    {
        s << spec[i] << ' ';
    }

    return s;
}

}

#endif /* end of include guard: HSpectrumObject */
