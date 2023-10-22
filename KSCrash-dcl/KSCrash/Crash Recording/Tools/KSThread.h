//
//  KSThread.h
//  KSCrash-dcl
//
//  Created by cl d on 2023/10/21.
//

#ifndef KSThread_h
#define KSThread_h

#include <stdio.h>
typedef uintptr_t KSThread;



/* Get the current mach thread ID.
 * mach_thread_self() receives a send right for the thread port which needs to
 * be deallocated to balance the reference count. This function takes care of
 * all of that for you.
 *
 * @return The current thread ID.
 */
KSThread ksthread_self(void);
#endif /* KSThread_h */
