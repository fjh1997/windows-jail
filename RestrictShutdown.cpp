#include <Windows.h>
#include <conio.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <strsafe.h>
#include <WinSafer.h>

#include <locale.h>
#define BUFSIZE 4096

// Experiment code from the Web article:
//	Windows Vista for Developers �C Part 4 �C User Account Control
//	https://weblogs.asp.net/kennykerr/Windows-Vista-for-Developers-_1320_-Part-4-_1320_-User-Account-Control
void ReadFromPipe(void);
void ErrorExit(PTSTR);
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;
static wchar_t commandLine[1024];
static const wchar_t *skipBlanks(const wchar_t *s)
{
	while (*s == L' ')
		s++;
	return s;
}

static const wchar_t *skipNonBlanks(const wchar_t *s)
{
	while (*s != 0 && *s != L' ')
		s++;
	return s;
}

static const wchar_t *skipPastChar(const wchar_t *s, wchar_t c)
{
	while (*s != 0)
	{
		if (*s == c)
		{
			s++;
			break;
		}
		s++;
	}
	return s;
}

static const wchar_t *parseNextWord(const wchar_t *s, wchar_t *wBuf, int wBufSize)
{
	const wchar_t *s1 = skipBlanks(s);
	const wchar_t *s2 = skipNonBlanks(s1);
	wcsncpy_s(wBuf, wBufSize, s1, s2 - s1);
	return s2;
}

static bool parseCommandLineParms()
{
	const wchar_t *s = GetCommandLine();
	s = skipBlanks(s);
	if (*s == L'"')
		s = skipPastChar(s + 1, L'"'); // if the commandline starts with a quote, we have to skip past the next quote
	else
		s = skipNonBlanks(s); // otherwise the program path does not contain blanks and we skip to the next blank
	s = skipBlanks(s);
	if (*s == 0)
	{ // no command-line parameters
		_tprintf(_T("usage: restrictshutdown.exe cmdline\n"));
		return false;
	}
	// s = parseNextWord(s, user, sizeof(user)/2);
	//  s = parseNextWord(s, password, sizeof(password)/2);
	s = skipBlanks(s);
	wcscpy_s(commandLine, s);
	if (commandLine[0] == 0)
	{
		fprintf(stderr, "Missing command-line arguments.\n");
		return false;
	}
	return true;
}
void do_test(LPWSTR cmdline)
{
	// HANDLE hTokenOrigProcess = nullptr;
	// HANDLE hTokenRestricted = nullptr;
	BOOL succ;
	// BOOL succ = OpenProcessToken(GetCurrentProcess(),
	// 	TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY,
	// 	&hTokenOrigProcess);
	// assert(succ);

	// // Will put `Administrators` (S-1-5-32-544) to disabled-SID list
	// //
	// unsigned char SIDbuf[sizeof(SID)+sizeof(DWORD)] = {};
	// SID *pSIDAdms = (SID*)&SIDbuf;
	// DWORD sidbytes = sizeof(SIDbuf);
	// succ = CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, pSIDAdms, &sidbytes);
	// assert(succ);
	// SID_AND_ATTRIBUTES sidsToDisable[] =
	// {
	// 	{ pSIDAdms	,0 },
	// };

	// // Will delete privilege "SeShutdownPrivilege"
	// //
	// LUID shutdownPrivilege = {};
	// succ = LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &shutdownPrivilege);
	// assert(succ);

	// // Create the restricted-token.
	// //
	// LUID_AND_ATTRIBUTES privsToDelete[] =
	// {
	// 	{ shutdownPrivilege, 0 },
	// };
	// succ = CreateRestrictedToken(hTokenOrigProcess,
	// 	DISABLE_MAX_PRIVILEGE, // flags
	// 	ARRAYSIZE(sidsToDisable), sidsToDisable,
	// 	ARRAYSIZE(privsToDelete), privsToDelete,
	// 	0, 0, // SIDs to restrict
	// 	&hTokenRestricted);
	// assert(succ);

	// Create a new Explorer.exe process with the new restricted-token.
	SAFER_LEVEL_HANDLE hLevel = NULL;
	if (!SaferCreateLevel(SAFER_SCOPEID_USER, SAFER_LEVELID_CONSTRAINED, SAFER_LEVEL_OPEN, &hLevel, NULL)) // runas  /trustlevel:0x10000   see https://learn.microsoft.com/zh-cn/windows/win32/api/winsafer/nf-winsafer-safercreatelevel
	{
		ErrorExit(TEXT("SaferCreateLevel"));
	}

	HANDLE hRestrictedToken = NULL;
	if (!SaferComputeTokenFromLevel(hLevel, NULL, &hRestrictedToken, 0, NULL))
	{
		SaferCloseLevel(hLevel);
		ErrorExit(TEXT("SaferComputeTokenFromLevel"));
	}

	SaferCloseLevel(hLevel);
	// Set the bInheritHandle flag so pipe handles are inherited.
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT.

	if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
		ErrorExit(TEXT("StdoutRd CreatePipe"));

	// Ensure the read handle to the pipe for STDOUT is not inherited.

	if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
		ErrorExit(TEXT("Stdout SetHandleInformation"));

	STARTUPINFO siStartInfo;
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));

	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = g_hChildStd_OUT_Wr;
	siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	PROCESS_INFORMATION procInfo;
	ZeroMemory(&procInfo, sizeof(PROCESS_INFORMATION));

	//_tprintf(_T("Now I will relaunch a new Explorer.exe process with restricted token.\n"));
	//_tprintf(_T("This new process will NOT let you shutdown the computer.\n"));

	//_tprintf(_T("\n"));
	//_tprintf(_T("Press Enter to Continue.\n"));

	//_getch();

	//_tprintf(_T("\n"));

	// WinExec("taskkill /F /IM explorer.exe", SW_SHOW);
	// Sleep(2000);

	succ = CreateProcessAsUser(hRestrictedToken,
							   0,				   // appname
							   cmdline,			   // cmd line
							   NULL,			   // process attributes
							   NULL,			   // thread attributes
							   TRUE,			   //  inherit handles
							   CREATE_NEW_CONSOLE, // flags
							   NULL,			   // inherit environment
							   NULL,			   // inherit current directory
							   &siStartInfo,
							   &procInfo);

	if (succ == false)
	{
		ErrorExit(TEXT("create"));
	}

	succ = CloseHandle(hRestrictedToken);
	succ = CloseHandle(procInfo.hThread);
	succ = CloseHandle(procInfo.hProcess);
	CloseHandle(g_hChildStd_OUT_Wr); // should close before read pipe,or the fileread() function will never return
	ReadFromPipe();

	//_tprintf(_T("OK. A new Explorer.exe process has been created.\n"));
}

int _tmain(int argc, TCHAR *argv[])
{

	setlocale(LC_ALL, "");

	if (!parseCommandLineParms())
		return 9;

	do_test(commandLine);

	return 0;
}

void ReadFromPipe(void)

// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT.
// Stop when there is no more data.
{
	DWORD dwRead, dwWritten;
	CHAR chBuf[BUFSIZE];
	BOOL bSuccess = FALSE;
	HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	for (;;)
	{
		bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
		if (!bSuccess || dwRead == 0)
		{
			break;
		}

		bSuccess = WriteFile(hParentStdOut, chBuf,
							 dwRead, &dwWritten, NULL);
		if (!bSuccess)
			break;
	}
}

void ErrorExit(PTSTR lpszFunction)

// Format a readable error message, display a message box,
// and exit from the application.
{
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
									  (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
					LocalSize(lpDisplayBuf) / sizeof(TCHAR),
					TEXT("%s failed with error %d: %s"),
					lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(1);
}