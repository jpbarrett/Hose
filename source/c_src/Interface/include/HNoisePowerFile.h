#ifndef HNoisePowerFile_H__
#define HNoisePowerFile_H__

#include "HDataAccumulationStruct.h"
#include "HNoisePowerHeaderStruct.h"
#include "HNoisePowerFileStruct.h"

static char const * const NOISE_POWER_HEADER_VERSION = "001";

extern struct HNoisePowerFileStruct* CreateNoisePowerFileStruct();
extern void InitializeNoisePowerFileStruct(struct HNoisePowerFileStruct* spectrum);
extern void ClearNoisePowerFileStruct(struct HNoisePowerFileStruct* spectrum);
extern void DestroyNoisePowerFileStruct(struct HNoisePowerFileStruct* spectrum);
extern void CopyNoisePowerFileStruct(const struct HNoisePowerFileStruct* src, struct HNoisePowerFileStruct* dest);
extern int ReadNoisePowerFile(const char* filename, struct HNoisePowerFileStruct* spectrum);
extern int WriteNoisePowerFile(const char* filename, struct HNoisePowerFileStruct* spectrum);


#endif /* end of include guard: HNoisePowerFile_H__ */
