#include <Windows.h>
#include <Shlwapi.h>
#include <conio.h>
#include <new.h>

#define STEAM_API_EXPORTS

#include "Steam\steam_api.h"
#include "Steam\steamclientpublic.h"
#include "Steam\steam_gameserver.h"

#include "RegistryFunctions.h"
#include "Win32MiniDump.h"
#include "CallbackManager.h"
#include "Steam_API_Base.h"

#include "Functions\Client.h"
#include "Functions\InterfacePointers.h"
#include "Functions\InterfacePointersNew.h"
#include "Functions\GameServer.h"
#include "Functions\Minidump.h"
#include "Functions\Callbacks.h"
#include "Functions\Shutdown.h"
#include "Functions\CreateInterface.h"
#include "Functions\Flat.h"

#include "ConfigManager.h"
#include "SteamSettings.h"

#include <synchapi.h>

extern "C" {
	S_API bool S_CALLTYPE SteamAPI_SetAppID(uint32 unAppID);
}

void LogWinAPIError(const char* functionName, DWORD errorCode)
{
	char errorMessage[256];
	FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		errorMessage,
		sizeof(errorMessage),
		nullptr);

	_cprintf_s("[Steam_API_Base] %s failed with error %lu: %s\r\n",
		functionName, errorCode, errorMessage);
}

void MyInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
{
	WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7,
		"[Steam_API_Base] Invalid parameter handler triggered!\r\n");
	// Логирование вместо немедленного выхода
}

int FailedMemoryAllocationHandler(size_t Size)
{
	MessageBoxW(nullptr, L"steam_api(64).dll Crashed (Memory Allocation Handler)!", L"Failed To Allocate Memory", MB_ICONERROR);
	ExitProcess(0);
}

void WriteColoredText(WORD Color, WORD OriginalColor, const char* Text)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Color);

	_cprintf_s("%s", Text);

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), OriginalColor);

	return;
}

void* InternalAPI_Init(HMODULE *hSteamclient, bool bInitLocal, const char *Interface)
{
	SteamUtilsForCallbacks = nullptr;
	SteamControllerForCallbacks = nullptr;
	SteamInputForCallbacks = nullptr;

	if (hSteamclient == nullptr)
		return nullptr;

	if (Interface == nullptr)
		return nullptr;

	*hSteamclient = nullptr;

	char szSteamClientDll[MAX_PATH] = { 0 };
	const char* InstallPath = SteamAPI_GetSteamInstallPath();

	if (_stricmp(InstallPath, "SteamAPIBaseInvalidPath") == 0)
	{
		if (bInitLocal == false)
			return nullptr;
	}
	else
	{
        #if defined(_M_IX86)
		    _snprintf_s(szSteamClientDll, MAX_PATH, _TRUNCATE, "%s\\steamclient.dll", InstallPath);
        #endif
        #if defined(_M_AMD64)
			_snprintf_s(szSteamClientDll, MAX_PATH, _TRUNCATE, "%s\\steamclient64.dll", InstallPath);
        #endif
	}

	if (SteamAPI_IsSteamRunning() == true)
	{
		*hSteamclient = LoadLibraryExA(szSteamClientDll, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);

		if (*hSteamclient == nullptr)
			WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] InternalAPI_Init --> Failed to load steamclient(64).dll!\r\n");
	}
	else
	{
		WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] InternalAPI_Init --> SteamAPI_IsSteamRunning() Failed!\r\n");
	}

	if (*hSteamclient == nullptr)
	{
		if (bInitLocal == false)
		{
			WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] InternalAPI_Init --> Unable to locate a running instance of Steam and bInitLocal is false!\r\n");
			return nullptr;
		}

        #if defined(_M_IX86)
		    *hSteamclient = LoadLibraryW(L"steamclient.dll");
        #endif
        #if defined(_M_AMD64)
			*hSteamclient = LoadLibraryW(L"steamclient64.dll");
        #endif

		if (*hSteamclient == nullptr)
		{
			WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] InternalAPI_Init --> Unable to locate a running instance of Steam, or a local steamclient(64).dll!\r\n");
			return nullptr;
		}
	}

	oCreateInterface = (_CreateInterface)GetProcAddress(*hSteamclient, "CreateInterface");

	if (oCreateInterface != nullptr)
	{
		SteamClientSafe = (ISteamClient *)oCreateInterface("SteamClient017", nullptr);
		oReleaseThreadLocalMemory = (_ReleaseThreadLocalMemory)GetProcAddress(*hSteamclient, "Steam_ReleaseThreadLocalMemory");
		ContextCounter++;

		return oCreateInterface(Interface, nullptr);
	}
	else
	{
		WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, "[Steam_API_Base] InternalAPI_Init --> Unable to locate interface factory in steamclient(64).dll!\r\n");

		FreeLibrary(*hSteamclient);
		*hSteamclient = nullptr;
	}

	return nullptr;
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpvReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		_invalid_parameter_handler OldHandler, NewHandler;
		NewHandler = MyInvalidParameterHandler;
		OldHandler = _set_invalid_parameter_handler(NewHandler);

		_set_new_handler(FailedMemoryAllocationHandler);

		AllocConsole();

		FILE* fp_stdout = nullptr;
		FILE* fp_stderr = nullptr;

		freopen_s(&fp_stdout, "stdout.txt", "w", stdout);
		freopen_s(&fp_stderr, "stderr.txt", "w", stderr);

		char szFullDllPath[MAX_PATH] = { 0 };

		const DWORD ModuleFileName = GetModuleFileNameA(hModule, szFullDllPath, sizeof(szFullDllPath));

		if (ModuleFileName == 0)
		{
			MessageBoxW(nullptr, L"Unable to get dll name (1)!", L"Steam API Base", MB_ICONERROR);
			ExitProcess(0);
		}

		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			MessageBoxW(nullptr, L"Unable to get dll name (2)!", L"Steam API Base", MB_ICONERROR);
			ExitProcess(0);
		}

		_cprintf_s("[Steam_API_Base] Dll Path --> %s\r\n", szFullDllPath);

        /*#if defined(_M_IX86)
		    if (StrStrIA(szFullDllPath, "steam_api.dll") == nullptr)
		    {
				MessageBoxW(nullptr, L"Dll name must be steam_api.dll!", L"Steam API Base", MB_ICONERROR);
				ExitProcess(0);
		    }
        #endif
        #if defined(_M_AMD64)
			if (StrStrIA(szFullDllPath, "steam_api64.dll") == nullptr)
			{
				MessageBoxW(nullptr, L"Dll name must be steam_api64.dll!", L"Steam API Base", MB_ICONERROR);
				ExitProcess(0);
			}
        #endif*/

		_cprintf_s("[Steam_API_Base] PID --> %lu\r\n", GetCurrentProcessId());
		_cprintf_s("[Steam_API_Base] ThreadID --> %lu\r\n", GetCurrentThreadId());

		GetInterfacePointers.Clear();
		GetGameServerInterfacePointers.Clear();

		InitializeSRWLock(&ContextLock);
		InitializeSRWLock(&CallbackLock);

		// Загружаем конфигурацию
		if (!GConfigManager()->LoadConfig())
		{
			WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7,
				"[Steam_API_Base] Failed to load configuration!\r\n");
		}

		if (!GSteamSettings()->LoadSettings())
		{
			WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7,
				"[Steam_API_Base] Failed to load Steam settings!\r\n");
		}
		else
		{
			// Выводим основную информацию о настройках
			GSteamSettings()->PrintSettings();
		}

		Win32MiniDump = new CWin32MiniDump();
	}

	return 1;
}