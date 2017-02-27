#pragma once

#include <string>

#define MONITOR_SVC_PIPENAME			_T("\\\\.\\Pipe\\MonitorCardManager")
#define LAUNCHER_PIPENAME				_T("\\\\.\\Pipe\\CardManagerLauncher")

#define CMD_CLOSE_PIPE		0
#define CMD_SET_PID			1
#define CMD_ALIVE			2
#define CMD_END_PROCESS		3

typedef struct _SVC_CMD
{
	_SVC_CMD()
	{
		ZeroMemory(this, sizeof(*this));
	}

	DWORD Cmd;
	DWORD dwPid;
	TCHAR szManagerPath[MAX_PATH];
} SVC_CMD;

inline HANDLE OpenNamedPipeHandle(
	__in const std::wstring& pipeName,
	__in DWORD dwTimeout
	)
{
	HANDLE hPipe = INVALID_HANDLE_VALUE;

	for ( ; ; )
	{
		hPipe = CreateFile(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hPipe != INVALID_HANDLE_VALUE)
		{
			break;
		}

		if (!WaitNamedPipe(pipeName.c_str(), dwTimeout))
		{
			break;
		}
	}

	return hPipe;
}