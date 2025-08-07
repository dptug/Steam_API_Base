//Client
HMODULE hSteamclient_Client = nullptr;
HSteamPipe hPipe = 0;
HSteamUser hUser = 0;
ISteamClient *SteamClient_Client = nullptr;
ISteamClient *SteamClientSafe = nullptr;
ISteamUtils *SteamUtilsForCallbacks = nullptr;
ISteamController *SteamControllerForCallbacks = nullptr;
ISteamInput *SteamInputForCallbacks = nullptr;
CSteamAPIContext GetInterfacePointers;
bool InitClientPointers = false;
SRWLOCK ContextLock;

//Server
HMODULE hSteamclient_Server = nullptr;
HSteamPipe hPipeServer = 0;
HSteamUser hUserServer = 0;
ISteamClient *SteamClient_Server = nullptr;
ISteamGameServer *pSteamGameServer = nullptr;
ISteamUtils *pSteamUtilsServer = nullptr;
CSteamGameServerAPIContext GetGameServerInterfacePointers;
bool InitGameServerPointers = false;

EServerMode eGameServerMode = eServerModeInvalid;

//Breakpad
bool UsingBreakpadCrashHandler = false;
bool FullMemoryDumps = false;
void *BreakpadContext = nullptr;
PFNPreMinidumpCallback BreakpadMinidumpCallback = nullptr;
char szBreakpadVersion[64] = { 0 };
char szBreakpadTimeAndDate[64] = { 0 };
uint32 BreakpadAppID = 0;
uint64 BreakpadSteamID = 0;

bool TryCatch = false;
int UseManualDispatch = 0;
char szSteamInstallPath[MAX_PATH] = { 0 };
bool AlreadyHaveInstallPath = false;
SRWLOCK CallbackLock;

void WriteColoredText(WORD Color, WORD OriginalColor, const char* Text);
void* InternalAPI_Init(HMODULE* hSteamclient, bool bInitLocal, const char *Interface);
void LoadBreakpad(HMODULE hSteamclient);
void SetMinidumpSteamID(uint64 SteamID);

typedef void* (S_CALLTYPE *_CreateInterface)(const char *pName, int *pReturnCode);
_CreateInterface oCreateInterface = nullptr;

typedef void (S_CALLTYPE *_ReleaseThreadLocalMemory)(int bThreadExit);
_ReleaseThreadLocalMemory oReleaseThreadLocalMemory = nullptr;

typedef bool (S_CALLTYPE* _IsKnownInterface)(const char *Interface);
_IsKnownInterface oIsKnownInterface = nullptr;

typedef void (S_CALLTYPE* _NotifyMissingInterface)(HSteamPipe hSteamPipe, const char* Interface);
_NotifyMissingInterface oNotifyMissingInterface = nullptr;

//Breakpad Functions

typedef void (S_CALLTYPE *_Breakpad_SteamMiniDumpInit)(uint32 AppID, const char *pchVersion, const char *Dates, bool bFullMemoryDumps, void *pvContext, PFNPreMinidumpCallback m_pfnPreMinidumpCallback);
_Breakpad_SteamMiniDumpInit oBreakpad_SteamMiniDumpInit = nullptr;

typedef void (S_CALLTYPE *_Breakpad_SteamSetAppID)(uint32 AppID);
_Breakpad_SteamSetAppID oBreakpad_SteamSetAppID = nullptr;

typedef void (S_CALLTYPE *_Breakpad_SteamSetSteamID)(uint64 ulSteamID);
_Breakpad_SteamSetSteamID oBreakpad_SteamSetSteamID = nullptr;

typedef void (S_CALLTYPE *_Breakpad_SteamWriteMiniDumpSetComment)(const char *Comment);
_Breakpad_SteamWriteMiniDumpSetComment oBreakpad_SteamWriteMiniDumpSetComment = nullptr;

typedef void (S_CALLTYPE *_Breakpad_SteamWriteMiniDumpUsingExceptionInfoWithBuildId)(uint32 uStructuredExceptionCode, void* pvExceptionInfo, uint32 uBuildID);
_Breakpad_SteamWriteMiniDumpUsingExceptionInfoWithBuildId oBreakpad_SteamWriteMiniDumpUsingExceptionInfoWithBuildId = nullptr;

uintp ContextCounter = 0;

struct ContextInit
{ 
	void(*pFn)(void* pCtx);
	uintp Counter;
	CSteamAPIContext ctx;
};