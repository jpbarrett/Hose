#ifndef HUnitTest_HH__
#define HUnitTest_HH__

/*
*File: HUnitTest.hh
*Class: HUnitTest
*Author: J. Barrett
*Email: barrettj@mit.edu
*Date:
*Description:
*/

class HUnitTest
{
    public:
        HUnitTest():fVerbosity(0),fErrorCode(-1){};
        virtual ~HUnitTest(){};

        virtual void Initialize() = 0;
        virtual void RunTest() = 0;

        virtual void RunTimedTest()
        {
    
        }

        void SetVerbosity(unsigned int verbosity){fVerbosity = verbosity;};
        int GetErrorCode() const {return fErrorCode;};
        bool HasPassed() const { if(fErrorCode == 0){ return true; } else {return false;} }

        //TODO determine the time format
        virtual X GetElapsedTime() const {};

    protected:

        unsigned int fVerbosity;
        int fErrorCode;

};

#endif /* end of include guard: HUnitTest */
