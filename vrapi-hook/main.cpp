#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/limits.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include "../shared/inline-hook/inlineHook.h"
#include "../shared/utils/utils.h"
#include <dlfcn.h>
#include "VrApi_Input.h"
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
// typedef struct ovrModeParms_
// {
// 	ovrStructureType	Type;

// 	/// Combination of ovrModeFlags flags.
// 	unsigned int		Flags;

// 	/// The Java VM is needed for the time warp thread to create a Java environment.
// 	/// A Java environment is needed to access various system services. The thread
// 	/// that enters VR mode is responsible for attaching and detaching the Java
// 	/// environment. The Java Activity object is needed to get the windowManager,
// 	/// packageName, systemService, etc.
// 	ovrJava				Java;

// 	OVR_VRAPI_PADDING_32_BIT( 4 );

	
// 	unsigned long long	Display;
// 	unsigned long long	WindowSurface;
// 	unsigned long long	ShareContext;
// } ovrModeParms;

// MAKE_HOOK_NAT(open_nat, open, int, char* path, int oflag, mode_t mode)
// {
   
// }


// #define MAKE_HOOK_NAT(name, addr, retval, ...) \
// long addr_ ## name = (long) addr; \
// retval (*name)(__VA_ARGS__) = NULL; \
// retval hook_ ## name(__VA_ARGS__) 

// #define INSTALL_HOOK_NAT(name) \
// registerInlineHook((uint32_t)(addr_ ## name), (uint32_t)hook_ ## name, (uint32_t **)&name);\
// inlineHook((uint32_t)(addr_ ## name));\

ovrTracking2 (*vrapi_GetPredictedTracking2)(ovrMobile * ovr, double absTimeInSeconds );
ovrTracking2 (*vrapi_GetPredictedTracking2)(ovrMobile * ovr, double absTimeInSeconds );
ovrResult (*vrapi_EnumerateInputDevices_dyn)( ovrMobile * ovr, const uint32_t index, ovrInputCapabilityHeader * capsHeader );

void* (*vrapi_EnterVrMode)(void* modeParams) = NULL;

// MAKE_HOOK_NAT(dlopen_nat, dlopen, void*, const char *filename, int flag )
// {
//     log("dlopen called %s %d", filename, flag);
//     return dlopen_nat(filename, flag);
// }
ovrMobile *ovm = NULL;

void setupThread()
{
    log("Setup thread...");
    for ( uint32_t deviceIndex = 0; ; deviceIndex++ )
	{
		ovrInputCapabilityHeader curCaps;
		if ( vrapi_EnumerateInputDevices_dyn(ovm, deviceIndex, &curCaps ) < 0 )
		{
			log("getinputdevices < 0");
			break;
		}
        log("getinputdevices > 0");


		if (curCaps.Type == ovrControllerType_TrackedRemote) {
            log("tracked remote found");
            std::thread t([=]() {
                while (true) {
                    log("thread tick");
                    ovrTracking remoteTracking1;
                    ovrTracking2 remoteTracking = vrapi_GetPredictedTracking2(ovm, .1);
                    
                    ovrResult result = vrapi_GetInputTrackingState(app->GetOvrMobile(),
                                                                   curCaps.DeviceID,
                                                                   .01, &remoteTracking1); 
                    log("x pos %f", remoteTracking. ...HeadPose.Pose.Position.x);
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
            });
            t.detach();

            return;
        }
    }
}

void * hook_vrapi_EnterVrMode(void* modeParams)
{
    log("EnterVrMode called!");
    ovm = (ovrMobile*)vrapi_EnterVrMode(modeParams);
    log("pointer to ovrMobile: %d", ovm);
    setupThread();
    return ovm;
}

// MAKE_HOOK_NAT(dlsym_nat, dlsym, void *, void *handle, const char *symbol)
// {
//     log("dlsym called %s", symbol);
//     return __libc_dlsym(handle, symbol);
// }
// ovrModeParms modeParms = vrapi_DefaultModeParms( &java );
// 		modeParms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
// 		modeParms.Display = eglDisplay;
// 		modeParms.WindowSurface = nativeWindow;
// 		modeParms.ShareContext = eglContext;
// 		ovrMobile * ovr = vrapi_EnterVrMode( &modeParms );

// ovrModeParms modeParms = vrapi_DefaultModeParms( &java );
// 		modeParms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
// 		modeParms.Display = eglDisplay;
// 		modeParms.WindowSurface = nativeWindow;
// 		modeParms.ShareContext = eglContext;
// 		ovrMobile * ovr = vrapi_EnterVrMode( &modeParms );


long addr_vrapi_EnterVrMode;
__attribute__((constructor)) void lib_main()
{
   
    log("Init of vrapi-hook");
    void * vrapiHandle = dlopen("libvrapi.so", RTLD_NOW);
    if (!vrapiHandle)
    {
        log("Did not open vrapi library!");
        return;
    }
    
    log("opened vrapi");
    //*(void**)(&vrapi_EnterVrMode) = 
    long addr_vrapi_EnterVrMode = (long)dlsym(vrapiHandle, "vrapi_EnterVrMode");
    if (!addr_vrapi_EnterVrMode)
    {
        log("Did not find vrapi_EnterVrMode!");
        return;
    }
    log("found vrapi_EnterVrMode");
    *(void**)(&vrapi_GetPredictedTracking2) = dlsym(vrapiHandle, "vrapi_GetPredictedTracking2");
    if (!&vrapi_GetPredictedTracking2)
    {
        log("couldn't load vrapi_GetPredictedTracking2");
        return;
    }
    log("found vrapi_GetPredictedTracking2");
    *(void**)(&vrapi_EnumerateInputDevices_dyn) = dlsym(vrapiHandle, "vrapi_EnumerateInputDevices");
    if (!&vrapi_EnumerateInputDevices_dyn)
    {
        log("couldn't load vrapi_EnumerateInputDevices");
        return;
    }
    log("found vrapi_EnumerateInputDevices");
    registerInlineHook((uint32_t)addr_vrapi_EnterVrMode, (uint32_t)hook_vrapi_EnterVrMode, (uint32_t **)&vrapi_EnterVrMode);
    log("registered hook");
    inlineHook((uint32_t)addr_vrapi_EnterVrMode);      
    log("inline hook complete");
    //INSTALL_HOOK_NAT(dlopen_nat);
   // INSTALL_HOOK_NAT(dlsym_nat);
}