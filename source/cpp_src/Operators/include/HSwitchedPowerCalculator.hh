#ifndef HSwitchedPowerCalculator_HH__
#define HSwitchedPowerCalculator_HH__

#include <iostream>
#include <ostream>
#include <ios>
#include <fstream>
#include <sstream>
#include <thread>
#include <utility>
#include <algorithm>
#include <stdint.h>
#include <cmath>

#include "HLinearBuffer.hh"

extern "C"
{
    #include "HDataAccumulationStruct.h"
}

#include "HDataAccumulationContainer.hh"

namespace hose
{

/*
*File: HSwitchedPowerCalculator.hh
*Class: HSwitchedPowerCalculator
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: Computes the sample RMS, for switched noise source,
*/


template< typename XBufferItemType >
class HSwitchedPowerCalculator:  public HConsumerProducer< XBufferItemType, HDataAccumulationContainer, HConsumerBufferHandler_WaitWithTimeout< XBufferItemType >, HProducerBufferHandler_Steal< HDataAccumulationContainer > >
{
    public:

        HSwitchedPowerCalculator()
        {
            //std::cout<<"switched power calc = "<<this<<std::endl;
        };
        virtual ~HSwitchedPowerCalculator(){};

        void SetSamplingFrequency(double samp_freq){fSamplingFrequency = samp_freq;};
        void SetSwitchingFrequency(double switch_freq){fSwitchingFrequency = switch_freq;};
        void SetBlankingPeriod(double blank_period){fBlankingPeriod = blank_period;}

    private:

        virtual void ExecuteThreadTask() override
        {
            //buffers
            HLinearBuffer< HDataAccumulationContainer >* sink = nullptr;
            HLinearBuffer< XBufferItemType >* source = nullptr;

            if( this->fSourceBufferPool->GetConsumerPoolSize( this->GetConsumerID() ) != 0 ) //only do work if there is stuff to process
            {
                //first get a sink buffer from the buffer handler
                HProducerBufferPolicyCode sink_code = this->fSinkBufferHandler.ReserveBuffer(this->fSinkBufferPool, sink);

                if( (sink_code & HProducerBufferPolicyCode::success) && sink != nullptr)
                {
                    std::lock_guard<std::mutex> sink_lock(sink->fMutex);

                    HConsumerBufferPolicyCode source_code = this->fSourceBufferHandler.ReserveBuffer(this->fSourceBufferPool, source, this->GetConsumerID());

                    if( (source_code & HConsumerBufferPolicyCode::success) && source !=nullptr)
                    {

                        std::lock_guard<std::mutex> source_lock(source->fMutex);

                        //grab the pointer to the accumulation container
                        auto accum_container = &( (sink->GetData())[0] ); //should have buffer size of 1

                        //set appropriate meta data quantities for the accumulation container
                        accum_container->SetSampleRate( source->GetMetaData()->GetSampleRate() );
                        accum_container->SetAcquisitionStartSecond( source->GetMetaData()->GetAcquisitionStartSecond() );
                        accum_container->SetLeadingSampleIndex( source->GetMetaData()->GetLeadingSampleIndex() );
                        accum_container->SetSampleLength( source->GetArrayDimension(0) );
                        accum_container->SetNoiseDiodeSwitchingFrequency(fSwitchingFrequency);
                        accum_container->SetNoiseDiodeBlankingPeriod(fBlankingPeriod);
                        accum_container->SetSidebandFlag( source->GetMetaData()->GetSidebandFlag() );
                        accum_container->SetPolarizationFlag( source->GetMetaData()->GetPolarizationFlag() );

                        //calculate the accumulations for this buffer
                        Calculate(fSamplingFrequency, fSwitchingFrequency, fBlankingPeriod, source, accum_container);

                        //release the buffers
                        this->fSourceBufferHandler.ReleaseBufferToConsumer(this->fSourceBufferPool, source, this->GetNextConsumerID());
                        this->fSinkBufferHandler.ReleaseBufferToConsumer(this->fSinkBufferPool, sink);
                    }
                    else
                    {
                        if(sink !=nullptr)
                        {
                           this->fSinkBufferHandler.ReleaseBufferToProducer(this->fSinkBufferPool, sink);
                        }
                    }
                }
            }
        }

////////////////////////////////////////////////////////////////////////////////

        virtual bool WorkPresent() override
        {
            return ( this->fSourceBufferPool->GetConsumerPoolSize( this->GetConsumerID() ) != 0 );
        }


////////////////////////////////////////////////////////////////////////////////

        static void Calculate(double sampling_frequency,
                              double switching_frequency,
                              double blanking_period,
                              HLinearBuffer< XBufferItemType >* source,
                              HDataAccumulationContainer* accum_container)
        {
            if( source != nullptr )
            {
                uint64_t leading_sample_index = accum_container->GetLeadingSampleIndex();
                uint64_t buffer_size = source->GetArrayDimension(0);
                XBufferItemType* raw_data = source->GetData();

                accum_container->SetNoiseDiodeSwitchingFrequency(switching_frequency);
                accum_container->SetNoiseDiodeBlankingPeriod(blanking_period);

                //determine which buffer samples are in the on/of periods
                //(this assumes the noise diode switching and acquisition triggering was synced with the 1pps signal)
                std::vector< std::pair<uint64_t, uint64_t> > on_intervals;
                std::vector< std::pair<uint64_t, uint64_t> > off_intervals;

                GetOnOffIntervals(sampling_frequency, switching_frequency, blanking_period, leading_sample_index, buffer_size, on_intervals, off_intervals);

                accum_container->ClearAccumulation();

                //loop over the on-intervals accumulating statistics
                for(unsigned int i=0; i<on_intervals.size(); i++)
                {
                    uint64_t begin = on_intervals[i].first;
                    uint64_t end = on_intervals[i].second;

                    struct HDataAccumulationStruct stat;
                    stat.start_index = leading_sample_index + begin;
                    stat.stop_index = leading_sample_index + end;
                    stat.sum_x = 0;
                    stat.sum_x2 = 0;
                    stat.count = 0;
                    stat.state_flag = H_NOISE_DIODE_ON;
                    for(uint64_t sample_index = begin; sample_index < end; sample_index++)
                    {
                        double val = raw_data[sample_index];
                        stat.sum_x += val;
                        stat.sum_x2 += val*val;
                        stat.count += 1.0;
                    }
                    accum_container->AppendAccumulation(stat);
                }

                //loop over the off-intervals accumulating statistics
                for(unsigned int i=0; i<off_intervals.size(); i++)
                {
                    uint64_t begin = off_intervals[i].first;
                    uint64_t end = off_intervals[i].second;

                    struct HDataAccumulationStruct stat;
                    stat.start_index = leading_sample_index + begin;
                    stat.stop_index = leading_sample_index + end;
                    stat.sum_x = 0;
                    stat.sum_x2 = 0;
                    stat.count = 0;
                    stat.state_flag = H_NOISE_DIODE_OFF;
                    for(uint64_t sample_index = begin; sample_index < end; sample_index++)
                    {
                        double val = raw_data[sample_index];
                        stat.sum_x += val;
                        stat.sum_x2 += val*val;
                        stat.count += 1.0;
                    }
                    accum_container->AppendAccumulation(stat);
                }
            }
        }

        ////////////////////////////////////////////////////////////////////////////////


        //returns intervals [x,y) of samples to be included in the ON/OFF sets
        //assumes signal synced such that ON is the first state (at acquisition start, index=0)
        static void GetOnOffIntervals(double sampling_frequency,
                                      double switching_frequency,
                                      double blanking_period,
                                      uint64_t start,
                                      uint64_t length,
                                      std::vector< std::pair<uint64_t, uint64_t> >& on_intervals,
                                      std::vector< std::pair<uint64_t, uint64_t> >& off_intervals)
        {
            on_intervals.clear();
            off_intervals.clear();

            double sampling_period = 1.0/sampling_frequency;
            double switching_period = 1.0/switching_frequency;
            double half_switching_period = switching_period/2.0;
            double buffer_length = sampling_period*length;
            double buffer_start = start*sampling_period;
            double buffer_end = buffer_start + buffer_length;

            //figure out the last/next on/off-times which occured on or before the start/end of this buffer
            unsigned int a = std::floor(buffer_start/half_switching_period);
            unsigned int b = std::ceil((buffer_start+sampling_period)/half_switching_period);
            bool on_at_start = false;
            if(a % 2 == 0){on_at_start = true;};

            unsigned int c = std::floor((buffer_end)/half_switching_period);
            unsigned int d = std::ceil( (buffer_end+sampling_period)/half_switching_period);

            std::vector< std::pair<double, double> > on_times;
            std::vector< std::pair<double, double> > off_times;

            //take care of simple cases first
            if(a == c && b == d)
            {
                if(on_at_start)
                {
                    //entire interval is contained within an on period
                    on_times.push_back( std::pair<double, double>(0.0, buffer_length) );
                }
                else
                {
                    //entire interval is contained within an off period
                    off_times.push_back( std::pair<double, double>(0.0, buffer_length) );
                }
            }
            else if( b == c)
            {
                //there is a single on/off transition during the interval
                if(on_at_start)
                {
                    on_times.push_back( std::pair<double, double>(0.0, b*half_switching_period - buffer_start) );
                    off_times.push_back( std::pair<double, double>(b*half_switching_period - buffer_start, buffer_length) );
                }
                else
                {
                    off_times.push_back( std::pair<double, double>(0.0, b*half_switching_period - buffer_start) );
                    on_times.push_back( std::pair<double, double>(b*half_switching_period - buffer_start, buffer_length) );
                }
            }
            else
            {
                //there are multiple on-off transitions during the interval

                //take care of the portion between the start and b
                if(on_at_start)
                {
                    on_times.push_back( std::pair<double, double>(0.0, b*half_switching_period - buffer_start) );
                }
                else
                {
                    off_times.push_back( std::pair<double, double>(0.0, b*half_switching_period - buffer_start) );
                }

                //take care the multiple switching periods between b and c
                for(unsigned int i=b; i<c; i++)
                {
                    if(i % 2 == 0)
                    {
                        on_times.push_back( std::pair<double, double>(i*half_switching_period - buffer_start, (i+1)*half_switching_period - buffer_start) );
                    }
                    else
                    {
                        off_times.push_back( std::pair<double, double>(i*half_switching_period - buffer_start, (i+1)*half_switching_period - buffer_start) );
                    }
                }

                //take care of the portion between c and then end
                if(c % 2 == 0)
                {
                    on_times.push_back( std::pair<double, double>(c*half_switching_period - buffer_start, buffer_length) );
                }
                else
                {
                    off_times.push_back( std::pair<double, double>(c*half_switching_period- buffer_start, buffer_length) );
                }

            }

            uint64_t blank = std::ceil( (blanking_period/2.0)*sampling_frequency );

            //now convert to sample indices, while removing any time ranges which have a size of zero
            for(unsigned int i=0; i<on_times.size(); i++)
            {
                if(on_times[i].second - on_times[i].first > 0.0)
                {
                    uint64_t begin = std::floor(on_times[i].first/sampling_period);
                    uint64_t end = std::floor(on_times[i].second/sampling_period);
                    //eliminate any intervals which are too short, and correct for blanking period
                    //yes...we are blanking at the buffer start/stop too, this is trivial amount of data loss that would be a pain to fix
                    if(end > begin + 2*blank+1)
                    {
                        on_intervals.push_back( std::pair<uint64_t, uint64_t>(begin+blank, end-blank) );
                    }
                }
            }

            for(unsigned int i=0; i<off_times.size(); i++)
            {
                if(off_times[i].second - off_times[i].first > 0.0)
                {
                    uint64_t begin =  std::floor(off_times[i].first/sampling_period);
                    uint64_t end = std::floor(off_times[i].second/sampling_period);
                    //eliminate any intervals which are too short, and correct for blanking period
                    //yes...we are blanking at the buffer start/stop too, this is trivial amount of data loss that would be a pain to fix
                    if(end > begin + 2*blank+1)
                    {
                        off_intervals.push_back( std::pair<uint64_t, uint64_t>(begin+blank,end-blank) );
                    }
                }
            }
        }

    private:


        //data /////////////////////////////////////////////////////////////////
        double fSamplingFrequency;
        double fSwitchingFrequency; //frequency at which the noise diode is switched
        double fBlankingPeriod; // ignore samples within +/- half the blanking period about switching time

};


}

#endif /* end of include guard: HSwitchedPowerCalculator */
