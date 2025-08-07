#include "SteamSettings.h"
#pragma comment(lib, "Shlwapi.lib")

//#include "../FindPatternFunctions.h"
extern bool bCallbackManagerInitialized;
CCallbackMgr* GCallbackMgr();

bool bAnonymousUser = false;

uint32 CheckCallbackRegistered(int iCallbackNum)
{
	uint32 nCallbackRegistered = 0;

	if (bCallbackManagerInitialized == true)
	{
		CCallbackMgr* pCallbackManager = GCallbackMgr();

		if (pCallbackManager->MapCallbacks.size() != 0)
		{
			std::multimap<int, class CCallbackBase*>::iterator mapCallbacksIterator;

			for (mapCallbacksIterator = pCallbackManager->MapCallbacks.begin(); mapCallbacksIterator != pCallbackManager->MapCallbacks.end(); mapCallbacksIterator++)
			{
				if (mapCallbacksIterator->first == iCallbackNum)
					nCallbackRegistered++;
			}
		}
	}

	_cprintf_s("[Steam_API_Base] CheckCallbackRegistered --> %d --> %u\r\n", iCallbackNum, nCallbackRegistered);

	return nCallbackRegistered;
}

void NotifyMissingInterface(HSteamPipe hSteamPipe, const char* Interface)
{
	HMODULE hDll = hSteamclient_Client;

	if (hSteamclient_Server != nullptr)
		hDll = hSteamclient_Server;

	oNotifyMissingInterface = (_NotifyMissingInterface)GetProcAddress(hDll, "Steam_NotifyMissingInterface");

	if (oNotifyMissingInterface == nullptr)
		return;

	oNotifyMissingInterface(hSteamPipe, Interface);
}

S_API HSteamPipe S_CALLTYPE GetHSteamPipe()
{
	_cprintf_s("[Steam_API_Base] GetHSteamPipe\r\n");
	return hPipe;
}

S_API HSteamUser S_CALLTYPE GetHSteamUser()
{
	_cprintf_s("[Steam_API_Base] GetHSteamUser\r\n");
	return hUser;
}

S_API HSteamPipe S_CALLTYPE SteamAPI_GetHSteamPipe()
{
	_cprintf_s("[Steam_API_Base] SteamAPI_GetHSteamPipe\r\n");
	return hPipe;
}

S_API HSteamUser S_CALLTYPE SteamAPI_GetHSteamUser()
{
	_cprintf_s("[Steam_API_Base] SteamAPI_GetHSteamUser\r\n");
	return hUser;
}

S_API const char* S_CALLTYPE SteamAPI_GetSteamInstallPath()
{
	_cprintf_s("[Steam_API_Base] SteamAPI_GetSteamInstallPath\r\n");

	if (AlreadyHaveInstallPath == false)
	{
		DWORD ActiveProcessPID = 0;
		LSTATUS GetPID = GetRegistryDWORD("Software\\Valve\\Steam\\ActiveProcess", "pid", ActiveProcessPID);

		if (GetPID == ERROR_SUCCESS && ActiveProcessPID != 0)
		{
			HANDLE hPIDProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, ActiveProcessPID);

			if (hPIDProcess != nullptr)
			{
				DWORD ExitCode = 0;
				BOOL bExitCode = GetExitCodeProcess(hPIDProcess, &ExitCode);

				CloseHandle(hPIDProcess);

				if (bExitCode == TRUE && ExitCode == 259)
				{
					LSTATUS GetDllString = ERROR_SUCCESS;

                    #if defined(_M_IX86)
					    GetDllString = GetRegistryString("Software\\Valve\\Steam\\ActiveProcess", "SteamClientDll", szSteamInstallPath, MAX_PATH);
                    #endif
                    #if defined(_M_AMD64)
						GetDllString = GetRegistryString("Software\\Valve\\Steam\\ActiveProcess", "SteamClientDll64", szSteamInstallPath, MAX_PATH);
                    #endif

					if (GetDllString == ERROR_SUCCESS)
					{
						BOOL FixPath = PathRemoveFileSpecA(szSteamInstallPath);

						if (FixPath == TRUE)
						{
							AlreadyHaveInstallPath = true;

							_cprintf_s("[Steam_API_Base] Steam Install Path --> %s\r\n", szSteamInstallPath);

							return szSteamInstallPath;
						}
						else
						{
							WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] PathRemoveFileSpecA Failed (SteamAPI_GetSteamInstallPath)!\r\n");
						}
					}
					else
					{
						WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] Unable to get steamclient(64).dll path (SteamAPI_GetSteamInstallPath)!\r\n");
					}
				}
				else
				{
					WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] GetExitCodeProcess Failed (SteamAPI_GetSteamInstallPath)!\r\n");
				}
			}
			else
			{
				WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] OpenProcess Failed (SteamAPI_GetSteamInstallPath)!\r\n");
			}
		}
		else
		{
			WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] Unable to get the PID of the Steam process (SteamAPI_GetSteamInstallPath)!\r\n");
		}
	}
	else
	{
		return szSteamInstallPath;
	}

	return "SteamAPIBaseInvalidPath";
}

S_API ESteamAPIInitResult S_CALLTYPE SteamInternal_SteamAPI_Init(const char* pszInternalCheckInterfaceVersions, SteamErrMsg* pOutErrMsg)
{
	_cprintf_s("[Steam_API_Base] SteamAPI_Init\r\n");

	if (SteamClient_Client != nullptr)
	{
		WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] SteamAPI_Init has already been called!\r\n");
		return k_ESteamAPIInitResult_OK;
	}

	// Получаем настройки
	CSteamSettings* settings = GSteamSettings();
	const auto& config = settings->GetSteamConfig();
	const auto& miscConfig = settings->GetMiscConfig();
	const auto& wrapperConfig = settings->GetWrapperConfig();

	// Определяем активный App ID
	uint32 activeAppId = settings->GetActiveAppId();
	_cprintf_s("[Steam_API_Base] Using App ID: %u\r\n", activeAppId);

	// Устанавливаем переменные окружения
	char szAppID[32] = { 0 };
	_snprintf_s(szAppID, 32, _TRUNCATE, "%u", activeAppId);
	SetEnvironmentVariableA("SteamAppId", szAppID);

	// Устанавливаем язык игры
	std::string language = settings->GetGameLanguage();
	if (!language.empty())
	{
		SetEnvironmentVariableA("SteamLanguage", language.c_str());
		_cprintf_s("[Steam_API_Base] Language set to: %s\r\n", language.c_str());
	}

	// Устанавливаем режим офлайн
	if (config.forceOffline)
	{
		SetEnvironmentVariableA("SteamForceOffline", "1");
		_cprintf_s("[Steam_API_Base] Force offline mode enabled\r\n");
	}

	// Проверяем настройку низкой жестокости
	if (config.lowViolence)
	{
		SetEnvironmentVariableA("SteamLowViolence", "1");
		_cprintf_s("[Steam_API_Base] Low violence mode enabled\r\n");
	}

	SteamClient_Client = static_cast<ISteamClient*>(InternalAPI_Init(&hSteamclient_Client, bAnonymousUser, STEAMCLIENT_INTERFACE_VERSION));

	if (SteamClient_Client == nullptr)
	{
		WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] SteamAPI_Init --> Failed to get SteamClient interface!\r\n");
		SteamAPI_Shutdown();
		return k_ESteamAPIInitResult_FailedGeneric;
	}

	if (bAnonymousUser == false)
	{
		hPipe = SteamClient_Client->CreateSteamPipe();

		if (hPipe == 0)
		{
			WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] SteamAPI_Init --> Create pipe failed!\r\n");
			SteamAPI_Shutdown();
			return k_ESteamAPIInitResult_NoSteamClient;
		}

		hUser = SteamClient_Client->ConnectToGlobalUser(hPipe);
	}
	else
	{
		hUser = SteamClient_Client->CreateLocalUser(&hPipe, k_EAccountTypeAnonUser);
	}

	if (hUser != 0)
	{
		// Проверка интерфейсов
		if (pszInternalCheckInterfaceVersions != nullptr)
		{
			HMODULE hDll = hSteamclient_Client;

			if (hSteamclient_Server != nullptr)
				hDll = hSteamclient_Server;

			oIsKnownInterface = (_IsKnownInterface)GetProcAddress(hDll, "Steam_IsKnownInterface");

			if (oIsKnownInterface == nullptr)
			{
				SteamAPI_Shutdown();
				return k_ESteamAPIInitResult_VersionMismatch;
			}
		}

		ISteamUtils* pSteamUtils = (ISteamUtils*)SteamClient_Client->GetISteamUtils(hPipe, STEAMUTILS_INTERFACE_VERSION);

		if (pSteamUtils != nullptr)
		{
			// Переопределяем App ID если нужно
			if (config.wrapperMode)
			{
				_cprintf_s("[Steam_API_Base] Wrapper mode enabled, overriding App ID: %u -> %u\r\n",
					pSteamUtils->GetAppID(), activeAppId);
			}

			// Настройка Game ID
			char szGameID[MAX_PATH] = { 0 };
			_snprintf_s(szGameID, MAX_PATH, _TRUNCATE, "%llu", CGameID(activeAppId).ToUint64());
			SetEnvironmentVariableA("SteamGameId", szGameID);
			SetEnvironmentVariableA("SteamOverlayGameId", szGameID);

			SteamAPI_SetBreakpadAppID(activeAppId);
			Steam_RegisterInterfaceFuncs(hSteamclient_Client);
			LoadBreakpad(hSteamclient_Client);

			SteamClient_Client->Set_SteamAPI_CCheckCallbackRegisteredInProcess(CheckCallbackRegistered);

			// Загрузка GameOverlayRenderer
			if (!config.forceOffline)
			{
				HMODULE hGameOverlayRenderer = nullptr;

#if defined(_M_IX86)
				hGameOverlayRenderer = GetModuleHandleW(L"GameOverlayRenderer.dll");
#endif
#if defined(_M_AMD64)
				hGameOverlayRenderer = GetModuleHandleW(L"GameOverlayRenderer64.dll");
#endif

				if (activeAppId != 769 && hGameOverlayRenderer == nullptr)
				{
					const char* InstallPath = SteamAPI_GetSteamInstallPath();

					if (_stricmp(InstallPath, "SteamAPIBaseInvalidPath") != 0)
					{
						char szOverlayDllPath[MAX_PATH] = { 0 };

#if defined(_M_IX86)
						_snprintf_s(szOverlayDllPath, MAX_PATH, _TRUNCATE, "%s\\GameOverlayRenderer.dll", InstallPath);
#endif
#if defined(_M_AMD64)
						_snprintf_s(szOverlayDllPath, MAX_PATH, _TRUNCATE, "%s\\GameOverlayRenderer64.dll", InstallPath);
#endif

						LoadLibraryExA(szOverlayDllPath, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
					}
				}
			}

			ISteamUser* pSteamUser = (ISteamUser*)SteamClient_Client->GetISteamUser(hUser, hPipe, STEAMUSER_INTERFACE_VERSION);

			if (pSteamUser != nullptr)
			{
				// Переопределяем Steam ID если настроено
				if (miscConfig.steamId != 0)
				{
					SetMinidumpSteamID(miscConfig.steamId);
					_cprintf_s("[Steam_API_Base] Steam ID override: %llu\r\n", miscConfig.steamId);
				}
				else
				{
					SetMinidumpSteamID(pSteamUser->GetSteamID().ConvertToUint64());
				}
			}
			else
			{
				NotifyMissingInterface(hPipe, STEAMUSER_INTERFACE_VERSION);
				SteamAPI_Shutdown();
				return k_ESteamAPIInitResult_VersionMismatch;
			}

			InitClientPointers = GetInterfacePointers.Init();

			if (InitClientPointers == false)
			{
				WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] SteamAPI_Init --> GetInterfacePointers.Init failed!\r\n");
				SteamAPI_Shutdown();
				return k_ESteamAPIInitResult_VersionMismatch;
			}

			_cprintf_s("[Steam_API_Base] SteamAPI_Init completed successfully!\r\n");
			return k_ESteamAPIInitResult_OK;
		}
		else
		{
			WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] SteamAPI_Init --> Failed to get pointer to ISteamUtils!\r\n");
			NotifyMissingInterface(hPipe, STEAMUTILS_INTERFACE_VERSION);
			SteamAPI_Shutdown();
			return k_ESteamAPIInitResult_VersionMismatch;
		}
	}
	else
	{
		WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] SteamAPI_Init --> Failed to connect or create user!\r\n");
		SteamAPI_Shutdown();
		return k_ESteamAPIInitResult_NoSteamClient;
	}

	SteamAPI_Shutdown();
	return k_ESteamAPIInitResult_FailedGeneric;
}

S_API bool S_CALLTYPE SteamAPI_Init()
{
	bAnonymousUser = false;
	return SteamInternal_SteamAPI_Init(nullptr, nullptr) == k_ESteamAPIInitResult_OK;
}

S_API ESteamAPIInitResult S_CALLTYPE SteamAPI_InitFlat(SteamErrMsg* pOutErrMsg)
{
	bAnonymousUser = false;
	ESteamAPIInitResult InitResult = SteamInternal_SteamAPI_Init(nullptr, nullptr);

	return InitResult;
}

S_API bool S_CALLTYPE SteamAPI_InitAnonymousUser()
{
	bAnonymousUser = true;

	bool bInitAnon = SteamInternal_SteamAPI_Init(nullptr, nullptr) == k_ESteamAPIInitResult_OK;

	bAnonymousUser = false;

	return bInitAnon;
}

S_API bool S_CALLTYPE SteamAPI_InitSafe()
{
	_cprintf_s("[Steam_API_Base] SteamAPI_InitSafe\r\n");

	bAnonymousUser = false;

	bool Init = SteamAPI_Init();

	if (Init == true && SteamClientSafe != nullptr)
		return true;

	WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] SteamAPI_InitSafe Failed!\r\n");
	return false;
}

S_API bool S_CALLTYPE SteamAPI_IsSteamRunning()
{
	_cprintf_s("[Steam_API_Base] SteamAPI_IsSteamRunning\r\n");

	DWORD ActiveProcessPID = 0;
	LSTATUS GetPID = GetRegistryDWORD("Software\\Valve\\Steam\\ActiveProcess", "pid", ActiveProcessPID);

	if (GetPID == ERROR_SUCCESS && ActiveProcessPID != 0)
	{
		HANDLE hPIDProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, ActiveProcessPID);

		if (hPIDProcess != nullptr)
		{
			DWORD ExitCode = 0;
			BOOL bExitCode = GetExitCodeProcess(hPIDProcess, &ExitCode);

			CloseHandle(hPIDProcess);

			if (bExitCode == TRUE && ExitCode == 259)
			{
				return true;
			}
			else
			{
				WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] GetExitCodeProcess Failed (SteamAPI_IsSteamRunning)!\r\n");
			}
		}
		else
		{
			WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] OpenProcess Failed (SteamAPI_IsSteamRunning)!\r\n");
		}
	}
	else
	{
		WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] Unable to get the PID of the Steam process (SteamAPI_IsSteamRunning)!\r\n");
	}

	return false;
}

S_API void S_CALLTYPE SteamAPI_ReleaseCurrentThreadMemory()
{
	_cprintf_s("[Steam_API_Base] SteamAPI_ReleaseCurrentThreadMemory\r\n");

	if (oReleaseThreadLocalMemory != nullptr)
		oReleaseThreadLocalMemory(0);

	return;
}

S_API bool S_CALLTYPE SteamAPI_RestartAppIfNecessary(uint32 unOwnAppID)
{
	_cprintf_s("[Steam_API_Base] SteamAPI_RestartAppIfNecessary\r\n");

	if (unOwnAppID == k_uAppIdInvalid)
		return false;

	char Buffer[32] = { 0 };
	DWORD AppIDVariable = GetEnvironmentVariableA("SteamAppId", Buffer, 32);

	if (AppIDVariable == 0)
	{
		WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] SteamAPI_RestartAppIfNecessary --> AppID doesn't exist!\r\n");
		SecureZeroMemory(Buffer, sizeof(Buffer));
	}
	else if (AppIDVariable >= 32)
	{
		WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] SteamAPI_RestartAppIfNecessary --> AppID is too long!\r\n");
		SecureZeroMemory(Buffer, sizeof(Buffer));
		return true;
	}
	else
	{
		errno = 0x0;

		long ConvertAppID = strtol(Buffer, nullptr, 10);

		if (errno == 0x0)
		{
			if (ConvertAppID != LONG_MAX && ConvertAppID != LONG_MIN && ConvertAppID != 0)
			{
				return false;
			}
		}
	}

	FILE* AppIDFile = nullptr;

	if (fopen_s(&AppIDFile, "steam_appid.txt", "rb") == 0)
	{
		char szAppId[32] = { 0 };
		if (fgets(szAppId, 32, AppIDFile) != nullptr)
		{
			fclose(AppIDFile);

			errno = 0x0;

			long ConvertAppID = strtol(szAppId, nullptr, 10);

			if (errno == 0x0)
			{
				if (ConvertAppID != LONG_MAX && ConvertAppID != LONG_MIN && ConvertAppID != 0)
				{
					return false;
				}
			}
		}
		else
		{
			fclose(AppIDFile);
		}
	}

	WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] SteamAPI_RestartAppIfNecessary Failed!\r\n");
	return true;
}

S_API void* S_CALLTYPE SteamInternal_ContextInit(void *pContextInitData)
{
	_cprintf_s("[Steam_API_Base] SteamInternal_ContextInit\r\n");

	if (pContextInitData != nullptr)
	{
        #if defined(_M_IX86)
		ContextInit* InitData = (ContextInit*)pContextInitData;

		char* pPointer = static_cast<char*>(pContextInitData);

		if (InitData->Counter == ContextCounter)
			return pPointer + 8;

		AcquireSRWLockExclusive(&ContextLock);

		if (InitData->Counter != ContextCounter)
		{
			InitData->pFn(pPointer + 8);
			InitData->Counter = ContextCounter;
		}

		ReleaseSRWLockExclusive(&ContextLock);

		return pPointer + 8;
        #endif

        #if defined(_M_AMD64)
		ContextInit* InitData = (ContextInit*)pContextInitData;

		char* pPointer = static_cast<char*>(pContextInitData);

		if (InitData->Counter == ContextCounter)
			return pPointer + 16;

		AcquireSRWLockExclusive(&ContextLock);

		if (InitData->Counter != ContextCounter)
		{
			InitData->pFn(pPointer + 16);
			InitData->Counter = ContextCounter;
		}

		ReleaseSRWLockExclusive(&ContextLock);

		return pPointer + 16;
        #endif
	}

	WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] SteamInternal_ContextInit Failed!\r\n");
	return nullptr;

	//SDK 1.42
/*#if defined(_M_IX86)
	__asm
	{
	    mov esi, [pContextInitData]
	    mov eax, [esi + 4]
	    cmp eax, ContextCounter
	    lea eax, [esi + 8]
	    jz Skip
	    push eax
	    mov eax, [esi]
	    call eax
	    mov eax, ContextCounter
	    add esp, 4
	    mov[esi + 4], eax
	    lea eax, [esi + 8]
	    Skip:
	}
#endif*/
}