#ifndef HMetaDataBase_HH__
#define HMetaDataBase_HH__

#include <string>
#include <vector>

/*
*File: HMetaDataBase.hh
*Class: HMetaDataBase
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description: base class for meta data container, all meta data has a 'measurement' name, and time stamp
*/

namespace hose
{

class HMetaDataBase
{
    public:

        HMetaDataBase():
            fMeasurementName(""),
            fTimeStamp("")
        {
            fFieldNames.clear();
        };

        HMetaDataBase(std::string name):
            fMeasurementName(name),
            fTimeStamp("")
        {
            fFieldNames.clear();
        };

        virtual ~HMetaDataBase(){};

        void SetTimeStamp(std::string time_stamp){fTimeStamp = time_stamp;}
        std::string GetTimeStamp() const {return fTimeStamp;};

        bool GetTimeStampAsEpochSecond(const uint64_t& epoch_sec) const
        {
            //all meta data has a time stamp
            //time is in UTC with format: YYYY-MM-DDTHH:MM:SS.(F)Z
            //number of digits in the fraction part (F) may vary

            if( fTimeStamp.back() != 'Z' )
            {
                //only support UTC/Zulu time zone, if something else snuck in, fail
                return false;
            }
            else
            {
                //break the time stamp down
                std::string year_str =
                std::string


            }



            static double EpochTime(const std::string& iso8601Time)
            {
                struct tm t;
                if (iso8601Time.back() != 'Z') throw PBException("Non Zulu 8601 timezone not supported");
                char* ptr = strptime(iso8601Time.c_str(), "%FT%T", &t);
                if( ptr == nullptr)
                {
                    throw PBException("strptime failed, can't parse " + iso8601Time);
                }
                double t2 = timegm(&t); // UTC
                if (*ptr)
                {
                    double fraction = atof(ptr);
                    t2 += fraction;
                }
                return t2;
            }
        }


        double GetTimeStampAsEpochSecond() const
        {

        }

        std::string GetMeasurementName() const {return fMeasurementName;};
        virtual std::vector< std::string > GetFieldNames() const {return fFieldNames;};

    protected:

        //all meta data has a time stamp
        //time is in UTC with format: YYYY-MM-DDTHH:MM:SS.(F)Z
        //number of digits in the fraction part (F) may vary
        std::string fTimeStamp;

        //name should be fixed depending on the meta data type
        std::string fMeasurementName;

        //should be a fixed list of names depending on the meta data type
        std::vector< std::string > fFieldNames;
};

}

#endif /* end of include guard: HMetaDataBase */
