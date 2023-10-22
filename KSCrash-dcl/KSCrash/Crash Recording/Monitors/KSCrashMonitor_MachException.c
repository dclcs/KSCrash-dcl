//
//  KSCrashMonitor_MachException.c
//  KSCrash-dcl
//
//  Created by cl d on 2023/10/21.
//

#include "KSCrashMonitor_MachException.h"
#include "KSLogger.h"
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <pthread/pthread.h>
#include "KSThread.h"

// ============================================================================
#pragma mark - Constants -
// ============================================================================

static const char* kThreadPrimary = "KSCrash Exception Handler (Primary)";
static const char* kThreadSecondary = "KSCrash Exception Handler (Secondary)";

static bool g_isHandlingCrash = false;
/** A mach exception message (according to ux_exception.c, xnu-1699.22.81).
 */
#pragma pack(4)
typedef struct
{
    /** Mach header. */
    mach_msg_header_t          header;

    // Start of the kernel processed data.

    /** Basic message body data. */
    mach_msg_body_t            body;

    /** The thread that raised the exception. */
    mach_msg_port_descriptor_t thread;

    /** The task that raised the exception. */
    mach_msg_port_descriptor_t task;

    // End of the kernel processed data.

    /** Network Data Representation. */
    NDR_record_t               NDR;

    /** The exception that was raised. */
    exception_type_t           exception;

    /** The number of codes. */
    mach_msg_type_number_t     codeCount;

    /** Exception code and subcode. */
    // ux_exception.c defines this as mach_exception_data_t for some reason.
    // But it's not actually a pointer; it's an embedded array.
    // On 32-bit systems, only the lower 32 bits of the code and subcode
    // are valid.
    mach_exception_data_type_t code[0];

    /** Padding to avoid RCV_TOO_LARGE. */
    char                       padding[512];
} MachExceptionMessage;
#pragma pack()

/** A mach reply message (according to ux_exception.c, xnu-1699.22.81).
 */
#pragma pack(4)
typedef struct
{
    /** Mach header. */
    mach_msg_header_t header;

    /** Network Data Representation. */
    NDR_record_t      NDR;

    /** Return code. */
    kern_return_t     returnCode;
} MachReplyMessage;
#pragma pack()

// ============================================================================
#pragma mark - Globals -
// ============================================================================

static volatile bool g_isEnabled = true;


/** Holds exception port info regarding the previously installed exception
 * handlers.
 */
static struct
{
    exception_mask_t        masks[EXC_TYPES_COUNT];
    exception_handler_t     ports[EXC_TYPES_COUNT];
    exception_behavior_t    behaviors[EXC_TYPES_COUNT];
    thread_state_flavor_t   flavors[EXC_TYPES_COUNT];
    mach_msg_type_number_t  count;
} g_previousExceptionPorts;

/** Our exception port. */
static mach_port_t g_exceptionPort = MACH_PORT_NULL;

/** Primary exception handler thread. */
static pthread_t g_primaryPThread;
static thread_t g_primaryMachThread;

/** Secondary exception handler thread in case crash handler crashes. */
static pthread_t g_secondaryPThread;
static thread_t g_secondaryMachThread;

static char g_primaryEventID[37];
static char g_secondaryEventID[37];



// ============================================================================
#pragma mark - Handler -
// ============================================================================

/** Our exception handler thread routine.
 * Wait for an exception message, uninstall our exception port, record the
 * exception information, and write a report.
 */
static void* handleExceptions(void* const userData)
{
    MachExceptionMessage exceptionMessage = {{0}};
    MachReplyMessage replayMessage = {{0}};
    char* eventID = g_primaryEventID;
    
    const char *threadName = (const char*)userData;
    KSLOG_DEBUG("handleException threadName: %s", threadName);
    if (threadName == kThreadSecondary)
    {
        KSLOG_DEBUG("This is the secondary thread. Suspending.");
        thread_suspend((thread_t)ksthread_self());
        eventID = g_secondaryEventID;
    }
    pthread_setname_np(threadName);
    
    for (; ;) 
    {
        KSLOG_DEBUG("Waiting for mach exception");
        
        kern_return_t kr = mach_msg(&exceptionMessage.header,
                                    MACH_RCV_MSG,
                                    0,
                                    sizeof(exceptionMessage),
                                    g_exceptionPort,
                                    MACH_MSG_TIMEOUT_NONE,
                                    MACH_PORT_NULL);
        if(kr == KERN_SUCCESS)
        {
            break;
        }
        
        KSLOG_ERROR("mach_msg: %s", mach_error_string(kr));
    }
    
    KSLOG_DEBUG("Trapped mach exception code 0x%llx, subcode 0x%llx", exceptionMessage.code[0], exceptionMessage.code[1]);
    
    if (g_isEnabled)
    {
        thread_act_array_t threads = NULL;
        mach_msg_type_number_t numThreads = 0;
        // TODO: kmsc_suspendEnvironment
        g_isHandlingCrash = true;
        // TODO: kmsc_notifyFatalExceptionCaptured
        KSLOG_DEBUG("Exception handler is installed. Continuing exception handling.");
        
        if(ksthread_self() == g_primaryMachThread)
        {
            KSLOG_DEBUG("This is the primary exception thread. Activating secondary thread.");
            //TODO: restoreExceptionPorts();
            if (thread_resume(g_secondaryMachThread) != KERN_SUCCESS)
            {
                KSLOG_DEBUG("Could not activate secondary thread. Restoring original exception ports.");
            }
        }
        else
        {
            KSLOG_DEBUG("This is the secondary exception thread.");
        }
        
        // Fill out crash information
        KSLOG_DEBUG("Fetching machine state.");
//        KSMC_NEW_CONTEXT(machineContext);
//        KSCrash_MonitorContext* crashContext = &g_monitorContext;
//        crashContext->offendingMachineContext = machineContext;
//        kssc_initCursor(&g_stackCursor, NULL, NULL);
//        if(ksmc_getContextForThread(exceptionMessage.thread.name, machineContext, true))
//        {
//            kssc_initWithMachineContext(&g_stackCursor, KSSC_MAX_STACK_DEPTH, machineContext);
//            KSLOG_TRACE("Fault address %p, instruction address %p",
//                        kscpu_faultAddress(machineContext), kscpu_instructionAddress(machineContext));
//            if(exceptionMessage.exception == EXC_BAD_ACCESS)
//            {
//                crashContext->faultAddress = kscpu_faultAddress(machineContext);
//            }
//            else
//            {
//                crashContext->faultAddress = kscpu_instructionAddress(machineContext);
//            }
//        }
//        KSLOG_DEBUG("Filling out context.");
//        crashContext->crashType = KSCrashMonitorTypeMachException;
//        crashContext->eventID = eventID;
//        crashContext->registersAreValid = true;
//        crashContext->mach.type = exceptionMessage.exception;
//        crashContext->mach.code = exceptionMessage.code[0] & (int64_t)MACH_ERROR_CODE_MASK;
//        crashContext->mach.subcode = exceptionMessage.code[1] & (int64_t)MACH_ERROR_CODE_MASK;
//        if(crashContext->mach.code == KERN_PROTECTION_FAILURE && crashContext->isStackOverflow)
//        {
//            // A stack overflow should return KERN_INVALID_ADDRESS, but
//            // when a stack blasts through the guard pages at the top of the stack,
//            // it generates KERN_PROTECTION_FAILURE. Correct for this.
//            crashContext->mach.code = KERN_INVALID_ADDRESS;
//        }
//        crashContext->signal.signum = signalForMachException(crashContext->mach.type, crashContext->mach.code);
//        crashContext->stackCursor = &g_stackCursor;
//
//        kscm_handleException(crashContext);
//
//        KSLOG_DEBUG("Crash handling complete. Restoring original handlers.");
//        g_isHandlingCrash = false;
//        ksmc_resumeEnvironment(threads, numThreads);
    }
    KSLOG_DEBUG("Replying to mach exception message.");
    replayMessage.header = exceptionMessage.header;
    replayMessage.NDR = exceptionMessage.NDR;
    replayMessage.returnCode = KERN_FAILURE;
    
    mach_msg(&replayMessage.header,
             MACH_SEND_MSG,
             sizeof(replayMessage),
             0,
             MACH_PORT_NULL,
             MACH_MSG_TIMEOUT_NONE,
             MACH_PORT_NULL);
    
    return NULL;
}

bool installExceptionHandler(void)
{
    KSLOG_DEBUG("Installing mach exception handler");
    bool attributes_created = false;
    pthread_attr_t attr;
    
    kern_return_t kr;
    int error;
    
    const task_t thisTask = mach_task_self();
    exception_mask_t mask = EXC_MASK_BAD_ACCESS |
    EXC_MASK_BAD_INSTRUCTION |
    EXC_MASK_ARITHMETIC |
    EXC_MASK_SOFTWARE |
    EXC_MASK_BREAKPOINT;
    
    KSLOG_DEBUG("Backing up original exception ports.");
    kr = task_get_exception_ports(thisTask,
                                  mask, 
                                  g_previousExceptionPorts.masks,
                                  &g_previousExceptionPorts.count,
                                  g_previousExceptionPorts.ports,
                                  g_previousExceptionPorts.behaviors,
                                  g_previousExceptionPorts.flavors);
    if (kr != KERN_SUCCESS)
    {
        KSLOG_DEBUG("task_get_exception_ports: %s", mach_error_string(kr));
        goto failed;
    }
    
    if (g_exceptionPort == MACH_PORT_NULL)
    {
        KSLOG_DEBUG("Allocating new port with receive rights.");
        kr = mach_port_allocate(thisTask,
                                MACH_PORT_RIGHT_RECEIVE,
                                &g_exceptionPort);
        
        if (kr != KERN_SUCCESS)
        {
            KSLOG_DEBUG("mach_port_allocate: %@", mach_error_string(kr));
            goto failed;
        }
        KSLOG_DEBUG("Adding send rights to port.");
        kr = mach_port_insert_right(thisTask,
                                    g_exceptionPort,
                                    g_exceptionPort,
                                    MACH_MSG_TYPE_MAKE_SEND);
        
        if (kr != KERN_SUCCESS)
        {
            KSLOG_ERROR("mach_port_insert_right: %s", mach_error_string(kr));
            goto failed;
        }
    }
    
    KSLOG_DEBUG("Installing port as exception handler.");
    kr = task_set_exception_ports(thisTask,
                                  mask,
                                  g_exceptionPort,
                                  (int)(EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES),
                                  THREAD_STATE_NONE);
    if(kr != KERN_SUCCESS)
    {
        KSLOG_ERROR("task_set_exception_ports: %s", mach_error_string(kr));
        goto failed;
    }
    
    KSLOG_DEBUG("Creating secondary exception thread (suspended).");
    pthread_attr_init(&attr);
    attributes_created = true;
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    error = pthread_create(&g_secondaryPThread,
                           &attr,
                           &handleExceptions,
                           (void*)kThreadSecondary);
    if (error != 0)
    {
        KSLOG_ERROR("pthread_suspended_np: %s", strerror(error));
        goto failed;
    }
    g_secondaryMachThread = pthread_mach_thread_np(g_secondaryPThread);
    //TODO: ksmc_addReservedThread
    
    KSLOG_DEBUG("Creating primary exception thread.");
    error = pthread_create(&g_primaryPThread,
                           &attr,
                           &handleExceptions,
                           (void*)kThreadPrimary);
    if (error != 0)
    {
        KSLOG_ERROR("pthread_create: %s", strerror(error));
        goto failed;
    }
    
    pthread_attr_destroy(&attr);
    g_primaryMachThread = pthread_mach_thread_np(g_primaryPThread);
    //TODO: ksmc_addReservedThread
    KSLOG_DEBUG("Mach exception handler installed");
    return true;
    
failed:
    KSLOG_DEBUG("Failed to install mach exception handler.");
    if (attributes_created)
    {
        pthread_attr_destroy(&attr);
    }
    //TODO: uninstallExceptionHandler
    return false;
}


static void setEnabled(bool isEnabled)
{
    
}


static bool isEnabled(void)
{
    return true;
}




KSCrashMonitorAPI *kscm_machexception_getAPI(void)
{
    static KSCrashMonitorAPI api =
    {
        .setEnabled = setEnabled,
        .isEnabled = isEnabled
    };
    return &api;
}
