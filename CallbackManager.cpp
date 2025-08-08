#include <Windows.h>
#include <conio.h>
#include "Steam\steam_api.h"
#include "Steam\steamclientpublic.h"
#include "CallbackManager.h"
#include <vector>

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
	printf("[Steam_API_Base] CCallbackMgr::Destructor\r\n");

	// Безопасная очистка карты колбэков
	auto it = m_Callbacks.begin();
	while (it != m_Callbacks.end())
	{
		printf("[Steam_API_Base] Cleaning up callback %d\r\n", it->second.callbackId);
		it = m_Callbacks.erase(it);
	}

	printf("[Steam_API_Base] All callbacks cleaned up\r\n");
}

bool CCallbackMgr::HasCallback(class CCallbackBase* pCallback)
{
	if (!pCallback)
		return false;

	return m_Callbacks.find(pCallback) != m_Callbacks.end();
}

void CCallbackMgr::Clear()
{
	printf("[Steam_API_Base] CCallbackMgr::Clear\r\n");

	auto it = m_Callbacks.begin();
	while (it != m_Callbacks.end())
	{
		it = m_Callbacks.erase(it);
	}

	m_Callbacks.clear();
	printf("[Steam_API_Base] All callbacks cleared\r\n");
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

	// === ДОБАВИТЬ ОЧИСТКУ ПАМЯТИ ===
	for (auto& pair : this->MapAPIBuffer)
	{
		delete[] pair.second;  // Освобождаем каждый выделенный буфер
	}
	this->MapAPIBuffer.clear();

	this->MapCallbacks.clear();
	this->MapAPICalls.clear();
}

// Уберите удаление из MapAPICalls из метода RunCallResult:
void CCallbackMgr::RunCallResult(HSteamPipe hSteamPipe, SteamAPICall_t APICall, CCallbackBase* pCallback)
{
	_cprintf_s("[Steam_API_Base] Running CallResult --> %lld --> Callback --> %d --> Size --> %d\r\n",
		APICall, pCallback->GetICallback(), pCallback->GetCallbackSizeBytes());

	BYTE* CallResultBuffer = new BYTE[pCallback->GetCallbackSizeBytes()]();
	bool pbFailed = false;

	bool bAPICall = this->oSteam_GetAPICallResult(hSteamPipe, APICall, CallResultBuffer,
		pCallback->GetCallbackSizeBytes(), pCallback->GetICallback(), &pbFailed);

	if (bAPICall == true && pbFailed == false)
	{
		size_t OriginalCallbackMapSize = this->MapCallbacks.size();
		pCallback->Run(CallResultBuffer, pbFailed, APICall);

		// Left 4 Dead 2 lobby hack
		if (OriginalCallbackMapSize != this->MapCallbacks.size())
		{
			WriteColoredText(FOREGROUND_BLUE | FOREGROUND_INTENSITY, 7,
				"[Steam_API_Base] Callback(s) was added or removed while CallResult was running!\r\n");
			this->MapCallbacks.erase(LobbyEnter_t::k_iCallback);
			pCallback->Run(CallResultBuffer);
		}

		this->MapAPIBuffer.insert(std::make_pair(APICall, CallResultBuffer));
		// УБРАЛИ: this->MapAPICalls.erase(APICall);
	}
	else
	{
		WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7,
			"[Steam_API_Base] Steam_GetAPICallResult Failed!\r\n");
		delete[] CallResultBuffer;
		// УБРАЛИ: this->MapAPICalls.erase(APICall);
	}
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
					// Собираем идентификаторы для удаления
					std::vector<SteamAPICall_t> callsToRemove;

					// Безопасно итерируемся, собираем что нужно удалить
					for (auto& pair : this->MapAPICalls)
					{
						if (SteamAPICallCompleted->m_hAsyncCall == pair.first &&
							SteamAPICallCompleted->m_iCallback == pair.second->GetICallback() &&
							SteamAPICallCompleted->m_cubParam == (uint32)pair.second->GetCallbackSizeBytes())
						{
							this->RunCallResult(hSteamPipe, pair.first, pair.second);
							callsToRemove.push_back(pair.first);
						}
					}

					// Удаляем уже после итерации
					for (SteamAPICall_t callId : callsToRemove)
					{
						this->MapAPICalls.erase(callId);
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
						// Собираем идентификаторы для удаления
						std::vector<SteamAPICall_t> callsToRemove;

						// Безопасно итерируемся, собираем что нужно удалить
						for (auto& pair : this->MapAPICalls)
						{
							if (SteamAPICallCompleted->m_hAsyncCall == pair.first &&
								SteamAPICallCompleted->m_iCallback == pair.second->GetICallback() &&
								SteamAPICallCompleted->m_cubParam == (uint32)pair.second->GetCallbackSizeBytes())
							{
								this->RunCallResult(hSteamPipe, pair.first, pair.second);
								callsToRemove.push_back(pair.first);
							}
						}

						// Удаляем уже после итерации
						for (SteamAPICall_t callId : callsToRemove)
						{
							this->MapAPICalls.erase(callId);
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

void CCallbackMgr::UnregisterCallback(class CCallbackBase* pCallback)
{
	printf("[Steam_API_Base] CCallbackMgr::UnregisterCallback\r\n");

	if (!pCallback)
	{
		printf("[Steam_API_Base] Null callback pointer\r\n");
		return;
	}

	// Безопасная работа с итераторами
	for (auto it = m_Callbacks.begin(); it != m_Callbacks.end(); ++it)
	{
		if (it->second.pCallback == pCallback)
		{
			printf("[Steam_API_Base] Found callback to unregister\r\n");
			m_Callbacks.erase(it);
			printf("[Steam_API_Base] Callback unregistered successfully\r\n");
			return;
		}
	}

	printf("[Steam_API_Base] Callback not found for unregister\r\n");
}

void CCallbackMgr::RegisterCallback(class CCallbackBase* pCallback, int iCallback)
{
	printf("[Steam_API_Base] CCallbackMgr::RegisterCallback --> %d\r\n", iCallback);

	if (!pCallback)
	{
		printf("[Steam_API_Base] Null callback pointer\r\n");
		return;
	}

	// Проверяем, не зарегистрирован ли уже этот колбэк
	for (auto it = m_Callbacks.begin(); it != m_Callbacks.end(); ++it)
	{
		if (it->second.pCallback == pCallback)
		{
			printf("[Steam_API_Base] Callback already registered, updating\r\n");
			it->second.callbackId = iCallback;
			return;
		}
	}

	// Добавляем новый колбэк
	CallbackInfo info;
	info.pCallback = pCallback;
	info.callbackId = iCallback;
	m_Callbacks[pCallback] = info;

	printf("[Steam_API_Base] Callback registered successfully\r\n");
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