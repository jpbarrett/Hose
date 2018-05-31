#ifndef HSpectrumFile_H__
#define HSpectrumFile_H__

#include "HSpectrumHeaderStruct.h"
#include "HSpectrumFileStruct.h"

extern void InitializeSpectrumObject(struct HSpectrumFileStruct* spectrum);
extern void ClearSpectrumObject(struct HSpectrumFileStruct* spectrum);
extern int ReadSpectrumObject(const char* filename, struct HSpectrumFileStruct* spectrum);
extern int WriteSpectrumObject(const char* filename, struct HSpectrumFileStruct* spectrum);

#endif /* end of include guard: HSpectrumFile_H__ */
