//
//  main.m
//  KSCrash-dcl
//
//  Created by cl d on 2023/10/21.
//

#import <UIKit/UIKit.h>
#import "AppDelegate.h"
#import "KSCrash/Crash Recording/Monitors/KSCrashMonitor_MachException.h"
int main(int argc, char * argv[]) {
    NSString * appDelegateClassName;
    installExceptionHandler();
    @autoreleasepool {
        
        // Setup code that might create autoreleased objects goes here.
        appDelegateClassName = NSStringFromClass([AppDelegate class]);
    }
    return UIApplicationMain(argc, argv, nil, appDelegateClassName);
}
