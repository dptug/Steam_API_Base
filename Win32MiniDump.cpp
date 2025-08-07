#include <Windows.h>
#include <Shlwapi.h>
#include "Win32MiniDump.h"

//Made by RevCrew, Steam006

CWin32MiniDump::CWin32MiniDump()
{
	this->szMinidumpFolder = new wchar_t[MAX_PATH]();
	this->m_sName.clear();
	this->m_sComment.clear();
	this->m_hDbgHelp = nullptr;
	this->m_fnMiniDumpWriteDump = nullptr;
	this->bInit = false;

	this->m_hDbgHelp = LoadLibraryExW(L"DbgHelp.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

	if (this->m_hDbgHelp != nullptr)
	{
		FARPROC fpMiniDumpWriteDump = GetProcAddress(this->m_hDbgHelp, "MiniDumpWriteDump");

		if (fpMiniDumpWriteDump != nullptr)
		{
			this->m_fnMiniDumpWriteDump = fnMiniDumpWriteDump(fpMiniDumpWriteDump);

			wchar_t szExePath[MAX_PATH] = { 0 };
			const DWORD ModuleFileName = GetModuleFileNameW(nullptr, szExePath, MAX_PATH);

			if (ModuleFileName == 0)
			{
				MessageBoxW(nullptr, L"Failed to get minidump folder path (1)!", L"Win32MiniDump", MB_ICONERROR);
				ExitProcess(0);
			}

			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				MessageBoxW(nullptr, L"Failed to get minidump folder path (2)!", L"Win32MiniDump", MB_ICONERROR);
				ExitProcess(0);
			}

			wchar_t szExeName[MAX_PATH] = { 0 };
			wcscpy_s(szExeName, MAX_PATH, szExePath);

			PathStripPathW(szExeName);
			PathRemoveExtensionW(szExeName);

			this->m_sName.assign(szExeName);

			wchar_t* szExePathNew = wcsrchr(szExePath, L'\\');

			if (szExePathNew != nullptr)
				*(szExePathNew) = L'\0';

			const int StringFactory = _snwprintf_s(this->szMinidumpFolder, MAX_PATH, _TRUNCATE, L"%s\\%s_Minidumps", szExePath, szExeName);

			if (StringFactory == -1)
			{
				MessageBoxW(nullptr, L"Unable to get minidump folder path (3)!", L"Win32MiniDump", MB_ICONERROR);
				ExitProcess(0);
			}

			const BOOL bCreateFolder = CreateDirectoryW(this->szMinidumpFolder, nullptr);

			if (bCreateFolder == FALSE && GetLastError() != ERROR_ALREADY_EXISTS)
			{
				MessageBoxW(nullptr, L"Unable to get minidump folder path (4)!", L"Win32MiniDump", MB_ICONERROR);
				ExitProcess(0);
			}

			this->SRWInit();
			this->bInit = true;
		}
		else
		{
			FreeLibrary(this->m_hDbgHelp);
			this->m_hDbgHelp = nullptr;
			delete[]this->szMinidumpFolder;
		}
	}
	else
	{
		delete[]this->szMinidumpFolder;
	}
}

CWin32MiniDump::~CWin32MiniDump()
{
	this->m_fnMiniDumpWriteDump = nullptr;

	this->m_sName.clear();
	this->m_sComment.clear();

	if (this->m_hDbgHelp != nullptr)
	{
		FreeLibrary(this->m_hDbgHelp);
		this->m_hDbgHelp = nullptr;
	}

	delete[]this->szMinidumpFolder;
	this->bInit = false;
}

bool CWin32MiniDump::InitStatus()
{
	return this->bInit;
}

void CWin32MiniDump::SRWInit()
{
	InitializeSRWLock(&this->MiniDumpLock);
}

void CWin32MiniDump::SRWEnter()
{
	AcquireSRWLockExclusive(&this->MiniDumpLock);
}

void CWin32MiniDump::SRWLeave()
{
	ReleaseSRWLockExclusive(&this->MiniDumpLock);
}

void CWin32MiniDump::SetComment(const wchar_t* cszComment)
{
	this->m_sComment.assign(cszComment);
}

size_t CWin32MiniDump::GetCommentSize()
{
	if (this->m_sComment.length() == 0)
		return 0;

	return this->m_sComment.length() * sizeof(wchar_t);
}

const wchar_t* CWin32MiniDump::GetComment()
{
	return this->m_sComment.c_str();
}

void CWin32MiniDump::ClearComment()
{
	this->m_sComment.clear();
}

void CWin32MiniDump::WriteUsingExceptionInfo(DWORD dwExceptionCode, _EXCEPTION_POINTERS* pStructuredExceptionPointers)
{
	if (this->m_hDbgHelp == nullptr)
		return;

	if (this->m_fnMiniDumpWriteDump == nullptr)
		return;

	if (this->m_sName.length() == 0)
		return;

	HANDLE hCurrentProcess = GetCurrentProcess();
	const DWORD PID = GetCurrentProcessId();

	SYSTEMTIME Time = { 0 };
	GetLocalTime(&Time);

	wchar_t szFileName[MAX_PATH] = { 0 };
	const int StringFactory = _snwprintf_s(szFileName, MAX_PATH, _TRUNCATE, L"%s\\%s_%04u-%02u-%02u-%02u.%02u.%02u.mdmp", this->szMinidumpFolder, this->m_sName.c_str(), Time.wYear, Time.wMonth, Time.wDay, Time.wHour, Time.wMinute, Time.wSecond);

	if (StringFactory == -1)
		return;

	this->SRWEnter();

	HANDLE hMiniDumpFile = CreateFileW(szFileName, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (hMiniDumpFile == INVALID_HANDLE_VALUE)
	{
		this->SRWLeave();
		return;
	}

	MINIDUMP_EXCEPTION_INFORMATION mdmpExceptionInfo = { 0 };
	mdmpExceptionInfo.ThreadId = GetCurrentThreadId();
	mdmpExceptionInfo.ExceptionPointers = pStructuredExceptionPointers;
	mdmpExceptionInfo.ClientPointers = FALSE;

	if (this->GetCommentSize() != 0)
	{
		MINIDUMP_USER_STREAM UserStream[2] = { 0 };

		UserStream[0].Type = CommentStreamW;
		UserStream[0].BufferSize = (ULONG)this->GetCommentSize();
		UserStream[0].Buffer = (LPVOID)this->GetComment();

		MINIDUMP_EXCEPTION_STREAM MinidumpExcptionStream = { 0 };
		MinidumpExcptionStream.ExceptionRecord.ExceptionCode = dwExceptionCode;

		UserStream[1].Type = ExceptionStream;
		UserStream[1].BufferSize = sizeof(MINIDUMP_EXCEPTION_STREAM);
		UserStream[1].Buffer = &MinidumpExcptionStream;

		MINIDUMP_USER_STREAM_INFORMATION MinidumpUserStreamInfo = { 0 };
		MinidumpUserStreamInfo.UserStreamCount = 2;
		MinidumpUserStreamInfo.UserStreamArray = UserStream;

		this->m_fnMiniDumpWriteDump(hCurrentProcess, PID, hMiniDumpFile, (_MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpWithHandleData | MiniDumpWithUnloadedModules | MiniDumpWithProcessThreadData | MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo), &mdmpExceptionInfo, &MinidumpUserStreamInfo, nullptr);
	}
	else
	{
		MINIDUMP_USER_STREAM UserStream[1] = { 0 };

		MINIDUMP_EXCEPTION_STREAM MinidumpExcptionStream = { 0 };
		MinidumpExcptionStream.ExceptionRecord.ExceptionCode = dwExceptionCode;

		UserStream[0].Type = ExceptionStream;
		UserStream[0].BufferSize = sizeof(MINIDUMP_EXCEPTION_STREAM);
		UserStream[0].Buffer = &MinidumpExcptionStream;

		MINIDUMP_USER_STREAM_INFORMATION MinidumpUserStreamInfo = { 0 };
		MinidumpUserStreamInfo.UserStreamCount = 1;
		MinidumpUserStreamInfo.UserStreamArray = UserStream;

		this->m_fnMiniDumpWriteDump(hCurrentProcess, PID, hMiniDumpFile, (_MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpWithHandleData | MiniDumpWithUnloadedModules | MiniDumpWithProcessThreadData | MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo), &mdmpExceptionInfo, &MinidumpUserStreamInfo, nullptr);
	}

	CloseHandle(hMiniDumpFile);
	this->SRWLeave();
}