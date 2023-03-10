#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>

#define BUFSIZE 4096

HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

HANDLE g_hInputFile = NULL;

static wchar_t user[64];
static wchar_t password[64];
static wchar_t commandLine[1024];

void CreateChildProcess(void);
void WriteToPipe(void);
void ReadFromPipe(void);
void ErrorExit(PTSTR);
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
      _tprintf(_T("RunAsUsr <user> <password> <commandline>\n"));
      return false;
   }
   s = parseNextWord(s, user, sizeof(user) / 2);
   s = parseNextWord(s, password, sizeof(password) / 2);
   s = skipBlanks(s);
   wcscpy_s(commandLine, s);
   if (commandLine[0] == 0)
   {
      fprintf(stderr, "Missing command-line arguments.\n");
      return false;
   }
   return true;
}
int _tmain(int argc, TCHAR *argv[])
{
   SECURITY_ATTRIBUTES saAttr;
   if (!parseCommandLineParms()) return 9;

   // Set the bInheritHandle flag so pipe handles are inherited.

   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
   saAttr.bInheritHandle = TRUE;
   saAttr.lpSecurityDescriptor = NULL;

   // Create a pipe for the child process's STDOUT.

   if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
      ErrorExit(TEXT("StdoutRd CreatePipe"));

   // Ensure the read handle to the pipe for STDOUT is not inherited.

   if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
      ErrorExit(TEXT("Stdout SetHandleInformation"));

   // Create the child process.

   CreateChildProcess();

   // Get a handle to an input file for the parent.
   // This example assumes a plain text file and uses string output to verify data flow.

   ReadFromPipe();


   // The remaining open handles are cleaned up when this process terminates.
   // To avoid resource leaks in a larger application, close handles explicitly.

   return 0;
}

void CreateChildProcess()
// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{
   //TCHAR szCmdline[] = TEXT("cmd.exe /c whoami");
   PROCESS_INFORMATION piProcInfo;
   STARTUPINFO siStartInfo;
   BOOL bSuccess = FALSE;

   // Set up members of the PROCESS_INFORMATION structure.

   ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

   // Set up members of the STARTUPINFO structure.
   // This structure specifies the STDIN and STDOUT handles for redirection.

   ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
   siStartInfo.cb = sizeof(STARTUPINFO);
   siStartInfo.hStdError = g_hChildStd_OUT_Wr;
   siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
   siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

   // Create the child process.

   bSuccess = CreateProcessWithLogonW(user,
                                      NULL,
                                      password,
                                      NULL,         // logon flag
                                      NULL,         // Appname
                                     commandLine,    // command line
                                      0,            // creation flags
                                      NULL,         // use parent's environment
                                      NULL,         // use parent's current directory
                                      &siStartInfo, // STARTUPINFO pointer
                                      &piProcInfo); // receives PROCESS_INFORMATION

   // If an error occurs, exit the application.
   if (!bSuccess)
      ErrorExit(TEXT("CreateProcess"));
   else
   {
      // Close handles to the child process and its primary thread.
      // Some applications might keep these handles to monitor the status
      // of the child process, for example.

      CloseHandle(piProcInfo.hProcess);
      CloseHandle(piProcInfo.hThread);

      // Close handles to the stdin and stdout pipes no longer needed by the child process.
      // If they are not explicitly closed, there is no way to recognize that the child process has ended.

      CloseHandle(g_hChildStd_OUT_Wr);
   }
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
       if( ! bSuccess || dwRead == 0 ) {
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