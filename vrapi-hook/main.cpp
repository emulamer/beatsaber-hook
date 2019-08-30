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
#include "inline-hook/inlineHook.h"
#include <dlfcn.h>
#include "VrApi_Input.h"
#include <thread>
#include <chrono>
#include <string>
#include "main.h"
#include "fakes.h"
#include <utility>
#include <set>
//long name ## _addr = (long) addr; 
#define HOOK_DEF(name, retval, ...) \
retval (*name ## _def)(__VA_ARGS__) = NULL; \
retval hook_ ## name(__VA_ARGS__) 

#define INIT_HOOK(libhandle, name) \
long addr_ ## name = (long)dlsym(libhandle, #name); \
if (!addr_ ## name) \
{ \
    err("Could not dlsym " #name ); \
    return; \
} \
registerInlineHook((uint32_t)(addr_ ## name), (uint32_t)hook_ ## name, (uint32_t **)&name ## _def);\
inlineHook((uint32_t)(addr_ ## name));

#define LOGLOTS 0

#define log(...) __android_log_print(ANDROID_LOG_INFO, "VRAPIHook", __VA_ARGS__)
#define err(...) __android_log_print(ANDROID_LOG_ERROR, "VRAPIHook", __VA_ARGS__)



#pragma region leaderboards
typedef enum ovrLeaderboardFilterType_ {
  ovrLeaderboard_FilterNone,
  ovrLeaderboard_FilterFriends,
  ovrLeaderboard_FilterUnknown,
} ovrLeaderboardFilterType;
typedef uint64_t ovrRequest;

typedef struct ovrLeaderboardEntryArray *ovrLeaderboardEntryArrayHandle;
typedef struct ovrLeaderboardEntry *ovrLeaderboardEntryHandle;
typedef struct ovrLeaderboardUpdateStatus *ovrLeaderboardUpdateStatusHandle;
typedef struct ovrMessage *ovrMessageHandle;
typedef struct ovrError *ovrErrorHandle;
typedef struct ovrUser *ovrUserHandle;
typedef struct ovrAssetDetails *ovrAssetDetailsHandle;
typedef struct ovrLanguagePackInfo *ovrLanguagePackInfoHandle;
typedef uint64_t ovrID;



HOOK_DEF(ovr_FreeMessage, void, ovrMessageHandle handle)
{
   // log("ovr_FreeMessage called");
    ovr_FreeMessage_def(handle);
}
HOOK_DEF(ovr_Leaderboard_GetEntries, ovrRequest, const char *leaderboardName, int limit, void * filter, void * startAt) 
{
    ovrRequest res = ovr_Leaderboard_GetEntries_def(leaderboardName, limit, filter, startAt);
    log("ovr_Leaderboard_GetEntries called with leaderboard name %s", leaderboardName);
    return res;
}
HOOK_DEF(ovr_Leaderboard_GetEntriesAfterRank, ovrRequest, const char *leaderboardName, int limit, unsigned long long afterRank)
{
    ovrRequest res = ovr_Leaderboard_GetEntriesAfterRank_def(leaderboardName, limit, afterRank);
    log("ovr_Leaderboard_GetEntriesAfterRank called for leaderboard name %s limit %d, afterRank %d, with result %d", leaderboardName, limit, afterRank, res);
    return res;
}
HOOK_DEF(ovr_Leaderboard_GetNextEntries, ovrRequest, const ovrLeaderboardEntryArrayHandle handle)
{
    ovrRequest res = ovr_Leaderboard_GetNextEntries_def(handle);
    log("ovr_Leaderboard_GetNextEntries called");
    return res;
}
HOOK_DEF(ovr_Leaderboard_GetPreviousEntries, ovrRequest, const ovrLeaderboardEntryArrayHandle handle)
{
    ovrRequest res = ovr_Leaderboard_GetPreviousEntries_def(handle);
    log("ovr_Leaderboard_GetPreviousEntries called");
    return res;
}

HOOK_DEF(ovr_Leaderboard_WriteEntry, ovrRequest, const char *leaderboardName, long long score, const void *extraData, unsigned int extraDataLength, bool forceUpdate)
{   
    ovrRequest res = ovr_Leaderboard_WriteEntry_def(leaderboardName, score, extraData, extraDataLength, forceUpdate);
    log("ovr_Leaderboard_WriteEntry called for leaderboard %s with score %d extraDataLength %d, forceUpdate %d", leaderboardName, score, extraDataLength, forceUpdate);
    return res;
}
HOOK_DEF(ovrMessageType_ToString, const char*, ovrMessageType value)
{
    log ("ovrMessageType_ToString called");
    return ovrMessageType_ToString_def(value);
}
HOOK_DEF(ovr_Message_GetType, ovrMessageType, const ovrMessageHandle obj)
{
    if (Fake::IsFake(obj)) {
        auto fakeRet = ((FakeMessage*)obj)->MessageType;
        log("ovr_Message_GetType called for handle %d, fake identified, returning %d", obj, fakeRet);
        return fakeRet;
    }
    auto ret = ovr_Message_GetType_def(obj);
    log("ovr_Message_GetType called with handle %d, returned %d", obj, ret);
    return ret;
}
HOOK_DEF(ovr_Message_IsError, bool, const ovrMessageHandle obj)
{
    if (Fake::IsFake(obj)) {
        auto fakeRet = ((FakeMessage*)obj)->IsError;
        log("ovr_Message_IsError called for handle %d, fake identified, returning %d", obj, fakeRet);
        return fakeRet;
    }
    bool ret = ovr_Message_IsError_def(obj);    
    log("ovr_Message_IsError called with %d, returned %d", obj, ret);
    return ret;
}


HOOK_DEF(ovr_PopMessage, ovrMessageHandle )
{
    //log("ovr_PopMessage called");
    ovrMessageHandle handle = ovr_PopMessage_def();
    if (handle == 0)
    {
        //todo, this is where responses can be injected when ready
        return handle;
    }
    ovrMessageType type = ovr_Message_GetType_def(handle);
    log("handle is %d", handle);
    const char* typeName = ovrMessageType_ToString_def(type);
    log("message type was %s", typeName);
    if (type == ovrMessageType::ovrMessage_Leaderboard_GetEntriesAfterRank) {
        log("popped message type ovrMessage_Leaderboard_GetEntriesAfterRank, handle val %d", handle);
        if (ovr_Message_IsError_def(handle)) {
            /*todo:
                eat this, and return a 0 to the parent.
                start a thread to do the web request
                add code above to check for when handle is 0:
                    loop through all fake messages, look for one that IsReady = true
                    send that one out

                    implement deleting stuff on free of message
             */
            log("****THIS CALL WAS ERROR RESPONSE, THIS IS WHERE TO INJECT****");
            FakeLeaderboardEntryArrayMessage *m = new FakeLeaderboardEntryArrayMessage();
            
            FakeLeaderboardEntry *e1 = new FakeLeaderboardEntry();
            e1->Rank = 10;
            e1->Score = 100000;
            e1->User.OculusID = "testguy";
            FakeLeaderboardEntry *e2 = new FakeLeaderboardEntry();
            e2->Rank = 10;
            e2->Score = 100000;
            e2->User.OculusID = "testguy";
            FakeLeaderboardEntry *e3 = new FakeLeaderboardEntry();
            e3->Rank = 10;
            e3->Score = 100000;
            e3->User.OculusID = "testguy";
            
            m->Entries.push_back(*e1);
            m->Entries.push_back(*e2);
            m->Entries.push_back(*e3);
            ovr_FreeMessage_def(handle);
            log("returning handle %d", m);
            return (ovrMessageHandle)m;
        }
    }

    return handle;
}
HOOK_DEF(ovr_Message_GetError, ovrErrorHandle, const ovrMessageHandle obj)
{
    log("ovr_Message_GetError called with %d, valueat is %d", obj);
    return ovr_Message_GetError_def(obj);
}
HOOK_DEF(ovr_Message_GetRequestID, ovrRequest, const ovrMessageHandle obj)
{
    auto ret = ovr_Message_GetRequestID_def(obj);
    log("ovr_Message_GetRequestID called with %d, returned %d", obj, ret);
    return ret;
}
HOOK_DEF(ovr_Message_GetLeaderboardEntryArray, ovrLeaderboardEntryArrayHandle, const ovrMessageHandle obj)
{
    if (Fake::IsFake(obj)) {
        auto fakeRet = ((ovrLeaderboardEntryArrayHandle)obj);
        log("ovr_Message_GetLeaderboardEntryArray called for handle %d, fake identified, returning %d", obj, fakeRet);
        return fakeRet;
    }
    
    ovrLeaderboardEntryArrayHandle ret = ovr_Message_GetLeaderboardEntryArray_def(obj);
    log("ovr_Message_GetLeaderboardEntryArray called with %d, returning %d", obj, ret);
    return ret;
}
HOOK_DEF(ovr_LeaderboardEntryArray_GetElement, ovrLeaderboardEntryHandle, const ovrLeaderboardEntryArrayHandle obj, size_t index)
{
    if (Fake::IsFake(obj)) {
        auto fakeRet = (ovrLeaderboardEntryHandle)&((FakeLeaderboardEntryArrayMessage*)obj)->Entries.at(index);
        log("ovr_LeaderboardEntryArray_GetElement called for handle %d, fake identified, returning %d", obj, fakeRet);
        return fakeRet;
    }
    log("ovr_LeaderboardEntryArray_GetElement called with %d, idx %d", obj, index);
    ovrLeaderboardEntryHandle ret = ovr_LeaderboardEntryArray_GetElement_def(obj, index);
    return ret;
}
HOOK_DEF(ovr_LeaderboardEntryArray_GetNextUrl, const char *, const ovrLeaderboardEntryArrayHandle obj)
{
    log("ovr_LeaderboardEntryArray_GetNextUrl called");
    const char * ret = ovr_LeaderboardEntryArray_GetNextUrl_def(obj);
    return ret;
}
HOOK_DEF(ovr_LeaderboardEntryArray_GetPreviousUrl, const char *, const ovrLeaderboardEntryArrayHandle obj)
{
    const char * ret = ovr_LeaderboardEntryArray_GetPreviousUrl_def(obj);
    log("ovr_LeaderboardEntryArray_GetPreviousUrl called, returned: %s", ret);
    return ret;
}
HOOK_DEF(ovr_LeaderboardEntryArray_GetSize, size_t, const ovrLeaderboardEntryArrayHandle obj)
{
    if (Fake::IsFake(obj)) {
        auto fakeRet = ((FakeLeaderboardEntryArrayMessage*)obj)->Entries.size();
        log("ovr_LeaderboardEntryArray_GetSize called for handle %d, fake identified, returning %d", obj, fakeRet);
        return fakeRet;
    }
    size_t ret = ovr_LeaderboardEntryArray_GetSize_def(obj);
    log("ovr_LeaderboardEntryArray_GetSize called, returned %d", ret);
    return ret;
}
HOOK_DEF(ovr_LeaderboardEntryArray_GetTotalCount, unsigned long long, const ovrLeaderboardEntryArrayHandle obj)
{
    unsigned long long ret = ovr_LeaderboardEntryArray_GetTotalCount_def(obj);
    log("ovr_LeaderboardEntryArray_GetTotalCount called, returned %d", ret);
    return ret;
}
HOOK_DEF(ovr_LeaderboardEntryArray_HasNextPage, bool, const ovrLeaderboardEntryArrayHandle obj)
{
    bool ret = ovr_LeaderboardEntryArray_HasNextPage_def(obj);
    log("ovr_LeaderboardEntryArray_HasNextPage called, returned %d", ret);
    return ret;
}
HOOK_DEF(ovr_LeaderboardEntryArray_HasPreviousPage, bool, const ovrLeaderboardEntryArrayHandle obj)
{
    bool ret = ovr_LeaderboardEntryArray_HasPreviousPage_def(obj);
    log("ovr_LeaderboardEntryArray_HasPreviousPage called, returned %d", ret);
    return ret;
}
HOOK_DEF(ovr_LeaderboardEntry_GetExtraData, const char *, const ovrLeaderboardEntryHandle obj)
{
    if (Fake::IsFake(obj)) {
        log("ovr_LeaderboardEntry_GetExtraData called for handle %d, fake identified, returning null", obj);
        return NULL;
    }
    const char * ret = ovr_LeaderboardEntry_GetExtraData_def(obj);
    log("ovr_LeaderboardEntry_GetExtraData called with %d, returned %s", obj, ret);
    return ret;
}
HOOK_DEF(ovr_LeaderboardEntry_GetExtraDataLength, unsigned int, const ovrLeaderboardEntryHandle obj)
{
    log("ovr_LeaderboardEntry_GetExtraDataLength called with %d", obj);
    if (Fake::IsFake(obj)) {
        log("ovr_LeaderboardEntry_GetExtraDataLength called for handle %d, fake identified, returning 0", obj);
        return 0;
    }
    unsigned int ret = ovr_LeaderboardEntry_GetExtraDataLength_def(obj);
    log("ovr_LeaderboardEntry_GetExtraDataLength called, returned %d", ret);
    return ret;
}
HOOK_DEF(ovr_LeaderboardEntry_GetRank, int, const ovrLeaderboardEntryHandle obj)
{    
   if (Fake::IsFake(obj)) {
        auto fakeRet = ((FakeLeaderboardEntry*)obj)->Rank;
        log("ovr_LeaderboardEntry_GetRank called for handle %d, fake identified, returning %d", obj, fakeRet);
        return fakeRet;
    }
    int ret = ovr_LeaderboardEntry_GetRank_def(obj);
    log("ovr_LeaderboardEntry_GetRank called with %d, returned %d", obj, ret);
    return ret;
}
HOOK_DEF(ovr_LeaderboardEntry_GetScore, long long, const ovrLeaderboardEntryHandle obj)
{
    if (Fake::IsFake(obj)) {
        auto fakeRet = ((FakeLeaderboardEntry*)obj)->Score;
        log("ovr_LeaderboardEntry_GetScore called for handle %d, fake identified, returning %d", obj, fakeRet);
        return fakeRet;
    }
    long long ret = ovr_LeaderboardEntry_GetScore_def(obj);
    log("ovr_LeaderboardEntry_GetScore called with %d, returned %d", obj, ret);
    return ret;
}
HOOK_DEF(ovr_LeaderboardEntry_GetTimestamp, unsigned long long, const ovrLeaderboardEntryHandle obj)
{
    if (Fake::IsFake(obj)) {
        auto fakeRet = ((FakeLeaderboardEntry*)obj)->Timestamp;
        log("ovr_LeaderboardEntry_GetTimestamp called for handle %d, fake identified, returning %d", obj, fakeRet);
        return fakeRet;
    }
    unsigned long long ret = ovr_LeaderboardEntry_GetTimestamp_def(obj);
    log("ovr_LeaderboardEntry_GetTimestamp called, returned %d", ret);
    return ret;
}
HOOK_DEF(ovr_LeaderboardEntry_GetUser, ovrUserHandle, const ovrLeaderboardEntryHandle obj)
{
    if (Fake::IsFake(obj)) {
        auto fakeRet = (ovrUserHandle)&((FakeLeaderboardEntry*)obj)->User;
        log("ovr_LeaderboardEntry_GetUser called for handle %d, fake identified, returning %d", obj, fakeRet);
        return fakeRet;
    }
    ovrUserHandle ret = ovr_LeaderboardEntry_GetUser_def(obj);
    log("ovr_LeaderboardEntry_GetUser called with %d, returning %d", obj, ret);
    return ret;
}
HOOK_DEF(ovr_User_GetOculusID, const char *, const ovrUserHandle obj)
{
    if (Fake::IsFake(obj)) {
        auto fakeRet =((FakeUser*)obj)->OculusID;
        log("ovr_User_GetOculusID called for handle %d, fake identified, returning %s", obj, fakeRet);
        return fakeRet;
    }
    const char * ret = ovr_User_GetOculusID_def(obj);
    log("ovr_User_GetOculusID called with %d, returned %s", obj, ret);
    return ret;
}
HOOK_DEF(ovr_Message_GetNativeMessage, ovrMessageHandle, const ovrMessageHandle obj)
{
    if (Fake::IsFake(obj)) {
        log("ovr_Message_GetNativeMessage called, returning original");
        return obj;
    }
    auto ret = ovr_Message_GetNativeMessage_def(obj);
    log("ovr_Message_GetNativeMessage called with %d, returned %d", obj, ret);
    return ret;
}
HOOK_DEF(ovr_User_GetPresence, const char *, const ovrUserHandle obj)
{
    if (Fake::IsFake(obj)) {
        auto fakeRet =((FakeUser*)obj)->Presence;
        log("ovr_User_GetPresence called for handle %d, fake identified, returning %s", obj, fakeRet);
        return fakeRet;
    }
    auto ret = ovr_User_GetPresence_def(obj);
    log("ovr_User_GetPresence called with %d, result: %s", obj, ret);
    return ret;
}
HOOK_DEF(ovr_User_GetPresenceStatus, ovrUserPresenceStatus, const ovrUserHandle obj)
{
    if (Fake::IsFake(obj)) {
        auto fakeRet =((FakeUser*)obj)->PresenceStatus;
        log("ovr_User_GetPresenceStatus called for handle %d, fake identified, returning %d", obj, fakeRet);
        return fakeRet;
    }
    auto ret = ovr_User_GetPresenceStatus_def(obj);
    log("ovr_User_GetPresenceStatus called with %d, result: %d", obj, ret);
    return ret;
}

HOOK_DEF(ovr_User_GetID, ovrID, const ovrUserHandle obj)
{
    if (Fake::IsFake(obj)) {
        auto fakeRet =((FakeUser*)obj)->UserID;
        log("ovr_User_GetID called for handle %d, fake identified, returning %d", obj, fakeRet);
        return fakeRet;
    }
    auto ret = ovr_User_GetID_def(obj);
    log("ovr_User_GetID called with %d, result: %d", obj, ret);
    return ret;
}
HOOK_DEF(ovr_User_GetImageUrl,const char *, const ovrUserHandle obj)
{
    if (Fake::IsFake(obj)) {
        auto fakeRet =((FakeUser*)obj)->ImageUrl;
        log("ovr_User_GetImageUrl called for handle %d, fake identified, returning %s", obj, fakeRet);
        return fakeRet;
    }
    auto ret = ovr_User_GetImageUrl_def(obj);
    log("ovr_User_GetImageUrl called with %d, result: %s", obj, ret);
    return ret;
}
HOOK_DEF(ovr_User_GetInviteToken, const char *, const ovrUserHandle obj)
{
    if (Fake::IsFake(obj)) {
        auto fakeRet =((FakeUser*)obj)->InviteToken;
        log("ovr_User_GetInviteToken called for handle %d, fake identified, returning %s", obj, fakeRet);
        return fakeRet;
    }
    auto ret = ovr_User_GetInviteToken_def(obj);
    log("ovr_User_GetInviteToken called with %d, result: %s", obj, ret);
    return ret;
}
HOOK_DEF(ovr_User_GetSmallImageUrl, const char *, const ovrUserHandle obj)
{
    if (Fake::IsFake(obj)) {
        auto fakeRet =((FakeUser*)obj)->SmallImageUrl;
        log("ovr_User_GetSmallImageUrl called for handle %d, fake identified, returning %s", obj, fakeRet);
        return fakeRet;
    }
    auto ret = ovr_User_GetSmallImageUrl_def(obj);
    log("ovr_User_GetSmallImageUrl called with %d, result: %s", obj, ret);
    return ret;
}

HOOK_DEF(ovr_Message_GetLeaderboardUpdateStatus, ovrLeaderboardUpdateStatusHandle, const ovrMessageHandle obj)
{
    log("ovr_Message_GetLeaderboardUpdateStatus called");
    return ovr_Message_GetLeaderboardUpdateStatus_def(obj);
}
HOOK_DEF(ovr_LeaderboardUpdateStatus_GetDidUpdate, bool, const ovrLeaderboardUpdateStatusHandle obj)
{
    bool ret = ovr_LeaderboardUpdateStatus_GetDidUpdate_def(obj);
    log("ovr_LeaderboardUpdateStatus_GetDidUpdate_def called and got result %d", ret);
    return ret;
}


#pragma endregion

#pragma region probably not needed
HOOK_DEF(ovr_LanguagePackInfo_GetEnglishName, const char *, const ovrLanguagePackInfoHandle obj)
{
    auto ret = ovr_LanguagePackInfo_GetEnglishName_def(obj);
    log("ovr_LanguagePackInfo_GetEnglishName called with %d, returned %s", obj, ret);
    return ret;
}

HOOK_DEF(ovr_LanguagePackInfo_GetNativeName, const char *, const ovrLanguagePackInfoHandle obj)
{
    auto ret = ovr_LanguagePackInfo_GetNativeName_def(obj);
    log("ovr_LanguagePackInfo_GetNativeName called with %d, returned %s", obj, ret);
    return ret;    
}

HOOK_DEF(ovr_LanguagePackInfo_GetTag, const char *, const ovrLanguagePackInfoHandle obj)
{
    auto ret = ovr_LanguagePackInfo_GetTag_def(obj);
    log("ovr_LanguagePackInfo_GetTag called with %d, returned %s", obj, ret);
    return ret;
}

HOOK_DEF(ovr_Message_GetString, const char *, const ovrMessageHandle obj)
{
    log("ovr_Message_GetString called");
    return ovr_Message_GetString_def(obj);
}

HOOK_DEF(ovrMessage_AssetFile_GetList, ovrRequest)
{
    log("ovrMessage_AssetFile_GetList");
    ovrMessage_AssetFile_GetList_def();
}
HOOK_DEF(ovr_AssetDetails_GetLanguage, ovrLanguagePackInfoHandle, const ovrAssetDetailsHandle obj)
{
    log("ovr_AssetDetails_GetLanguage called with %d", obj);
    return ovr_AssetDetails_GetLanguage_def(obj);
}
#pragma endregion

#pragma region tracking
//ovrTracking2 (*vrapi_GetPredictedTracking2)(ovrMobile * ovr, double absTimeInSeconds );
//ovrResult (*vrapi_EnumerateInputDevices_def)( ovrMobile * ovr, const uint32_t index, ovrInputCapabilityHeader * capsHeader ) = NULL;
//ovrResult (*vrapi_GetCurrentInputState_def)( ovrMobile * ovr, const ovrDeviceID deviceID, ovrInputStateHeader * inputState ) = NULL;
//ovrTracking2 (*vrapi_GetPredictedTracking2_def)( ovrMobile * ovr, double absTimeInSeconds ) = NULL;
//ovrTracking (*vrapi_GetPredictedTracking_def)( ovrMobile * ovr, double absTimeInSeconds ) = NULL;
//ovrMobile* (*vrapi_EnterVrMode_def)(void* modeParams) = NULL;
//ovrResult (*vrapi_GetInputTrackingState_def)( ovrMobile * ovr, const ovrDeviceID deviceID, const double absTimeInSeconds, ovrTracking * tracking ) = NULL;
ovrMobile *ovm = NULL;

void logVec3(const char* name, ovrVector3f vec)
{
    log("\t%s :", name);
    log("\t\tx: %f", vec.x);
    log("\t\ty: %f", vec.y);
    log("\t\tz: %f", vec.z);
}
void logQuat(const char* name, ovrQuatf vec)
{
    log("\t%s:", name);
    log("\t\tx: %f", vec.x);
    log("\t\ty: %f", vec.y);
    log("\t\tz: %f", vec.z);
    log("\t\tw: %f", vec.w);
}
void logVec2(const char* name, ovrVector2f vec)
{
    log("\t%s:", name);
    log("\t\tx: %f", vec.x);
    log("\t\ty: %f", vec.y);
}
void logPose(ovrRigidBodyPosef pose)
{
    logVec3("Angular Acceleration", pose.AngularAcceleration);
    logVec3("Angular Velocity", pose.AngularVelocity);
    logVec3("Linear Acceleration", pose.LinearAcceleration);
    logVec3("Linear Velocity", pose.LinearVelocity);
    logVec3("Pose Position", pose.Pose.Position);
    logQuat("Pose Orientation", pose.Pose.Orientation);
    log("\tPrediction Sec: %f", pose.PredictionInSeconds);
    log("\tTime Sec: %f", pose.TimeInSeconds);
}
void logButtons(uint32_t buttons)
{
    log("\tButtons:");
    log("\t\tA:\t%s", (buttons & ovrButton_A)?"PRESSED":"");
    log("\t\tB:\t%s", (buttons & ovrButton_B)?"PRESSED":"");
    log("\t\tBack:\t%s", (buttons & ovrButton_Back)?"PRESSED":"");
    log("\t\tDown:\t%s", (buttons & ovrButton_Down)?"PRESSED":"");
    log("\t\tEnter:\t%s", (buttons & ovrButton_Enter)?"PRESSED":"");
    log("\t\tGripTrigger:\t%s", (buttons & ovrButton_GripTrigger)?"PRESSED":"");
    log("\t\tJoystick:\t%s", (buttons & ovrButton_Joystick)?"PRESSED":"");
    log("\t\tLeft:\t%s", (buttons & ovrButton_Left)?"PRESSED":"");
    log("\t\tLShoulder:\t%s", (buttons & ovrButton_LShoulder)?"PRESSED":"");
    log("\t\tLThumb:\t%s", (buttons & ovrButton_LThumb)?"PRESSED":"");
    log("\t\tRight:\t%s", (buttons & ovrButton_Right)?"PRESSED":"");
    log("\t\tRShoulder:\t%s", (buttons & ovrButton_RShoulder)?"PRESSED":"");
    log("\t\tRThumb:\t%s", (buttons & ovrButton_RThumb)?"PRESSED":"");
    log("\t\tTrigger:\t%s", (buttons & ovrButton_Trigger)?"PRESSED":"");
    log("\t\tUp:\t%s", (buttons & ovrButton_Up)?"PRESSED":"");
    log("\t\tX:\t%s", (buttons & ovrButton_X)?"PRESSED":"");
    log("\t\tY:\t%s", (buttons & ovrButton_Y)?"PRESSED":"");
}

HOOK_DEF(vrapi_EnterVrMode, ovrMobile*, void* modeParams)
{
    log("EnterVrMode called!");
    ovm = vrapi_EnterVrMode_def(modeParams);
    log("pointer to ovrMobile: %d", ovm);
    return ovm;
}
    



    
    
    
/*ovrMobile * hook_vrapi_EnterVrMode(void* modeParams)
{
    log("EnterVrMode called!");
    ovm = vrapi_EnterVrMode_def(modeParams);
    log("pointer to ovrMobile: %d", ovm);
    //setupThread();
    return ovm;
}*/
HOOK_DEF(vrapi_GetInputTrackingState, ovrResult, ovrMobile * ovr, const ovrDeviceID deviceID, const double absTimeInSeconds, ovrTracking * tracking)
{
    ovrResult res = vrapi_GetInputTrackingState_def(ovr, deviceID, absTimeInSeconds, tracking);
    if (res != 0)
    {
        err("Result of call was nonzero: %d", res);
        return res;
    }
    if (LOGLOTS) {
        log("GetInputTrackingState called for device id %d and absTime %f", deviceID, absTimeInSeconds);
        log("Tracking Status: %d", tracking->Status);
        logPose(tracking->HeadPose);
    }       
    
    return res;
}

HOOK_DEF(vrapi_EnumerateInputDevices, ovrResult, ovrMobile * ovr, const uint32_t index, ovrInputCapabilityHeader * capsHeader )
{
    if (LOGLOTS) {
        log("EnumerateInputDevices called");
    }
    return vrapi_EnumerateInputDevices_def(ovr, index, capsHeader);
}
HOOK_DEF(vrapi_GetCurrentInputState, ovrResult, ovrMobile * ovr, const ovrDeviceID deviceID, ovrInputStateHeader * inputState )
{
    
    ovrResult res = vrapi_GetCurrentInputState_def(ovr, deviceID, inputState);
    if (res != 0)
    {
        err("Result of call was nonzero: %d", res);
        return res;
    }
    if (LOGLOTS) {
        log("GetCurrentInputState called for device id %d", deviceID);
        log("Controller type: %d", inputState->ControllerType);
        if (inputState->ControllerType == ovrControllerType::ovrControllerType_TrackedRemote)
        {
            log("--Tracked Remote--");
            ovrInputStateTrackedRemote *trackedRemote = (ovrInputStateTrackedRemote*)inputState;
            logButtons(trackedRemote->Buttons);
            logVec2("Joystick", trackedRemote->Joystick);
            logVec2("Trackpad Position", trackedRemote->TrackpadPosition);
            log("Trackpad Status: ", trackedRemote->TrackpadStatus);
            log("Grip Trigger: %f", trackedRemote->GripTrigger);
            log("Index Trigger: %f", trackedRemote->IndexTrigger);
            log("Touches: %d", trackedRemote->Touches);
            log("Battery Percent: %d", trackedRemote->BatteryPercentRemaining);
        }
        else if (inputState->ControllerType == ovrControllerType::ovrControllerType_Headset)
        {
            log("--Headset--");
            ovrInputStateHeadset *headset = (ovrInputStateHeadset*)inputState;
            log("buttons: %d", headset->Buttons);
            logVec2("Trackpad Position", headset->TrackpadPosition);
            log("Trackpad Status: ", headset->TrackpadStatus);
        }
    }
    return res;
}
HOOK_DEF(vrapi_GetPredictedTracking2, ovrTracking2, ovrMobile * ovr, double absTimeInSeconds )
{
    if (LOGLOTS) {
        log("GetPredictedTracking2 called");
    }
    return vrapi_GetPredictedTracking2_def(ovr, absTimeInSeconds);
}
HOOK_DEF(vrapi_GetPredictedTracking, ovrTracking, ovrMobile * ovr, double absTimeInSeconds )
{
    if (LOGLOTS) {
        log("GetPredictedTracking called");
    }
    return vrapi_GetPredictedTracking_def(ovr, absTimeInSeconds);
}

#pragma endregion

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

    void * ovrplatformHandle = dlopen("libovrplatformloader.so", RTLD_NOW);


    INIT_HOOK(vrapiHandle, vrapi_EnterVrMode);
    INIT_HOOK(vrapiHandle, vrapi_GetInputTrackingState);
    INIT_HOOK(vrapiHandle, vrapi_EnumerateInputDevices);
    INIT_HOOK(vrapiHandle, vrapi_GetCurrentInputState);
    INIT_HOOK(vrapiHandle, vrapi_GetPredictedTracking2);
    INIT_HOOK(vrapiHandle, vrapi_GetPredictedTracking);

    INIT_HOOK(ovrplatformHandle, ovr_Leaderboard_GetEntries);
    INIT_HOOK(ovrplatformHandle, ovr_Leaderboard_GetEntriesAfterRank);
    INIT_HOOK(ovrplatformHandle, ovr_Leaderboard_GetNextEntries);
    INIT_HOOK(ovrplatformHandle, ovr_Leaderboard_GetPreviousEntries);
    INIT_HOOK(ovrplatformHandle, ovr_Leaderboard_WriteEntry);
    INIT_HOOK(ovrplatformHandle, ovrMessageType_ToString);
    INIT_HOOK(ovrplatformHandle, ovr_Message_GetType);
    INIT_HOOK(ovrplatformHandle, ovr_PopMessage);
    INIT_HOOK(ovrplatformHandle, ovr_Message_GetLeaderboardEntryArray);
    INIT_HOOK(ovrplatformHandle, ovr_Message_GetLeaderboardUpdateStatus);
    INIT_HOOK(ovrplatformHandle, ovr_LeaderboardUpdateStatus_GetDidUpdate);
    INIT_HOOK(ovrplatformHandle, ovr_LeaderboardEntryArray_GetElement);
    INIT_HOOK(ovrplatformHandle, ovr_LeaderboardEntryArray_GetNextUrl);
    INIT_HOOK(ovrplatformHandle, ovr_LeaderboardEntryArray_GetPreviousUrl);
    INIT_HOOK(ovrplatformHandle, ovr_LeaderboardEntryArray_GetSize);
    INIT_HOOK(ovrplatformHandle, ovr_LeaderboardEntryArray_GetTotalCount);
    INIT_HOOK(ovrplatformHandle, ovr_LeaderboardEntryArray_HasNextPage);
    INIT_HOOK(ovrplatformHandle, ovr_LeaderboardEntryArray_HasPreviousPage);
    INIT_HOOK(ovrplatformHandle, ovr_LeaderboardEntry_GetExtraData);
    INIT_HOOK(ovrplatformHandle, ovr_LeaderboardEntry_GetExtraDataLength);
    INIT_HOOK(ovrplatformHandle, ovr_LeaderboardEntry_GetRank);
    INIT_HOOK(ovrplatformHandle, ovr_LeaderboardEntry_GetScore);
    INIT_HOOK(ovrplatformHandle, ovr_LeaderboardEntry_GetTimestamp);
    INIT_HOOK(ovrplatformHandle, ovr_LeaderboardEntry_GetUser);
    INIT_HOOK(ovrplatformHandle, ovr_User_GetOculusID);
    INIT_HOOK(ovrplatformHandle, ovr_Message_IsError);
    INIT_HOOK(ovrplatformHandle, ovr_FreeMessage);
    INIT_HOOK(ovrplatformHandle, ovr_Message_GetError);
    INIT_HOOK(ovrplatformHandle, ovr_Message_GetString);
    INIT_HOOK(ovrplatformHandle, ovr_LanguagePackInfo_GetEnglishName);
    INIT_HOOK(ovrplatformHandle, ovr_LanguagePackInfo_GetNativeName);
    INIT_HOOK(ovrplatformHandle, ovr_LanguagePackInfo_GetTag);
    INIT_HOOK(ovrplatformHandle, ovr_AssetDetails_GetLanguage);
    INIT_HOOK(ovrplatformHandle, ovr_Message_GetNativeMessage);
    INIT_HOOK(ovrplatformHandle, ovr_Message_GetRequestID);
    INIT_HOOK(ovrplatformHandle, ovr_User_GetPresence);
    INIT_HOOK(ovrplatformHandle, ovr_User_GetPresenceStatus);
    INIT_HOOK(ovrplatformHandle, ovr_User_GetID);
    INIT_HOOK(ovrplatformHandle, ovr_User_GetImageUrl);
    INIT_HOOK(ovrplatformHandle, ovr_User_GetInviteToken);
    INIT_HOOK(ovrplatformHandle, ovr_User_GetSmallImageUrl);

    if (!addr_vrapi_GetInputTrackingState)
    {
        err("Did not find vrapi_GetInputTrackingState!");
        return;
    }
    if (!addr_vrapi_EnumerateInputDevices)
    {
        err("Did not find vrapi_EnumerateInputDevices!");
        return;
    }
    if (!addr_vrapi_GetCurrentInputState)
    {
        err("Did not find vrapi_GetCurrentInputState!");
        return;
    }
    if (!addr_vrapi_GetPredictedTracking2)
    {
        err("Did not find vrapi_GetPredictedTracking2!");
        return;
    }
    if (!addr_vrapi_GetPredictedTracking)
    {
        err("Did not find vrapi_GetPredictedTracking!");
        return;
    }
    
    registerInlineHook((uint32_t)addr_vrapi_GetInputTrackingState, (uint32_t)hook_vrapi_GetInputTrackingState, (uint32_t **)&vrapi_GetInputTrackingState_def);
    inlineHook((uint32_t)addr_vrapi_GetInputTrackingState);

    registerInlineHook((uint32_t)addr_vrapi_EnumerateInputDevices, (uint32_t)hook_vrapi_EnumerateInputDevices, (uint32_t **)&vrapi_EnumerateInputDevices_def);
    inlineHook((uint32_t)addr_vrapi_EnterVrMode);

    registerInlineHook((uint32_t)addr_vrapi_GetCurrentInputState, (uint32_t)hook_vrapi_GetCurrentInputState, (uint32_t **)&vrapi_GetCurrentInputState_def);
    inlineHook((uint32_t)addr_vrapi_GetCurrentInputState);

    registerInlineHook((uint32_t)addr_vrapi_GetPredictedTracking2, (uint32_t)hook_vrapi_GetPredictedTracking2, (uint32_t **)&vrapi_GetPredictedTracking2_def);
    inlineHook((uint32_t)addr_vrapi_GetPredictedTracking2);

   // registerInlineHook((uint32_t)addr_vrapi_EnterVrMode, (uint32_t)hook_vrapi_EnterVrMode, (uint32_t **)&vrapi_EnterVrMode_def);
   // inlineHook((uint32_t)addr_vrapi_EnterVrMode);

    registerInlineHook((uint32_t)addr_vrapi_GetPredictedTracking, (uint32_t)hook_vrapi_GetPredictedTracking, (uint32_t **)&vrapi_GetPredictedTracking_def);
    inlineHook((uint32_t)addr_vrapi_GetPredictedTracking);

}