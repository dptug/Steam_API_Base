#include <Windows.h>
#include <conio.h>
#include "Steam\steam_api.h"
#include "Steam\steamclientpublic.h"
#include "CallbackManager.h"

void WriteColoredText(WORD Color, WORD OriginalColor, const char* Text);
bool bCallbackManagerInitialized = false;
extern HSteamUser hUser;
extern HSteamUser hUserServer;

CCallbackMgr::CCallbackMgr()
{
	WriteColoredText(FOREGROUND_BLUE | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] CCallbackMgr::Constructor\r\n");

	this->oSteam_BGetCallback = nullptr;
	this->oSteam_FreeLastCallback = nullptr;
	this->oSteam_GetAPICallResult = nullptr;
	this->HSteamUserCurrent = 0;
	this->ManualDispatchCallback = 0;
	this->ManualDispatchCallbackSize = 0;
	this->bIsRunningCallbacks = false;
	this->MapCallbacks.clear();
	this->MapAPICalls.clear();
	bCallbackManagerInitialized = true;
}

CCallbackMgr::~CCallbackMgr()
{
	WriteColoredText(FOREGROUND_BLUE | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] CCallbackMgr::Destructor\r\n");

	bCallbackManagerInitialized = false;
	this->oSteam_BGetCallback = nullptr;
	this->oSteam_FreeLastCallback = nullptr;
	this->oSteam_GetAPICallResult = nullptr;
	this->HSteamUserCurrent = 0;
	this->ManualDispatchCallback = 0;
	this->ManualDispatchCallbackSize = 0;
	this->bIsRunningCallbacks = false;
	this->MapCallbacks.clear();
	this->MapAPICalls.clear();
}

void CCallbackMgr::Unload()
{
	WriteColoredText(FOREGROUND_BLUE | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] CCallbackMgr::Unload\r\n");

	bCallbackManagerInitialized = false;
	this->oSteam_BGetCallback = nullptr;
	this->oSteam_FreeLastCallback = nullptr;
	this->oSteam_GetAPICallResult = nullptr;
	this->HSteamUserCurrent = 0;
	this->ManualDispatchCallback = 0;
	this->ManualDispatchCallbackSize = 0;
	this->bIsRunningCallbacks = false;
	this->MapCallbacks.clear();
	this->MapAPICalls.clear();
}

void CCallbackMgr::RunCallResult(HSteamPipe hSteamPipe, SteamAPICall_t APICall, CCallbackBase* pCallback)
{
	_cprintf_s("[Steam_API_Base] Running CallResult --> %lld --> Callback --> %d --> Size --> %d\r\n", APICall, pCallback->GetICallback(), pCallback->GetCallbackSizeBytes());

	BYTE* CallResultBuffer = new BYTE[pCallback->GetCallbackSizeBytes()]();

	bool pbFailed = false;

	// If hSteamAPICall is not found, IsAPICallCompleted is called and the result is stored in pbFailed. GetAPICallResult returns false and no data is copied to the buffer.
	// If iCallbackExpected is incorrect, both GetAPICallResult and pbFailed will return true and no data is copied to the buffer.
	// If the expected size is the same size as cubCallback the data is copied to the buffer with that size.
	// If the expected size is larger than cubCallback the data is copied to the buffer with cubCallback.
	// If the expected size is less than cubCallback but >= 1024 bytes the data is copied to the buffer with the expected size.
	// If the expected size is less than cubCallback but <= 1023 bytes the data is padded with zeros to cubCallback (up to 1024 bytes) and copied to the buffer with that size.
	bool bAPICall = this->oSteam_GetAPICallResult(hSteamPipe, APICall, CallResultBuffer, pCallback->GetCallbackSizeBytes(), pCallback->GetICallback(), &pbFailed);

	// Callback data was only copied to the buffer if pbFailed is false
	if (bAPICall == true && pbFailed == false)
	{
		size_t OriginalCallbackMapSize = this->MapCallbacks.size();

		pCallback->Run(CallResultBuffer, pbFailed, APICall);

		//Left 4 Dead 2 lobby hack
		if (OriginalCallbackMapSize != this->MapCallbacks.size())
		{
			WriteColoredText(FOREGROUND_BLUE | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] Callback(s) was added or removed while CallResult was running!\r\n");

			this->MapCallbacks.erase(LobbyEnter_t::k_iCallback);
			pCallback->Run(CallResultBuffer);
		}

		this->MapAPIBuffer.insert(std::make_pair(APICall, CallResultBuffer));
	}
	else
	{
		WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] Steam_GetAPICallResult Failed!\r\n");

		delete[]CallResultBuffer;
	}

	return;
}

void  CCallbackMgr::RunCallbacks(HSteamPipe hSteamPipe, bool bGameServerCallbacks)
{
	if (this->oSteam_BGetCallback != nullptr && this->oSteam_FreeLastCallback != nullptr && this->oSteam_GetAPICallResult != nullptr)
	{
		CallbackMsg_t CallbackMsg = { 0 };
		if (this->oSteam_BGetCallback(hSteamPipe, &CallbackMsg))
		{
			_cprintf_s("[Steam_API_Base] Callback --> %d\r\n", CallbackMsg.m_iCallback);

			this->HSteamUserCurrent = CallbackMsg.m_hSteamUser;

			if (CallbackMsg.m_iCallback == SteamAPICallCompleted_t::k_iCallback && CallbackMsg.m_cubParam == sizeof(SteamAPICallCompleted_t))
			{
				SteamAPICallCompleted_t* SteamAPICallCompleted = (SteamAPICallCompleted_t*)CallbackMsg.m_pubParam;

				if (this->MapAPICalls.size() != 0)
				{
					std::map<SteamAPICall_t, CCallbackBase*>::iterator mapCallResultIterator;

					for (mapCallResultIterator = this->MapAPICalls.begin(); mapCallResultIterator != this->MapAPICalls.end(); mapCallResultIterator++)
					{
						if (SteamAPICallCompleted->m_hAsyncCall == mapCallResultIterator->first &&
							SteamAPICallCompleted->m_iCallback == mapCallResultIterator->second->GetICallback() &&
							SteamAPICallCompleted->m_cubParam == (uint32)mapCallResultIterator->second->GetCallbackSizeBytes())
						{
							this->RunCallResult(hSteamPipe, mapCallResultIterator->first, mapCallResultIterator->second);
						}
					}
				}
			}
			else
			{
				if (this->MapCallbacks.size() != 0)
				{
					std::multimap<int, class CCallbackBase*>::iterator mapCallbacksIterator;

					for (mapCallbacksIterator = this->MapCallbacks.begin(); mapCallbacksIterator != this->MapCallbacks.end(); mapCallbacksIterator++)
					{
						CCallbackBase* pCallback = mapCallbacksIterator->second;

						if (mapCallbacksIterator->first == CallbackMsg.m_iCallback && (pCallback->m_nCallbackFlags & pCallback->k_ECallbackFlagsRegistered))
						{
							if (CallbackMsg.m_hSteamUser == hUserServer && (pCallback->m_nCallbackFlags & pCallback->k_ECallbackFlagsGameServer) && bGameServerCallbacks == true)
							{
								_cprintf_s("[Steam_API_Base] Running GameServer Callback --> %d --> Flag(s) --> %d\r\n", CallbackMsg.m_iCallback, pCallback->m_nCallbackFlags);

								pCallback->Run(CallbackMsg.m_pubParam);
								break;
							}
							else if (CallbackMsg.m_hSteamUser == hUser && (pCallback->m_nCallbackFlags & pCallback->k_ECallbackFlagsGameServer) == 0 && bGameServerCallbacks == false)
							{
								_cprintf_s("[Steam_API_Base] Running User Callback --> %d --> Flag(s) --> %d\r\n", CallbackMsg.m_iCallback, pCallback->m_nCallbackFlags);

								//We will not run invalid HTML_NeedsPaint_t callbacks (TF2 Hack)
								bool SkipPaintCallback = false;

								if (CallbackMsg.m_iCallback == HTML_NeedsPaint_t::k_iCallback && CallbackMsg.m_cubParam == sizeof(HTML_NeedsPaint_t))
								{
									HTML_NeedsPaint_t* HTML_NeedsPaint = (HTML_NeedsPaint_t*)CallbackMsg.m_pubParam;

									if (HTML_NeedsPaint->unWide == 1 || HTML_NeedsPaint->unTall == 1)
										SkipPaintCallback = true;
								}

								if (SkipPaintCallback == false)
									pCallback->Run(CallbackMsg.m_pubParam);

								break;
							}
						}
					}
				}
			}
			_cprintf_s("[Steam_API_Base] Freeing Callback --> %d\r\n", CallbackMsg.m_iCallback);

			SecureZeroMemory(CallbackMsg.m_pubParam, CallbackMsg.m_cubParam);
			this->oSteam_FreeLastCallback(hSteamPipe);
		}
	}

	return;
}

void CCallbackMgr::RunCallbacksTryCatch(HSteamPipe hSteamPipe, bool bGameServerCallbacks)
{
	try
	{
		if (this->oSteam_BGetCallback != nullptr && this->oSteam_FreeLastCallback != nullptr && this->oSteam_GetAPICallResult != nullptr)
		{
			CallbackMsg_t CallbackMsg = { 0 };
			if (this->oSteam_BGetCallback(hSteamPipe, &CallbackMsg))
			{
				_cprintf_s("[Steam_API_Base] Callback (TryCatch) --> %d\r\n", CallbackMsg.m_iCallback);

				this->HSteamUserCurrent = CallbackMsg.m_hSteamUser;

				if (CallbackMsg.m_iCallback == SteamAPICallCompleted_t::k_iCallback && CallbackMsg.m_cubParam == sizeof(SteamAPICallCompleted_t))
				{
					SteamAPICallCompleted_t* SteamAPICallCompleted = (SteamAPICallCompleted_t*)CallbackMsg.m_pubParam;

					if (this->MapAPICalls.size() != 0)
					{
						std::map<SteamAPICall_t, CCallbackBase*>::iterator mapCallResultIterator;

						for (mapCallResultIterator = this->MapAPICalls.begin(); mapCallResultIterator != this->MapAPICalls.end(); mapCallResultIterator++)
						{
							if (SteamAPICallCompleted->m_hAsyncCall == mapCallResultIterator->first &&
								SteamAPICallCompleted->m_iCallback == mapCallResultIterator->second->GetICallback() &&
								SteamAPICallCompleted->m_cubParam == (uint32)mapCallResultIterator->second->GetCallbackSizeBytes())
							{
								this->RunCallResult(hSteamPipe, mapCallResultIterator->first, mapCallResultIterator->second);
							}
						}
					}
				}
				else
				{
					if (this->MapCallbacks.size() != 0)
					{
						std::multimap<int, class CCallbackBase*>::iterator mapCallbacksIterator;

						for (mapCallbacksIterator = this->MapCallbacks.begin(); mapCallbacksIterator != this->MapCallbacks.end(); mapCallbacksIterator++)
						{
							CCallbackBase* pCallback = mapCallbacksIterator->second;

							if (mapCallbacksIterator->first == CallbackMsg.m_iCallback && (pCallback->m_nCallbackFlags & pCallback->k_ECallbackFlagsRegistered))
							{
								if (CallbackMsg.m_hSteamUser == hUserServer && (pCallback->m_nCallbackFlags & pCallback->k_ECallbackFlagsGameServer) && bGameServerCallbacks == true)
								{
									_cprintf_s("[Steam_API_Base] Running GameServer Callback (TryCatch) --> %d --> Flag(s) --> %d\r\n", CallbackMsg.m_iCallback, pCallback->m_nCallbackFlags);

									pCallback->Run(CallbackMsg.m_pubParam);
									break;
								}
								else if (CallbackMsg.m_hSteamUser == hUser && (pCallback->m_nCallbackFlags & pCallback->k_ECallbackFlagsGameServer) == 0 && bGameServerCallbacks == false)
								{
									_cprintf_s("[Steam_API_Base] Running User Callback (TryCatch) --> %d --> Flag(s) --> %d\r\n", CallbackMsg.m_iCallback, pCallback->m_nCallbackFlags);

									pCallback->Run(CallbackMsg.m_pubParam);
									break;
								}
							}
						}
					}
				}
				_cprintf_s("[Steam_API_Base] Freeing Callback (TryCatch) --> %d\r\n", CallbackMsg.m_iCallback);

				SecureZeroMemory(CallbackMsg.m_pubParam, CallbackMsg.m_cubParam);
				this->oSteam_FreeLastCallback(hSteamPipe);
			}
		}
	}
	catch (...)
	{
		WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] Exception in callback code!\r\n");
	}

	return;
}

void CCallbackMgr::RegisterCallback(CCallbackBase *pCallback, int iCallback)
{
	if (pCallback != nullptr)
	{
		if (pCallback->GetCallbackSizeBytes() != 0)
		{
			_cprintf_s("[Steam_API_Base] CCallbackMgr::RegisterCallback --> %d --> Size --> %d --> Original flag(s) --> %d\r\n", iCallback, pCallback->GetCallbackSizeBytes(), pCallback->m_nCallbackFlags);

			pCallback->m_nCallbackFlags |= pCallback->k_ECallbackFlagsRegistered;
			pCallback->m_iCallback = iCallback;
			this->MapCallbacks.insert(std::make_pair(iCallback, pCallback));
		}
	}
	else
	{
		WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] CCallbackMgr::RegisterCallback --> pCallback is nullptr!\r\n");
	}

	return;
}

void CCallbackMgr::UnregisterCallback(CCallbackBase* pCallback)
{
	if (pCallback != nullptr)
	{
		if (pCallback->m_nCallbackFlags & pCallback->k_ECallbackFlagsRegistered)
		{
			if (this->MapCallbacks.size() != 0)
			{
				_cprintf_s("[Steam_API_Base] CCallbackMgr::UnregisterCallback --> %d --> Flag(s) --> %d\r\n", pCallback->GetICallback(), pCallback->m_nCallbackFlags);
				pCallback->m_nCallbackFlags &= ~pCallback->k_ECallbackFlagsRegistered;

				std::multimap<int, class CCallbackBase*>::iterator mapCallbacksIterator;

				for (mapCallbacksIterator = this->MapCallbacks.begin(); mapCallbacksIterator != this->MapCallbacks.end(); mapCallbacksIterator++)
				{
					if (mapCallbacksIterator->second == pCallback)
					{
						MapCallbacks.erase(mapCallbacksIterator);
					}
				}
			}
		}
		else
		{
			WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] CCallbackMgr::UnregisterCallback --> Callback is not registered!\r\n");
		}
	}
	else
	{
		WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] CCallbackMgr::UnregisterCallback --> pCallback is nullptr!\r\n");
	}

	return;
}

void CCallbackMgr::RegisterCallResult(CCallbackBase *pCallback, SteamAPICall_t APICall)
{
	if (pCallback != nullptr && APICall != k_uAPICallInvalid)
	{
		if (pCallback->GetICallback() != 0 && pCallback->GetCallbackSizeBytes() != 0)
		{
			_cprintf_s("[Steam_API_Base] CCallbackMgr::RegisterCallResult --> %lld --> Callback --> %d --> Size --> %d --> Original flag(s) --> %d\r\n", APICall, pCallback->GetICallback(), pCallback->GetCallbackSizeBytes(), pCallback->m_nCallbackFlags);

			pCallback->m_nCallbackFlags |= pCallback->k_ECallbackFlagsRegistered;

			auto it = this->MapAPICalls.find(APICall);

			if (it == this->MapAPICalls.end())
			{
				this->MapAPICalls.insert(std::make_pair(APICall, pCallback));
			}
			else
			{
				WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] CCallbackMgr::RegisterCallResult --> CallResult already exist in map!\r\n");
				it->second = pCallback;
			}
		}
		else
		{
			WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] CCallbackMgr::RegisterCallResult --> Callback is zero!\r\n");
		}
	}
	else
	{
		WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] CCallbackMgr::RegisterCallResult --> pCallback is nullptr or APICall is k_uAPICallInvalid!\r\n");
	}

	return;
}

void CCallbackMgr::UnregisterCallResult(CCallbackBase *pCallback, SteamAPICall_t APICall)
{
	if (pCallback != nullptr && APICall != k_uAPICallInvalid)
	{
		if (pCallback->m_nCallbackFlags & pCallback->k_ECallbackFlagsRegistered)
		{
			_cprintf_s("[Steam_API_Base] CCallbackMgr::UnregisterCallResult --> %lld --> Callback --> %d --> Flag(s) --> %d\r\n", APICall, pCallback->GetICallback(), pCallback->m_nCallbackFlags);

			pCallback->m_nCallbackFlags &= ~pCallback->k_ECallbackFlagsRegistered;

			auto it = this->MapAPIBuffer.find(APICall);

			if (it != this->MapAPIBuffer.end())
			{
				WriteColoredText(FOREGROUND_BLUE | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] CCallbackMgr::UnregisterCallResult --> Freeing CallResult Buffer!\r\n");
				delete[]it->second;
				MapAPIBuffer.erase(it);
			}
		}
		else
		{
			WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] CCallbackMgr::UnregisterCallResult --> CallResult is not registered!\r\n");
		}
	}
	else
	{
		WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] CCallbackMgr::UnregisterCallResult --> pCallback is nullptr or APICall is k_uAPICallInvalid!\r\n");
	}

	return;
}

CCallbackMgr* GCallbackMgr()
{
	static CCallbackMgr CallbackMgr;
	return &CallbackMgr;
}