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

void EnsureNoSteamAppIdFile()
{
	const char* appIdFileName = "steam_appid.txt";

	// Проверяем существование файла
	DWORD fileAttributes = GetFileAttributesA(appIdFileName);

	if (fileAttributes != INVALID_FILE_ATTRIBUTES)
	{
		// Файл существует, удаляем его
		if (DeleteFileA(appIdFileName))
		{
			WriteColoredText(FOREGROUND_YELLOW | FOREGROUND_INTENSITY, 7,
				"[Steam_API_Base] Removed existing steam_appid.txt file\r\n");
		}
		else
		{
			DWORD error = GetLastError();
			char buffer[1024]; // Добавить в начало функции
			sprintf_s(buffer, sizeof(buffer), "[Steam_API_Base] Failed to remove steam_appid.txt (Error: %lu)\r\n", error);
			WriteColoredText(FOREGROUND_RED | FOREGROUND_INTENSITY, 7, buffer);
		}
	}
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

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		// Устанавливаем обработчики ошибок
		_set_invalid_parameter_handler(MyInvalidParameterHandler);
		_set_new_handler(FailedMemoryAllocationHandler);
		_set_new_mode(1);

		AllocConsole();
		FILE* fp_stdout = nullptr;
		freopen_s(&fp_stdout, "CONOUT$", "w", stdout);

		printf("[Steam_API_Base] DLL_PROCESS_ATTACH started\r\n");

		// Удаляем steam_appid.txt если он существует
		EnsureNoSteamAppIdFile();

		printf("[Steam_API_Base] Loading configuration...\r\n");

		// Инициализируем конфигурацию
		if (!GConfigManager()->LoadConfig())
		{
			printf("[Steam_API_Base] Failed to load configuration!\r\n");
		}
		else
		{
			printf("[Steam_API_Base] Configuration loaded successfully\r\n");

			// Выводим содержимое конфига для отладки
			printf("[Steam_API_Base] Debug config:\r\n");
			printf("  appid: %d\r\n", GConfigManager()->GetInt("steam", "appid", 0));
			printf("  wrappermode: %s\r\n", GConfigManager()->GetBool("steam", "wrappermode", false) ? "true" : "false");
			printf("  newappid: %d\r\n", GConfigManager()->GetInt("steam_wrapper", "newappid", 0));
		}

		printf("[Steam_API_Base] Loading Steam settings...\r\n");

		// Загружаем настройки
		if (!GSteamSettings()->LoadSettings())
		{
			printf("[Steam_API_Base] Failed to load Steam settings!\r\n");
		}
		else
		{
			printf("[Steam_API_Base] Steam settings loaded successfully\r\n");

			// Выводим настройки для отладки
			printf("[Steam_API_Base] Debug Steam settings:\r\n");
			printf("  Original AppID: %u\r\n", GSteamSettings()->GetOriginalAppId());
			printf("  Wrapper AppID: %u\r\n", GSteamSettings()->GetWrapperAppId());
			printf("  Wrapper Mode: %s\r\n", GSteamSettings()->IsWrapperMode() ? "true" : "false");
			printf("  Active AppID (after substitution): %u\r\n", GSteamSettings()->GetActiveAppId());
		}

		// Проверяем, что AppID установлен корректно
		uint32 activeAppID = GSteamSettings()->GetActiveAppId();
		printf("[Steam_API_Base] Final Active AppID: %u\r\n", activeAppID);

		{
			char buffer[1024];
			sprintf_s(buffer, sizeof(buffer), "[Steam_API_Base] Final Active AppID: %u\r\n", activeAppID);
			WriteColoredText(FOREGROUND_GREEN | FOREGROUND_INTENSITY, 7, buffer);
		}

		// Инициализация остальных компонентов
		InitializeSRWLock(&ContextLock);
		InitializeSRWLock(&CallbackLock);

		GetInterfacePointers.Clear();
		GetGameServerInterfacePointers.Clear();

		printf("[Steam_API_Base] DLL_PROCESS_ATTACH completed\r\n");

		break;
	}
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		// Очистка при выгрузке DLL
		if (lpReserved == NULL)
		{
			// Вызвано FreeLibrary, выполняем очистку
			SteamAPI_Shutdown();
		}
		break;
	}
	return TRUE;
}