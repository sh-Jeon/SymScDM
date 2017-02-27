#include "StdAfx.h"
#include "SCardManagerApp.h"
#include "CommTcpConverter.h"
#include <atlpath.h>
#include "AgentWnd.h"
#include "../Define.h"
#include <strsafe.h>

CString g_strGroupID = "\'01\'";
CString g_strRegPath;

ST_REQ_FLAG g_stReqFlag;

CRITICAL_SECTION g_crCommander;
CSCardManagerApp _theApp;
DWORD g_dwCmdPollingTime;

CSCardManagerApp::CSCardManagerApp(void)
{
	CoInitialize(NULL);

	m_hThreadEvent = CreateEvent(0, FALSE, FALSE, 0);
	m_hCmdChkThreadEvent = CreateEvent(0, FALSE, FALSE, 0);

	g_dwCmdPollingTime = 10000;

	WSADATA wsd;
	WSAStartup(MAKEWORD(2,2),&wsd);
	if (LOBYTE(wsd.wVersion) != 2 || HIBYTE(wsd.wVersion) != 2) 
	{
		WSACleanup( );
	}

	::InitializeCriticalSectionAndSpinCount(&g_crCommander, 4000);
	::InitializeCriticalSectionAndSpinCount(&m_csReaderTime, 4000);

	TCHAR szModulePath[MAX_PATH];
	::GetModuleFileName(_Module.m_hInst, szModulePath, MAX_PATH);
	ATLPath::RemoveFileSpec(szModulePath);
	g_strRegPath = szModulePath;
	g_strRegPath += L"\\";
	g_strRegPath += PLIST_INI;

	TCHAR szGroupID[5];
	ZeroMemory(szGroupID, 5);
	if (::GetPrivateProfileString(L"GROUP_INFO", L"ID", _T(""), szGroupID, sizeof(szGroupID), g_strRegPath))
	{	
		g_strGroupID = szGroupID;
	}

	m_dwListFetch = 0;
}

CSCardManagerApp::~CSCardManagerApp(void)
{
	_ClearConveterOBJ();
	WSACleanup();

	SetEvent(m_hThreadEvent);
	WaitForSingleObject(m_hThread, 3000);

	CloseHandle(m_hThreadEvent);
	CloseHandle(m_hCmdChkThreadEvent);

	::DeleteCriticalSection(&m_csReaderTime);
	::DeleteCriticalSection(&g_crCommander);
}

void CSCardManagerApp::_ClearConveterOBJ()
{
	MAP_COMMANDER::iterator itr;
	for (itr = m_mapCommander.begin(); itr != m_mapCommander.end(); itr++)
	{
		CReaderCommander* pCommander = itr->second;
		if (pCommander)
		{
			::EnterCriticalSection(&g_crCommander);
			pCommander->CloseSession();
			::LeaveCriticalSection(&g_crCommander);
		}
	}
	
	::EnterCriticalSection(&g_crCommander);
	m_mapCommander.clear();
	::LeaveCriticalSection(&g_crCommander);
}

void CSCardManagerApp::_LoadListFetchTime()
{
	TCHAR szTimeOut[10];
	DWORD dwRet = GetPrivateProfileStringW(L"CONFIG", L"ListFetchPolling", _T(""), szTimeOut, sizeof(szTimeOut), g_strRegPath);
	if (dwRet)
	{
		m_dwListFetch = _tstol(szTimeOut);
	}
}

void CSCardManagerApp::SetTerminalTime(int nReaderId, LPCTSTR strReaderTime)
{
	::EnterCriticalSection(&m_csReaderTime);

	m_mapReaderTime[nReaderId] = strReaderTime;

	::LeaveCriticalSection(&m_csReaderTime);
}

void CSCardManagerApp::SetTerminalTime(int nReaderID)
{
	CTime dbTime;
	m_ADODB.GetServerTime(dbTime);

	CTime t(
		dbTime.GetYear(),
		dbTime.GetMonth(),
		dbTime.GetDay(),
		dbTime.GetHour(),
		dbTime.GetMinute(),
		dbTime.GetSecond());	
	CString strServerTime = t.Format(L"%Y-%m-%d %H:%M:%S");
	g_AgentWnd.SetDBServerTime(strServerTime);

	CString szSetTime = dbTime.Format(L"%Y%m%d%H%M%S");	


	::EnterCriticalSection(&g_crCommander);

	MAP_COMMANDER::iterator itr = m_mapCommander.begin();
	while (itr != m_mapCommander.end())
	{
		CReaderCommander* pCommander = itr->second;
		if (pCommander && pCommander->m_bUse)
		{
			pCommander->SetTerminalTime(nReaderID, szSetTime.GetBuffer(0));
		}

		itr++;
	}

	::LeaveCriticalSection(&g_crCommander);
}

void CSCardManagerApp::_CheckReaderList()
{
	if (m_ADODB.m_pConn == NULL) {
		m_ADODB.Open();
	}

	_LoadListFetchTime();

	CTime dbTime;
	m_ADODB.GetServerTime(dbTime);

	CTime t(
		dbTime.GetYear(),
		dbTime.GetMonth(),
		dbTime.GetDay(),
		dbTime.GetHour(),
		dbTime.GetMinute(),
		dbTime.GetSecond());	
	CString strServerTime = t.Format(L"%Y-%m-%d %H:%M:%S");
	g_AgentWnd.SetDBServerTime(strServerTime);

	m_ADODB.GetServiceEnv();
	if (FALSE == m_ADODB.SetServiceAlive())
	{
		ATLTRACE(L"fail to SetServiceAlive\n");
		g_AgentWnd.AddLog(L"%s", m_ADODB.ErrMsg());
	}
	else
	{
		g_AgentWnd.SetDBLastCommand(L"SetServiceAlive");
		g_AgentWnd.AddLog(L"SetServiceAlive");
	}

	// DB의 reader기 list를 조회해서 현재 실행되고 있지 않은 새로운 ip면 추가 함.
	::EnterCriticalSection(&g_crCommander);

	m_vec_reader_info.clear();
	if (m_ADODB.GetCardReaderList(&m_vec_reader_info))
	{
		for (int i=0; i<m_vec_reader_info.size(); i++)
		{
			CString mapCommanderKey;
			mapCommanderKey.Format(L"%s:%d", m_vec_reader_info[i].strIP, m_vec_reader_info[i].nPort);

			MAP_COMMANDER::iterator itr = m_mapCommander.find(mapCommanderKey);
			if (itr == m_mapCommander.end())
			{
				CReaderCommander* pCommander = new CReaderCommander();
				if (pCommander)
				{
					int nReaderId = _tstoi(m_vec_reader_info[i].strReaderID);

					pCommander->SetSessionInfo(m_vec_reader_info[i].strIP, m_vec_reader_info[i].nPort);
					pCommander->SetCtrlName(m_vec_reader_info[i].strCtrlName);
					pCommander->AddTerminalID(nReaderId);

					CString mapCommanderKey;
					mapCommanderKey.Format(L"%s:%d", m_vec_reader_info[i].strIP, m_vec_reader_info[i].nPort);
					m_mapCommander[mapCommanderKey] = pCommander;

					::EnterCriticalSection(&m_csReaderTime);
					MAP_READER_TIME::iterator itrReaderTime = m_mapReaderTime.find(nReaderId);
					if (itrReaderTime != m_mapReaderTime.end())
					{
						pCommander->SetTerminalTime(itrReaderTime->first, itrReaderTime->second);

					}
					::LeaveCriticalSection(&m_csReaderTime);

					g_AgentWnd.AddCommanderToList(m_vec_reader_info[i].strIP, m_vec_reader_info[i].nPort);
					g_AgentWnd.UpdateCommanderInList(m_vec_reader_info[i].strIP, 
													 m_vec_reader_info[i].nPort, 
													 m_vec_reader_info[i].strCtrlName, 
													 L"",
													 L"");
				}
			}
			else
			{
				CReaderCommander* pCommander = itr->second;
				if (pCommander)
				{
					pCommander->SetCtrlName(m_vec_reader_info[i].strCtrlName);
					pCommander->AddTerminalID(_tstoi(m_vec_reader_info[i].strReaderID));
				}
			}
		}
	}

	::LeaveCriticalSection(&g_crCommander);

	g_AgentWnd.SetDBLastCommand(L"GetReaderList");
	g_AgentWnd.AddLog(L"GetReaderList OK");


	//////////////// map에는 있으나 db list에 없는 것은 제거 /////////////////
	Sleep(1000);
	
	::EnterCriticalSection(&g_crCommander);
	while (1)
	{
		MAP_COMMANDER::iterator itr = m_mapCommander.begin();
		while (itr != m_mapCommander.end())
		{	
			CReaderCommander* pCommander = itr->second;

			BOOL bFindUseCommander = FALSE;
			for (int i=0; i<m_vec_reader_info.size(); i++)
			{
				if (pCommander->GetReaderIP() == m_vec_reader_info[i].strIP &&
					pCommander->GetReaderPort() == m_vec_reader_info[i].nPort)
				{
					bFindUseCommander = TRUE;
				}
			}

			if (FALSE == bFindUseCommander)
			{
				g_AgentWnd.DeleteCommanderInList(pCommander->GetReaderIP(), pCommander->GetReaderPort());
				pCommander->CloseSession();
				bFindUseCommander = TRUE;

				m_mapCommander.erase(itr);

				break;
			}

			itr++;
		}

		if (itr == m_mapCommander.end()) break;
	}
	::LeaveCriticalSection(&g_crCommander);
}

unsigned int __stdcall CSCardManagerApp::MgrEnvThread(void* param)
{
	CSCardManagerApp* pApp = (CSCardManagerApp*)param;

	g_AgentWnd.AddSvrLog(L"----------- daemon start ------------------");

	while (1)
	{	
		DWORD dwIdx = WaitForSingleObject(pApp->m_hThreadEvent, pApp->m_dwListFetch);
		if (dwIdx == WAIT_OBJECT_0)
		{
			g_dwCmdPollingTime= 0;
			break;
		}
		if (0 == pApp->m_dwListFetch) pApp->m_dwListFetch = 5000;
		pApp->_CheckReaderList();

		pApp->_LoadListFetchTime();
	}
	CloseHandle(pApp->m_hThread);

	g_AgentWnd.AddSvrLog(L"----------- end MgrEnvThread  ------------------");

	return 0;
}

void CSCardManagerApp::RunDbCommand()
{
	// 리스트에 존재하는 converter이면 reader id들을 갱신
	::EnterCriticalSection(&g_crCommander);

	BOOL bFindCommander = FALSE;
	for (int i=0; i<m_vec_reader_info.size(); i++)
	{
		ST_READER_INFO *pstReaderInfo = &m_vec_reader_info[i];

		if (pstReaderInfo)
		{
			//ATLTRACE(L"get cmd %s\n", pstReaderInfo->strIP);
			Sleep(10);
			g_AgentWnd.SetDBLastCommand(L"GetSendCommand");
			VEC_SEND_CMD VecSendCmd;
			m_ADODB.GetSendCommand(pstReaderInfo->strIP, pstReaderInfo->nPort, &VecSendCmd);
			if (VecSendCmd.size() > 0)
			{
				g_AgentWnd.AddLog(L"Get SendCommand %s, %d", pstReaderInfo->strIP, pstReaderInfo->nPort);
			}

			CString mapCommanderKey;
			mapCommanderKey.Format(L"%s:%d", pstReaderInfo->strIP, pstReaderInfo->nPort);

			MAP_COMMANDER::iterator itr = m_mapCommander.find(mapCommanderKey);
			if (itr != m_mapCommander.end())
			{
				CReaderCommander* pCommander = itr->second;
				if (pCommander) {
					pCommander->StartJob(&VecSendCmd);
				}
			}
		}
	}

	::LeaveCriticalSection(&g_crCommander);
}

unsigned int __stdcall CSCardManagerApp::DBCommandCheckThread(void* param)
{
	CSCardManagerApp* pApp = (CSCardManagerApp*)param;

	while (1)
	{	
		DWORD dwIdx = WaitForSingleObject(pApp->m_hCmdChkThreadEvent, g_dwCmdPollingTime);
		if (dwIdx == WAIT_OBJECT_0)
		{
			break;
		}

		pApp->RunDbCommand();
	}
	CloseHandle(pApp->m_hCmdChkThread);

	return 0;
}

void CSCardManagerApp::SetProcessIDToCardService()
{
	//Named pipe를 통해 process id를 전달하여 감시할 수 있게 한다.
	HANDLE hSVC = OpenNamedPipeHandle(MONITOR_SVC_PIPENAME, 10000);
	if (hSVC == INVALID_HANDLE_VALUE)
	{
		return;
	}

	DWORD dwBytesWritten;
	SVC_CMD svcCmd;
	svcCmd.Cmd = CMD_SET_PID;
	svcCmd.dwPid = _getpid();

	TCHAR szModulePath[MAX_PATH];
	::GetModuleFileName(_Module.m_hInst, szModulePath, MAX_PATH);

	StringCbCopy(svcCmd.szManagerPath, sizeof(svcCmd.szManagerPath), szModulePath);
	BOOL bWrite = WriteFile(hSVC, &svcCmd, sizeof(svcCmd), &dwBytesWritten, 0);
	if (!bWrite)
	{
		CloseHandle(hSVC);
		return;
	}

	FlushFileBuffers(hSVC);

	DWORD dwRead;
	ZeroMemory(&svcCmd, sizeof(SVC_CMD));
	if (!ReadFile(hSVC, &svcCmd, sizeof(SVC_CMD), &dwRead, 0))
	{
		CloseHandle(hSVC);
		return;
	}

	CloseHandle(hSVC);
}

void CSCardManagerApp::StartDeamon(void)
{
	InitializeCriticalSection(&m_csADODB);
	m_ADODB.Open();

	unsigned thread_addr;
	m_hThread = (HANDLE)_beginthreadex(0, 0, MgrEnvThread, (void*)this, 0, &thread_addr);	
	if (m_hThread)
	{
		SetProcessIDToCardService();

		m_converterServer.InitServer();
	}

	unsigned thread_addr2;
	m_hCmdChkThread = (HANDLE)_beginthreadex(0, 0, DBCommandCheckThread, (void*)this, 0, &thread_addr2);
}

void CSCardManagerApp::StopCardService()
{
	HANDLE hSVC = OpenNamedPipeHandle(MONITOR_SVC_PIPENAME, 10000);
	if (hSVC == INVALID_HANDLE_VALUE)
	{
		return;
	}

	DWORD dwBytesWritten;
	SVC_CMD svcCmd;
	svcCmd.Cmd = CMD_END_PROCESS;
	svcCmd.dwPid = _getpid();
	svcCmd.szManagerPath[0] = L'\0';

	BOOL bWrite = WriteFile(hSVC, &svcCmd, sizeof(svcCmd), &dwBytesWritten, 0);
	if (!bWrite)
	{
		CloseHandle(hSVC);
		return;
	}

	FlushFileBuffers(hSVC);

	DWORD dwRead;
	ZeroMemory(&svcCmd, sizeof(SVC_CMD));
	if (!ReadFile(hSVC, &svcCmd, sizeof(SVC_CMD), &dwRead, 0))
	{
		CloseHandle(hSVC);
		return;
	}

	CloseHandle(hSVC);
}

void CSCardManagerApp::StopDaemon()
{
	StopCardService();

	_ClearConveterOBJ();

	SetEvent(m_hThreadEvent);
	WaitForSingleObject(m_hThread, 30000);

	SetEvent(m_hCmdChkThreadEvent);
	WaitForSingleObject(m_hCmdChkThread, 30000);

	g_AgentWnd.DestroyWindow();


	DeleteCriticalSection(&m_csADODB);

	PostQuitMessage(0);
}
