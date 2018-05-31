#include <stdio.h>
#include "HSpectrumFile.h"

int ReadSpectrumObject(const char* filename, struct HSpectrumFileStruct* spectrum)
{
    unsigned int i;
    FILE* infile = NULL;
    infile = fopen(filename,"rb");

    if(infile)
    {
        fread( (void*) &(spectrum->Version), sizeof(char), 1, infile);
        fread( (void*) &(spectrum->SidebandFlag), sizeof(char), 1, infile);
        fread( (void*) &(spectrum->PolarizationFlag), sizeof(char), 1, infile);

        fread( (void*) &(spectrum->StartTime), sizeof(uint64_t), 1, infile);
        fread( (void*) &(spectrum->SampleRate), sizeof(uint64_t), 1, infile);
        fread( (void*) &(spectrum->LeadingSampleIndex), sizeof(uint64_t), 1, infile);
        fread( (void*) &(spectrum->SampleLength), sizeof(size_t), 1, infile);
        fread( (void*) &(spectrum->NAverages), sizeof(size_t), 1, infile);

        fread( (void*) &(spectrum->ExperimentNameLength), sizeof(size_t), 1, infile);
        fread( (void*) spectrum->ExperimentName, sizeof(char), spectrum->ExperimentNameLength, infile);

        fread( (void*) &(spectrum->SourceNameLength), sizeof(size_t), 1, infile);
        fread( (void*) spectrum->SourceName, sizeof(char), spectrum->SourceNameLength, infile);

        fread( (void*) &(spectrum->ScanNameLength), sizeof(size_t), 1, infile);
        fread( (void*) spectrum->ScanName, sizeof(char), spectrum->ExperimentNameLength, infile);

        fread( (void*) &(spectrum->SpectrumLength), sizeof(size_t), 1, infile);

        if(spectrum->SpectrumData != NULL)
        {
            free(spectrum->SpectrumData);
        }
        spectrum->SpectrumData = (CSPECTRUMTYPE*) malloc( spectrum->SpectrumLength*sizeof(CSPECTRUMTYPE) );
        fread( (void*) spectrum->SpectrumData, sizeof( CSPECTRUMTYPE ), spectrum->SpectrumLength, infile);

        fread( (void*) &(spectrum->OnAccumulationLength), sizeof(size_t), 1, infile);
        if(spectrum->OnAccumulations != NULL)
        {
            free(spectrum->OnAccumulations);
        }
        spectrum->OnAccumulations = (struct HDataAccumulationStruct*) malloc( spectrum->OnAccumulationLength*sizeof(struct HDataAccumulationStruct) );
        for(i=0; i<spectrum->OnAccumulationLength; i++)
        {
            struct HDataAccumulationStruct stat; 
            fread( (void*) &(stat.sum_x), sizeof(double), 1, infile);
            fread( (void*) &(stat.sum_x2), sizeof(double), 1, infile);
            fread( (void*) &(stat.count), sizeof(double), 1, infile);
            fread( (void*) &(stat.start_index), sizeof(uint64_t), 1, infile);
            fread( (void*) &(stat.stop_index), sizeof(uint64_t), 1, infile);
            spectrum->OnAccumulations[i] = stat;
        }

        fread( (void*) &(spectrum->OffAccumulationLength), sizeof(size_t), 1, infile);
        if(spectrum->OffAccumulations != NULL)
        {
            free(spectrum->OffAccumulations);
        }
        spectrum->OffAccumulations = (struct HDataAccumulationStruct*) malloc( spectrum->OffAccumulationLength*sizeof(struct HDataAccumulationStruct) );
        for(i=0; i<spectrum->OffAccumulationLength; i++)
        {
            struct HDataAccumulationStruct stat; 
            fread( (void*) &(stat.sum_x), sizeof(double), 1, infile);
            fread( (void*) &(stat.sum_x2), sizeof(double), 1, infile);
            fread( (void*) &(stat.count), sizeof(double), 1, infile);
            fread( (void*) &(stat.start_index), sizeof(uint64_t), 1, infile);
            fread( (void*) &(stat.stop_index), sizeof(uint64_t), 1, infile);
            spectrum->OffAccumulations[i] = stat;
        }

        fclose(infile);

        return 0;
    }
    
    return 1;
}
