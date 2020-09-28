#pragma once

#include "macro.h"


#define DLL_EXPORT

#ifdef DLL_EXPORT
#define RTMPDLL extern "C" _declspec(dllexport)
#else
#define RTMPDLL extern "C" _declspec(dllimport)
#endif

RTMPDLL void rtmp_init();
RTMPDLL void rtmp_release_all();

RTMPDLL int64_t rtmp_alloc(const char * url);
RTMPDLL void rtmp_release_one(int64_t key);

RTMPDLL void rtmp_add_data(int64_t key, char *pData, int iSize);
RTMPDLL void rtmp_set_interval(int64_t key, int interval);

