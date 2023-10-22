//
//  KSThread.c
//  KSCrash-dcl
//
//  Created by cl d on 2023/10/21.
//

#include "KSThread.h"
#include <mach/mach.h>

KSThread ksthread_self(void)
{
    thread_t thread_self = mach_thread_self();
    mach_port_deallocate(mach_task_self(), thread_self);
    return (KSThread)thread_self;
}
