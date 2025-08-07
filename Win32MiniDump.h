#pragma once

//Made by RevCrew, Steam006

#include <string>
#include <DbgHelp.h>

using namespace std;

typedef BOOL (WINAPI* fnMiniDumpWriteDump)(IN HANDLE hProcess, IN DWORD ProcessId, IN HANDLE hFile, IN MINIDUMP_TYPE DumpType, IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL);

class CWin32MiniDump
{
private:
	wchar_t *szMinidumpFolder;
	HMODULE m_hDbgHelp;
	fnMiniDumpWriteDump m_fnMiniDumpWriteDump;
	wstring m_sName;
	wstring m_sComment;
	SRWLOCK MiniDumpLock;
	bool bInit;
public:
	CWin32MiniDump();
	~CWin32MiniDump();

	bool InitStatus();
	void SRWInit();
	void SRWEnter();
	void SRWLeave();
	void SetComment(const wchar_t* cszComment);
	size_t GetCommentSize();
	const wchar_t* GetComment();
	void ClearComment();
	void WriteUsingExceptionInfo(DWORD dwExceptionCode, _EXCEPTION_POINTERS* pStructuredExceptionPointers);
};
