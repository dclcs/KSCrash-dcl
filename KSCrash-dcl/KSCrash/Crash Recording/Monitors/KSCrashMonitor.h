//
//  KSCrashMonitor.h
//  KSCrash-dcl
//
//  Created by cl d on 2023/10/21.
//

#ifndef KSCrashMonitor_h
#define KSCrashMonitor_h

#include <stdio.h>
#include <stdbool.h>

struct KSCrash_MonitorContext;

// ============================================================================
#pragma mark - Internal API
// ============================================================================

typedef struct
{
    void (*setEnabled)(bool isEnabled);
    bool (*isEnabled)(void);
} KSCrashMonitorAPI;

#endif /* KSCrashMonitor_h */
