#ifndef __AMOD_SHARED_DEFS_H
#define __AMOD_SHARED_DEFS_H

#include "AloneModTimeInfo.h"

#ifdef CLIENT_DLL
extern const char* szMapName;
#endif

//TODO: make this not hard coded in
extern const char* g_pBonusMaps[4];

//TODO: this is now obsolete since i have created the MapTimeInfo_t. Remove me!!
#define AMOD_DAYTIME_EDITION 0

#endif