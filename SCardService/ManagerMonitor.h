#pragma once

#include <map>
#include <string>
#include <atlstr.h>

#include "../Define.h"

typedef struct _process_info_
{
	DWORD dwLastMsgTick;
	BOOL bExit;
	std::wstring strPath;
}ST_PROCESS_INFO;
typedef std::map<DWORD, ST_PROCESS_INFO> MAP_MGR_PROCESS;

class CManagerMonitor
{
public:
	CManagerMonitor(void);
	~CManagerMonitor(void);

	void init(DWORD dwMainThread);
	void Close();

	static unsigned int __stdcall NamedPipeThread(void *param);
	static unsigned int __stdcall ProcessCheckThread(void *param);
	static unsigned int __stdcall ProcessManagerThread(void *param);

	void RestartProcess(DWORD dwPid);

private:
	HANDLE m_hPipeThread;
	MAP_MGR_PROCESS map_Mgr_Process;
	HANDLE m_hSvcInstance;

	HANDLE m_hMonThread;
	HANDLE m_hMonExitEvent;

	CRITICAL_SECTION m_csMap;

	void _CheckManagerProc();
	void _CheckMgrAliveTime();

	void OnProcessAlive(DWORD dwPID);
	void OnEndProcess(DWORD dwPID);
};

extern CManagerMonitor g_ManagerMonitor;