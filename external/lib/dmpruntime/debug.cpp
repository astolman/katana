#include "debug.h"

#ifdef DMP_ENABLE_DEBUGGING 
int DEBUG_CATEGORY = DEBUG_ALL;
//int DEBUG_CATEGORY = DEBUG_MethodFlag::UNPROTECTED;
#else
//int DEBUG_CATEGORY = DEBUG_BARRIER | DEBUG_CONDVAR ;
//int DEBUG_CATEGORY = DEBUG_BARRIER;
int DEBUG_CATEGORY = DEBUG_MethodFlag::UNPROTECTED;
#endif