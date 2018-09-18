#ifndef HAntennaPosition_HH__
#define HAntennaPosition_HH__


#include "HMetaDataBase.hh"


namespace hose
{

class HAntennaPosition: public HMetaDataBase
{
    public:

        HAntennaPosition():HMetaDataBase( std::string("antenna_position") )
        {
            fFieldNames.push_back( std::string("az") );
            fFieldNames.push_back( std::string("el") );
        }
};

}

#endif /* end of include guard: HAntennaPosition_HH__ */
