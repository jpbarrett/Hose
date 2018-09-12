#ifndef HAntennaPosition_HH__
#define HAntennaPosition_HH__


#include "HMetaDataBase.hh"


namespace hose
{

class HAntennaPosition: public HMetaDataBase
{
    public:
        HAntennaPosition():HMetaDataBase( std::string("antenna_position") ),
            f

        ~HAntennaPosition() = default;
        HAntennaPosition(const HAntennaPosition& other) = default;
        HAntennaPosition(HAntennaPosition&& other) = default;
        HAntennaPosition& operator=(const HAntennaPosition& other) = default;
        HAntennaPosition& operator=(HAntennaPosition&& other) = default;
};

}

#endif /* end of include guard: HAntennaPosition_HH__ */
