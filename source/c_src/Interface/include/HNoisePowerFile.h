#ifndef HNoisePowerFile_H__
#define HNoisePowerFile_H__

#include "HDataAccumulationStruct.h"
#include "HNoisePowerFileStruct.h"

extern void InitializeNoisePowerObject(struct HNoisePowerFileStruct* power);
extern void ClearNoisePowerObject(struct HNoisePowerFileStruct* power);
extern int ReadNoisePowerObject(const char* filename, struct HNoisePowerFileStruct* power);
extern int WriteNoisePowerObject(const char* filename, struct HNoisePowerFileStruct* power);

#endif /* end of include guard: HNoisePowerFile_H__ */
