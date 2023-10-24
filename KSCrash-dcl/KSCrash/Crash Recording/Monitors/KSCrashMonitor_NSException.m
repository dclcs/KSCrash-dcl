//
//  KSCrashMonitor_NSException.m
//  KSCrash-dcl
//
//  Created by dcl on 2023/10/24.
//

#import "KSCrashMonitor_NSException.h"
#import "KSLogger.h"


static void handleException(NSException* exception, BOOL currentSnapshotUserReported)
{
    KSLOG_DEBUG(@"Trapped exception %@", exception);
}


KSCrashMonitorAPI* kscm_nsexception_getAPI(void)
{
    static KSCrashMonitorAPI api = 
    {
        
    };
    return &api;
}
