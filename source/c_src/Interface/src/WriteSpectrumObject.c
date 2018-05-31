#include <stdio.h>

#include "HSpectrumFile.h"


int WriteSpectrumObject(const char* filename, struct HSpectrumFileStruct* spectrum)
{
    unsigned int i;
    FILE* outfile = NULL;
    outfile = fopen(filename,"wb");

    if(outfile)
    {
        fwrite( &(spectrum->fHeader.fHeaderSize), sizeof(size_t), 1, outfile);
        fwrite( &(spectrum->fHeader.fVersion), sizeof(char), FLAG_WIDTH, outfile);
        fwrite( &(spectrum->fHeader.fSidebandFlag), sizeof(char), FLAG_WIDTH, outfile);
        fwrite( &(spectrum->fHeader.fPolarizationFlag), sizeof(char), FLAG_WIDTH, outfile);

        fwrite( &(spectrum->fHeader.fStartTime), sizeof(uint64_t), 1, outfile);
        fwrite( &(spectrum->fHeader.fSampleRate), sizeof(uint64_t), 1, outfile);
        fwrite( &(spectrum->fHeader.fLeadingSampleIndex), sizeof(uint64_t), 1, outfile);
        fwrite( &(spectrum->fHeader.fSampleLength), sizeof(size_t), 1, outfile);
        fwrite( &(spectrum->fHeader.fNAverages), sizeof(size_t), 1, outfile);
        fwrite( &(spectrum->fHeader.fSpectrumLength), sizeof(size_t), 1, outfile);

        fwrite(  spectrum->fHeader.fExperimentName, sizeof(char), NAME_WIDTH, outfile);
        fwrite(  spectrum->fHeader.fSourceName, sizeof(char), NAME_WIDTH, outfile);
        fwrite(  spectrum->fHeader.fScanName, sizeof(char), NAME_WIDTH, outfile);

        //write the raw spectrum data
        fwrite();


        fclose(outfile);

        return 0;
    }

    //error, couldn't open file
    return 1;
}
