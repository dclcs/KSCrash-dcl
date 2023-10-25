//
//  KSCrashMonitor_NSException.m
//  KSCrash-dcl
//
//  Created by dcl on 2023/10/24.
//

#import "KSCrashMonitor_NSException.h"
#import "KSLogger.h"


#pragma mark - Globals -
// ============================================================================

static volatile bool g_isEnabled = 0;

/** The exception handler that was in place before we installed ours. */
static NSUncaughtExceptionHandler* g_previousUncaughtExceptionHandler;

static void handleException(NSException* exception, BOOL currentSnapshotUserReported)
{
    KSLOG_DEBUG(@"Trapped exception %@", exception);
    
    if (g_isEnabled)
    {
        thread_act_array_t threads = NULL;
        mach_msg_type_number_t numThreads = 0;
        // TODO: ksmc_suspendEnvironment & kscm_notifyFatalExceptionCaptured
        
        KSLOG_DEBUG(@"Filling out context.");
        NSArray *addresses = [exception callStackReturnAddresses];
    }
}


static void handlCurrentSnapshotUserReportedException(NSException *exception)
{
    handleException(exception, true);
}


static void handleUncaughtException(NSException *exception)
{
    handleException(exception, false);
}

static void setEnabled(bool isEnabled)
{
    if (isEnabled != g_isEnabled)
    {
        g_isEnabled = isEnabled;
        if (isEnabled)
        {
            KSLOG_DEBUG(@"Backing up original handler.");
            g_previousUncaughtExceptionHandler = NSGetUncaughtExceptionHandler();
            
            KSLOG_DEBUG(@"Setting new handler.");
            NSSetUncaughtExceptionHandler(&handleUncaughtException);
        }
        else
        {
            KSLOG_DEBUG(@"Restoring original handler.");
            NSSetUncaughtExceptionHandler(g_previousUncaughtExceptionHandler);
        }
    }
}

static bool isEnabled(void)
{
    return g_isEnabled;
}

KSCrashMonitorAPI* kscm_nsexception_getAPI(void)
{
    static KSCrashMonitorAPI api = 
    {
        .setEnabled = setEnabled,
        .isEnabled = isEnabled
    };
    return &api;
}
