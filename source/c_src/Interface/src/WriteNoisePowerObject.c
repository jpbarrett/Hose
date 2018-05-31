#include <stdio.h>

#include "HSpectrumFile.h"


int WriteSpectrumObject(const char* filename, struct HSpectrumFileStruct* spectrum)
{
    unsigned int i;
    FILE* outfile = NULL;
    outfile = fopen(filename,"wb");

    if(outfile)
    {
        fwrite( (const void*) &(spectrum->fHeader.fVersion), 1 , sizeof(char), outfile);
        fwrite( (const void*) &(spectrum->fHeader.fSidebandFlag), 1 , sizeof(char), outfile);
        fwrite( (const void*) &(spectrum->fHeader.fPolarizationFlag), 1, sizeof(char), outfile);

        fwrite( (const void*) &(spectrum->fHeader.fStartTime), 1, sizeof(uint64_t), outfile);
        fwrite( (const void*) &(spectrum->fHeader.fSampleRate), 1, sizeof(uint64_t), outfile);
        fwrite( (const void*) &(spectrum->fHeader.fLeadingSampleIndex), 1, sizeof(uint64_t), outfile);
        fwrite( (const void*) &(spectrum->fHeader.fSampleLength), 1, sizeof(size_t), outfile);
        fwrite( (const void*) &(spectrum->fHeader.fNAverages), 1, sizeof(size_t), outfile);

        fwrite( (const void*) &(spectrum->fHeader.fExperimentNameLength), sizeof(size_t), 1, outfile);
        fwrite( (const void*) spectrum->fHeader.fExperimentName, sizeof(char), spectrum->fHeader.fExperimentNameLength, outfile);

        fwrite( (const void*) &(spectrum->fHeader.fSourceNameLength), sizeof(size_t), 1, outfile);
        fwrite( (const void*) spectrum->fHeader.fSourceName, sizeof(char), spectrum->fHeader.fSourceNameLength, outfile);

        fwrite( (const void*) &(spectrum->fHeader.fScanNameLength), sizeof(size_t), 1, outfile);
        fwrite( (const void*) spectrum->fHeader.fScanName, sizeof(char), spectrum->fHeader.fExperimentNameLength, outfile);

        fwrite( (const void*) &(spectrum->fHeader.fSpectrumLength), sizeof(size_t), 1, outfile);
        fwrite( (const void*) spectrum->fHeader.fSpectrumData, sizeof( CSPECTRUMTYPE ), spectrum->fHeader.fSpectrumLength, outfile);

        fwrite( (const void*) &(spectrum->fHeader.fOnAccumulationLength), sizeof(size_t), 1, outfile);
        for(i=0; i<spectrum->fHeader.fOnAccumulationLength; i++)
        {
            struct HDataAccumulationStruct stat = spectrum->OnAccumulations[i];
            fwrite( (const void*) &(stat.sum_x), sizeof(double), 1, outfile);
            fwrite( (const void*) &(stat.sum_x2), sizeof(double), 1, outfile);
            fwrite( (const void*) &(stat.count), sizeof(double), 1, outfile);
            fwrite( (const void*) &(stat.start_index), sizeof(uint64_t), 1, outfile);
            fwrite( (const void*) &(stat.stop_index), sizeof(uint64_t), 1, outfile);
        }

        fwrite( (const void*) &(spectrum->OffAccumulationLength), sizeof(size_t), 1, outfile);
        for(i=0; i<spectrum->OffAccumulationLength; i++)
        {
            struct HDataAccumulationStruct stat = spectrum->OffAccumulations[i];
            fwrite( (const void*) &(stat.sum_x), sizeof(double), 1, outfile);
            fwrite( (const void*) &(stat.sum_x2), sizeof(double), 1, outfile);
            fwrite( (const void*) &(stat.count), sizeof(double), 1, outfile);
            fwrite( (const void*) &(stat.start_index), sizeof(uint64_t), 1, outfile);
            fwrite( (const void*) &(stat.stop_index), sizeof(uint64_t), 1, outfile);
        }

        fclose(outfile);

        return 0;
    }

    //error, couldn't open file
    return 1;
}
