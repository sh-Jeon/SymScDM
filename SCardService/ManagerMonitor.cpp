
#include "StdAfx.h"
#include "ManagerMonitor.h"
#include <Sddl.h>
#include "../Define.h"
#include <strsafe.h>
#include <atlpath.h>
using namespace ATL;

#define PIPE_BUF_SIZE  16 * 1024

CManagerMonitor g_ManagerMonitor;

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

CManagerMonitor::CManagerMonitor(void)
{
	m_hPipeThread = NULL;
	m_hMonExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CManagerMonitor::~CManagerMonitor(void)
{
	SetEvent(m_hMonExitEvent);
	WaitForSingleObject(m_hMonThread, 3000);
	CloseHandle(m_hMonExitEvent);
}

void CManagerMonitor::init(DWORD dwMainThread)
{
	ATLTRACE(L"init monitor\n");
	unsigned int thread_addr;
	m_hPipeThread = (HANDLE)_beginthreadex(0, 0, NamedPipeThread, this, 0, &thread_addr);

	m_hMonThread = (HANDLE)_beginthreadex(0, 0, ProcessManagerThread, this, 0, &thread_addr);
}

void CManagerMonitor::RestartProcess(DWORD dwPid)
{
	MAP_MGR_PROCESS::iterator itr = map_Mgr_Process.find(dwPid);
	if (itr != map_Mgr_Process.end())
	{
		ATLTRACE(L"RestartProcess - exit flag %d", itr->second.bExit);
		if (FALSE == itr->second.bExit)
		{
			std::wstring strPath = itr->second.strPath;

			// Start Msg to SCardManager.exe 
			ATLTRACE(L"RestartProcess - path %s", strPath.c_str());
			// Start Msg to SCardManager.exe 
			HANDLE hLauncher = OpenNamedPipeHandle(LAUNCHER_PIPENAME, 10000);
			if (hLauncher != INVALID_HANDLE_VALUE)
			{
				DWORD dwBytesWritten;
				SVC_CMD svcCmd;
				svcCmd.Cmd = CMD_SET_PID;
				svcCmd.dwPid = _getpid();

				StringCbCopy(svcCmd.szManagerPath, sizeof(svcCmd.szManagerPath), strPath.c_str());
				BOOL bWrite = WriteFile(hLauncher, &svcCmd, sizeof(svcCmd), &dwBytesWritten, 0);
				if (!bWrite)
				{
					CloseHandle(hLauncher);
					return;
				}

				FlushFileBuffers(hLauncher);

				DWORD dwRead;
				ZeroMemory(&svcCmd, sizeof(SVC_CMD));
				if (!ReadFile(hLauncher, &svcCmd, sizeof(SVC_CMD), &dwRead, 0))
				{
					CloseHandle(hLauncher);
					return;
				}

				CloseHandle(hLauncher);
			}  
		}
		else
		{
			map_Mgr_Process.erase(itr);
		}
	}
}

unsigned int __stdcall CManagerMonitor::ProcessManagerThread(void *param)
{
	CManagerMonitor* pMonitor = (CManagerMonitor*)param;
	while (1)
	{
		DWORD dwIdx = WaitForSingleObject(pMonitor->m_hMonExitEvent, 10000);
		if (dwIdx == WAIT_OBJECT_0) break;

		pMonitor->_CheckMgrAliveTime();
	}

	return 0;
}

void CManagerMonitor::_CheckMgrAliveTime()
{
	MAP_MGR_PROCESS::iterator itr = map_Mgr_Process.begin();
	while (itr != map_Mgr_Process.end())
	{
		if (itr->second.bExit == FALSE && GetTickCount() - itr->second.dwLastMsgTick > 15000)
		{
			// Kill Process & Restart Process
			DWORD dwPid = itr->first;
			std::wstring strPath = itr->second.strPath;
			map_Mgr_Process.erase(itr);

			HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, dwPid);
			if (process)
			{
				if (ERROR_INVALID_PARAMETER != GetLastError())
				{
					TerminateProcess(process, (UINT)-1);
				}

				CloseHandle(process);
			}

			// Start Msg to SCardManager.exe 
			HANDLE hLauncher = OpenNamedPipeHandle(LAUNCHER_PIPENAME, 10000);
			if (hLauncher != INVALID_HANDLE_VALUE)
			{
				DWORD dwBytesWritten;
				SVC_CMD svcCmd;
				svcCmd.Cmd = CMD_SET_PID;
				svcCmd.dwPid = _getpid();

				StringCbCopy(svcCmd.szManagerPath, sizeof(svcCmd.szManagerPath), strPath.c_str());
				BOOL bWrite = WriteFile(hLauncher, &svcCmd, sizeof(svcCmd), &dwBytesWritten, 0);
				if (!bWrite)
				{
					CloseHandle(hLauncher);
					return;
				}

				FlushFileBuffers(hLauncher);

				DWORD dwRead;
				ZeroMemory(&svcCmd, sizeof(SVC_CMD));
				if (!ReadFile(hLauncher, &svcCmd, sizeof(SVC_CMD), &dwRead, 0))
				{
					CloseHandle(hLauncher);
					return;
				}

				CloseHandle(hLauncher);
			}

			break;
		}

		itr++;
	}
}

void CManagerMonitor::Close()
{
	HANDLE hPipe = CreateFile(
		MONITOR_SVC_PIPENAME,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if ( hPipe != INVALID_HANDLE_VALUE )
	{
		DWORD dwBytesWritten;
		SVC_CMD svcCmd;
		svcCmd.Cmd = CMD_CLOSE_PIPE;
		
		WriteFile ( hPipe, &svcCmd, sizeof ( SVC_CMD ), &dwBytesWritten, 0 );
		CloseHandle ( hPipe );
	}

	HANDLE hPipeLauncher = CreateFile(
		LAUNCHER_PIPENAME,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if ( hPipeLauncher != INVALID_HANDLE_VALUE )
	{
		DWORD dwBytesWritten;
		SVC_CMD svcCmd;
		svcCmd.Cmd = CMD_CLOSE_PIPE;
		
		WriteFile ( hPipe, &svcCmd, sizeof ( SVC_CMD ), &dwBytesWritten, 0 );
		CloseHandle ( hPipe );
	}
}

unsigned int __stdcall CManagerMonitor::NamedPipeThread(void *param)
{
	CManagerMonitor* pMonitor = (CManagerMonitor*)param;
	pMonitor->_CheckManagerProc();
	CloseHandle(pMonitor->m_hPipeThread);
	_endthreadex ( 0 );

	return 0;
}

void CManagerMonitor::_CheckManagerProc()
{
	SECURITY_ATTRIBUTES sa;
	BuildSecurityAttributes(&sa);
	
	HANDLE hPipe = CreateNamedPipe(
		MONITOR_SVC_PIPENAME,      // pipe name
		PIPE_ACCESS_DUPLEX,       // read/write access
		PIPE_TYPE_MESSAGE |       // message type pipe
		PIPE_READMODE_MESSAGE |   // message-read mode
		PIPE_WAIT,                // blocking mode
		PIPE_UNLIMITED_INSTANCES,
		PIPE_BUF_SIZE,            // output buffer size
		PIPE_BUF_SIZE,            // input buffer size
		NMPWAIT_USE_DEFAULT_WAIT, //client time-out
		&sa);

	ATLTRACE(L"Create Named Pipe : %s\n", MONITOR_SVC_PIPENAME);

	for ( ; ; )
	{
		ATLTRACE(L"wait connect\n");
		BOOL fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);	
		if (!fConnected)
		{
			CloseHandle(hPipe);
			hPipe = CreateNamedPipe(
				MONITOR_SVC_PIPENAME,      // pipe name
				PIPE_ACCESS_DUPLEX,       // read/write access
				PIPE_TYPE_MESSAGE |       // message type pipe
				PIPE_READMODE_MESSAGE |   // message-read mode
				PIPE_WAIT,                // blocking mode
				PIPE_UNLIMITED_INSTANCES,
				PIPE_BUF_SIZE,            // output buffer size
				PIPE_BUF_SIZE,            // input buffer size
				NMPWAIT_USE_DEFAULT_WAIT, //client time-out
				&sa );
			continue;
		}
		
		BYTE buf[PIPE_BUF_SIZE];
		DWORD dwRead;
	
		if (!ReadFile(hPipe, buf, PIPE_BUF_SIZE, &dwRead, NULL))
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

				ST_PROCESS_INFO stProcessInfo;
				stProcessInfo.bExit = FALSE;
				stProcessInfo.strPath = szManagerPath;
				stProcessInfo.dwLastMsgTick = GetTickCount();
				map_Mgr_Process[dwPID] = stProcessInfo;

				unsigned int thread_addr;
				_beginthreadex( 0, 0, ProcessCheckThread, this, 0, &thread_addr );

				break;
			}	
			case CMD_ALIVE:
			{
				DWORD dwPID = pCmd->dwPid;
				pCmd->Cmd = CMD_ALIVE;

				ATLTRACE(L"MgrMonitor - cmd alive : %d\n", dwPID);
				WriteFile(hPipe, pCmd, sizeof(*pCmd), &bytesWritten, NULL);
				OnProcessAlive(dwPID);

				break;
			}
			case CMD_END_PROCESS:
			{
				DWORD dwPID = pCmd->dwPid;
				pCmd->Cmd = CMD_END_PROCESS;

				ATLTRACE(L"MgrMonitor - cmd end process : %d\n", dwPID);
				WriteFile(hPipe, pCmd, sizeof(*pCmd), &bytesWritten, NULL);
				OnEndProcess(dwPID);

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

unsigned int __stdcall CManagerMonitor::ProcessCheckThread(void *param)
{
	DWORD dwPID = (DWORD)param;
	HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, dwPID);	
	if ( hProcess )
	{
		WaitForSingleObject(hProcess, INFINITE);

		// 정상 종료가 아니면 해당pid의 path를 찾아서 재실행 함.
		g_ManagerMonitor.RestartProcess(dwPID);
	}
	
	return 0;
}

void CManagerMonitor::OnProcessAlive(DWORD dwPID)
{
	// manager 살아 있음 표시
	MAP_MGR_PROCESS::iterator itr = map_Mgr_Process.find(dwPID);
	if (itr != map_Mgr_Process.end())
	{
		itr->second.dwLastMsgTick = GetTickCount();
	}
}

void CManagerMonitor::OnEndProcess(DWORD dwPID)
{
	// 정상적으로 프로그램이 종료 됨.
	ATLTRACE(L"OnEndProcess %d", dwPID);
	MAP_MGR_PROCESS::iterator itr = map_Mgr_Process.find(dwPID);
	if (itr != map_Mgr_Process.end())
	{
		ATLTRACE(L"OnEndProcess-SetPID %d", dwPID);
		itr->second.bExit = TRUE;
	}
}


