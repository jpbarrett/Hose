#ifndef HApplicationBackend_HH__
#define HApplicationBackend_HH__

/*
*File: HApplicationBackend.hh
*Class: HApplicationBackend
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

#include "HStateStruct.hh"

namespace hose
{

class HApplicationBackend
{
    public:
        HApplicationBackend();
        virtual ~HApplicationBackend();

        //check a request string for validity
        virtual bool CheckRequest(const std::string& /* request_string */){return true;};

        //get the current state and outgoing message from the application
        virtual HStateStruct GetCurrentState()
        {
            HStateStruct st;
            st.status_code = HStatusCode::unknown;
            st.status_message = std::string("acknowledged");
            return st;
        };

    private:
};

}

#endif /* end of include guard: HApplicationBackend */
