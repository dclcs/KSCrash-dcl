//
//  KSCrashMonitor_MachException.h
//  KSCrash-dcl
//
//  Created by cl d on 2023/10/21.
//

#ifndef KSCrashMonitor_MachException_h
#define KSCrashMonitor_MachException_h



#include "KSCrashMonitor.h"

KSCrashMonitorAPI* kscm_machexception_getAPI(void);

bool installExceptionHandler(void);

#endif /* KSCrashMonitor_MachException_h */
