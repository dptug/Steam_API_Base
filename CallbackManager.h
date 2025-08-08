#ifndef CALLBACKMANAGER_H
#define CALLBACKMANAGER_H

#pragma once

#include <map>

typedef bool (S_CALLTYPE *_Steam_BGetCallback)(HSteamPipe hSteamPipe, CallbackMsg_t *pCallbackMsg);
typedef void (S_CALLTYPE *_Steam_FreeLastCallback)(HSteamPipe hSteamPipe);
typedef bool (S_CALLTYPE *_Steam_GetAPICallResult)(HSteamPipe hSteamPipe, SteamAPICall_t hSteamAPICall, void* pCallback, int cubCallback, int iCallbackExpected, bool* pbFailed);

struct CallbackInfo
{
	class CCallbackBase* pCallback;
	int callbackId;

	CallbackInfo() : pCallback(nullptr), callbackId(0) {}
	CallbackInfo(class CCallbackBase* callback, int id) : pCallback(callback), callbackId(id) {}
};

class CCallbackMgr
{
private:
	// Используем указатель на колбэк как ключ вместо int
	std::map<class CCallbackBase*, CallbackInfo> m_Callbacks;
public:
	_Steam_BGetCallback oSteam_BGetCallback;
	_Steam_FreeLastCallback oSteam_FreeLastCallback;
	_Steam_GetAPICallResult oSteam_GetAPICallResult;
	HSteamUser HSteamUserCurrent;
	int ManualDispatchCallback;
	int ManualDispatchCallbackSize;
	bool bIsRunningCallbacks;
	std::multimap<int, CCallbackBase*> MapCallbacks;
	std::map<SteamAPICall_t, CCallbackBase*> MapAPICalls;
	std::map<SteamAPICall_t, BYTE*> MapAPIBuffer;
public:
	CCallbackMgr();
	~CCallbackMgr();

	void Unload();
	void RunCallbacks(HSteamPipe hSteamPipe, bool bGameServerCallbacks);
	void RunCallbacksTryCatch(HSteamPipe hSteamPipe, bool bGameServerCallbacks);
	void RunCallResult(HSteamPipe hSteamPipe, SteamAPICall_t APICall, CCallbackBase* pCallback);
	void RegisterCallback(CCallbackBase *pCallback, int iCallback);
	void RegisterCallResult(CCallbackBase *pCallback, SteamAPICall_t APICall);
	void UnregisterCallback(CCallbackBase* pCallback);
	void UnregisterCallResult(CCallbackBase *pCallback, SteamAPICall_t APICall);
};

#endif // CALLBACKMANAGER_H