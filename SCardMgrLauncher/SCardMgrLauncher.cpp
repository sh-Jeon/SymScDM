// SCardMgrLauncher.cpp : main source file for SCardMgrLauncher.exe
//

#include "stdafx.h"

#include "resource.h"
#include <Sddl.h>
#include "../Define.h"

CAppModule _Module;

VOID BuildSecurityAttributes(PSECURITY_ATTRIBUTES SecurityAttributes)
{
	LPTSTR sd = L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GRGW;;;WD)(A;;GR;;;RC)";

	ZeroMemory(SecurityAttributes, sizeof(SECURITY_ATTRIBUTES));

	ConvertStringSecurityDescriptorToSecurityDescriptor(
		sd,
		SDDL_REVISION_1,
		&SecurityAttributes->lpSecurityDescriptor,
		NULL);

	SecurityAttributes->nLength = sizeof(SECURITY_ATTRIBUTES);
	SecurityAttributes->bInheritHandle = TRUE;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpstrCmdLine*/, int /*nCmdShow*/)
{
	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

//	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = 0;
	// BLOCK: Run application
	{
		SECURITY_ATTRIBUTES sa;
		BuildSecurityAttributes(&sa);
	
		HANDLE hPipe = CreateNamedPipe(
			LAUNCHER_PIPENAME,      // pipe name
			PIPE_ACCESS_DUPLEX,       // read/write access
			PIPE_TYPE_MESSAGE |       // message type pipe
			PIPE_READMODE_MESSAGE |   // message-read mode
			PIPE_WAIT,                // blocking mode
			PIPE_UNLIMITED_INSTANCES,
			1024*3,            // output buffer size
			1024*3,            // input buffer size
			NMPWAIT_USE_DEFAULT_WAIT, //client time-out
			&sa);

		ATLTRACE(L"Create Named Pipe : %s\n", LAUNCHER_PIPENAME);

		for ( ; ; )
		{
			BOOL fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);	
			if (!fConnected)
			{
				CloseHandle(hPipe);
				hPipe = CreateNamedPipe(
					LAUNCHER_PIPENAME,      // pipe name
					PIPE_ACCESS_DUPLEX,       // read/write access
					PIPE_TYPE_MESSAGE |       // message type pipe
					PIPE_READMODE_MESSAGE |   // message-read mode
					PIPE_WAIT,                // blocking mode
					PIPE_UNLIMITED_INSTANCES,
					1024*3,            // output buffer size
					1024*3,            // input buffer size
					NMPWAIT_USE_DEFAULT_WAIT, //client time-out
					&sa );
				continue;
			}

			ATLTRACE(L"launch cadrmanager\n");
		
			BYTE buf[1024*3];
			DWORD dwRead;
	
			if (!ReadFile(hPipe, buf, 1024*3, &dwRead, NULL))
			{
				CloseHandle(hPipe);
				continue;
			}

			SVC_CMD* pCmd = (SVC_CMD *)buf;
			if (pCmd->Cmd == CMD_CLOSE_PIPE)
			{
				DisconnectNamedPipe(hPipe);
				break;
			}
		
			DWORD bytesWritten = 0;
			switch (pCmd->Cmd)
			{
				case CMD_SET_PID:
				{
					DWORD dwPID = pCmd->dwPid;
					ATLTRACE(L"MgrMonitor - set pid : %d\n", dwPID);

					TCHAR szManagerPath[MAX_PATH];
					ZeroMemory(szManagerPath, MAX_PATH);
					memcpy(szManagerPath, pCmd->szManagerPath, MAX_PATH);
					ATLTRACE(L"MgrMonitor - set path : %s\n", szManagerPath);

					pCmd->Cmd = CMD_SET_PID;
					WriteFile(hPipe, pCmd, sizeof(*pCmd), &bytesWritten, NULL);

					ShellExecute(GetDesktopWindow(),L"open",szManagerPath,NULL,NULL,SW_SHOWNORMAL);   

					break;
				}	

				default:
					break;
			}
		
			FlushFileBuffers(hPipe);
			DisconnectNamedPipe(hPipe);
		}

		CloseHandle(hPipe);
	}

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
