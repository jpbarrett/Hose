#include "HSpectrumFile.h"

void ClearSpectrumObject(struct HSpectrumFileStruct* spectrum)
{
    unsigned int i;
    spectrum->fHeader.fHeaderSize = sizeof(struct HSpectrumHeaderStruct);
    for(i=0; i<FLAG_WIDTH; i++)
    {
        spectrum->fHeader.fVersion[i] = '\0';
        spectrum->fHeader.fSidebandFlag[i] = '\0';
        spectrum->fHeader.fPolarizationFlag[i] = '\0';
    }
    spectrum->fHeader.fStartTime = 0; //acquisition start time in seconds since epoch
    spectrum->fHeader.fSampleRate = 0; //may need to accomodate doubles?
    spectrum->fHeader.fLeadingSampleIndex = 0; //sample index since start of the acquisition
    spectrum->fHeader.fSampleLength = 0; //total number of samples used to compute the spectrum
    spectrum->fHeader.fNAverages = 0; //if spectral averaging was used, how many averaging periods were used
    spectrum->fHeader.fSpectrumLength = 0; //number of spectral points

    for(i=0; i<NAME_WIDTH; i++)
    {
        spectrum->fHeader.fExperimentName[i] = '\0';
        spectrum->fHeader.fSourceName[i] = '\0';
        spectrum->fHeader.fScanName[i] = '\0';
    }

    if(spectrum->fRawSpectrumData != NULL)
    {
        free spectrum->fRawSpectrumData;
    }
    spectrum->fRawSpectrumData = NULL;
}