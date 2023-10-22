//
//  KSLogger.h
//  KSCrash-dcl
//
//  Created by cl d on 2023/10/21.
//

#ifndef KSLogger_h
#define KSLogger_h

#include <stdio.h>

void i_kslog_logC(const char* level,
                  const char* file,
                  int line,
                  const char* function,
                  const char* fmt, ...);

#define i_KSLOG_FULL i_kslog_logC

#define a_KSLOG_FULL(LEVEL, FMT, ...) \
    i_KSLOG_FULL(LEVEL, \
                 __FILE__, \
                 __LINE__, \
                 __PRETTY_FUNCTION__, \
                 FMT, \
                 ##__VA_ARGS__)

#define KSLOG_ERROR(FMT, ...) a_KSLOG_FULL("ERROR", FMT, ##__VA_ARGS__)

//#define KSLOG_DEBUG(FMT, ...)
#define KSLOG_DEBUG(FMT, ...) a_KSLOG_FULL("DEBUG", FMT, ##__VA_ARGS__)


#endif /* KSLogger_h */
