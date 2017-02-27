#include "StdAfx.h"
#include "ReaderCommander.h"
#include <strsafe.h>
#include <atltime.h>
#include "AgentWnd.h"
#include "SCardManagerApp.h"


CReaderCommander::CReaderCommander(void)
{
	m_bUse = TRUE;

	m_hResponse = CreateEvent( NULL, FALSE, FALSE, NULL );
	m_hIdle = CreateEvent(NULL, TRUE, FALSE, NULL);

	m_hTmThread = NULL;

	InitializeCriticalSectionAndSpinCount(&m_csSendCmd, 4000);
}

CReaderCommander::~CReaderCommander(void)
{
	//if (m_hTmThread)
	//{
	//	SetEvent(m_hTmCmd);
	//	WaitForSingleObject(m_hTmThread, g_dwCmdPollingTime + 3000);
	//}

	m_vTerminalID.clear();
	CloseHandle(m_hIdle);

	EnterCriticalSection(&m_csSendCmd);

	m_dbManager.Close();

	LeaveCriticalSection(&m_csSendCmd);

	::DeleteCriticalSection(&m_csSendCmd);
}

void CReaderCommander::SetCtrlName(LPCTSTR szCtrlName)
{
	m_strCtrlName = szCtrlName;
	g_AgentWnd.UpdateCommanderInList(m_strServerIP, m_nSvrPort, m_strCtrlName, L"N/A", L"Wait Command");
}

void CReaderCommander::StartJob(VEC_SEND_CMD* pVecSendCmd)
{
	::EnterCriticalSection(&m_csSendCmd);

	VEC_SEND_CMD::iterator itr;
	for (itr = pVecSendCmd->begin(); itr != pVecSendCmd->end(); itr++)
	{
		m_VecSendCmd.push_back(*itr);
	}

	if (m_hTmThread || m_VecSendCmd.size() == 0)
	{
		::LeaveCriticalSection(&m_csSendCmd);
		return;
	}

	m_dbManager.Open();

	m_bEndJob = FALSE;
	unsigned int thread_addr;
	m_hTmThread = (HANDLE)_beginthreadex(0, 0, TerminalCmdThread, (void*)this, 0, &thread_addr);

	::LeaveCriticalSection(&m_csSendCmd);
}

void CReaderCommander::CloseSession(void)
{
	m_bEndJob = TRUE;

	//g_dwCmdPollingTime = 30000;
	//WaitForSingleObject(m_hIdle, 30000);

	Close();

	if (m_hTmThread)
	{
		SetEvent(m_hTmCmd);
		WaitForSingleObject(m_hTmThread, 5000);
	}
	m_vTerminalID.clear();

	delete this;
}

unsigned int __stdcall CReaderCommander::TerminalCmdThread(void *param)
{
	CReaderCommander* pCommander = (CReaderCommander*)param;

	pCommander->SendTerminalCmd();

	if (pCommander->m_hTmThread)
	{
		CloseHandle(pCommander->m_hTmThread);
		pCommander->m_hTmThread = NULL;
	}

	return 0;

	//BOOL bNeedDelete = FALSE;
	//DWORD dwTickChkReaderList = GetTickCount(); 

	//while (1)
	//{
	//	SetEvent(pCommander->m_hIdle);
	//	DWORD dwIdx = WaitForSingleObject(pCommander->m_hTmCmd, g_dwCmdPollingTime);
	//	if (dwIdx == WAIT_OBJECT_0)
	//	{
	//		break;
	//	} else if (dwIdx == WAIT_TIMEOUT)
	//	{
	//		pCommander->SendTerminalCmd();
	//	}

	//	if (pCommander->m_bEndJob)
	//	{
	//		break;
	//	}
	//}

	//if (pCommander->m_hTmThread)
	//{
	//	CloseHandle(pCommander->m_hTmThread);
	//	pCommander->m_hTmThread = NULL;
	//}

	//return 0;
}

void CReaderCommander::OnClose(int nErrorCode)
{

}

int CReaderCommander::ConnectToTerminal()
{
	g_AgentWnd.AddLog(L"[%s] try connect to terminal ip : %s, %d", L"SendTerminalCmd", GetReaderIP(), GetReaderPort());
	ATLTRACE(L"[%s] try connect to terminal ip : %s, %d\n", L"SendTerminalCmd", GetReaderIP(), GetReaderPort());
	int nErr = ConnectEx();
	if (ERROR_SUCCESS != nErr)
	{	
		if (m_bEndJob)
		{
			Close();
			return ERROR_SHUTDOWN;
		}

		// timeout이면 그냥 리턴해야 됨.
		if (nErr == COMM_ERROR_TIMEOUT)
		{
			CString strLog;
			strLog.Format(L"컨버터 connect Timeout %s, %d", GetReaderIP(), GetReaderPort());
			if (g_AgentWnd.IsWindow())
			{
				g_AgentWnd.AddLog(L"%s", strLog);
				g_AgentWnd.OnConnectFail(GetReaderIP(), GetReaderPort());
				g_AgentWnd.UpdateCommanderInList(GetReaderIP(), m_nSvrPort, m_strCtrlName, L"Timeout", L"T");
			}
		}
		else
		{
			CString strErr;
			strErr.Format(L"%d", m_nErrCode);
			g_AgentWnd.UpdateCommanderInList(GetReaderIP(), m_nSvrPort, m_strCtrlName, L"DisConnected", strErr);
		}

		return nErr;
	}

	// Send DB Command
	if (FALSE == IsConnected())
	{
		CString strLog;
		strLog.Format(L"Send Command Skip 연결 끊어진 컨버터 %s", GetReaderIP());
		g_AgentWnd.AddLog(L"%s", strLog);

		return nErr;
	}

	g_AgentWnd.UpdateCommanderInList(GetReaderIP(), m_nSvrPort, m_strCtrlName, L"GetSendCommand", L"Connected");

	return nErr;
}

void CReaderCommander::SetTerminalTime(int nReaderID, LPCTSTR szServerTime)
{
	MAP_TERMINAL::iterator itr = m_vTerminalID.find(nReaderID);
	if (itr == m_vTerminalID.end())
	{
		return;
	}

	m_strReaderTime = szServerTime;
	return;

	////////// adjust time
	if (COMM_ERROR_SUCCESS != ConnectToTerminal())
	{
		if (m_nErrCode == COMM_ERROR_TIMEOUT)
		{
			CString strLog;
			strLog.Format(L"현재 시간 전송 Time out 발생 - %s, %d", m_strServerIP, GetReaderPort());
			g_AgentWnd.AddSvrLog(L"%s", strLog);

			if (g_AgentWnd.IsWindow())
			{
				g_AgentWnd.OnConnectFail(GetReaderIP(), GetReaderPort());
				g_AgentWnd.UpdateCommanderInList(GetReaderIP(), m_nSvrPort, m_strCtrlName, L"Timeout", L"T");
			}
		}
		else
		{
			CString strErr;
			strErr.Format(L"%d", m_nErrCode);
			g_AgentWnd.UpdateCommanderInList(GetReaderIP(), m_nSvrPort, m_strCtrlName, L"DisConnected", strErr);
		}
	}

	CString strReaderID;
	strReaderID.Format(L"%04x", nReaderID);

	CString strSendData;
	strSendData.Format(L"%s0009%02x01%s", strReaderID, (int)'S', szServerTime);

	g_AgentWnd.AddLog(L"현재 시간 동기화 전송 %s, %s, %d", strSendData, GetReaderIP(), GetReaderPort());

	SendCmdRequest(nReaderID, strSendData, strSendData.GetLength(), 0);
	// Send and WaitFor Timeout

	CString strCmd;
	CString strResult;
	strResult.Format(L"%02x%02x", m_stCurrentJob.header.ACK[0], m_stCurrentJob.header.ACK[1]);

	CString strRevData;
	if (m_stCurrentJob.vecData.size()){
		for (int i=0; i<m_stCurrentJob.vecData.size(); i++)
		{
			CString strTemp;
			strTemp.Format(L"%02x", m_stCurrentJob.vecData[i]);
			strRevData.Append(strTemp, strTemp.GetLength());
		}

		g_AgentWnd.AddSvrLog(L"[%s] recv data from terminal : %s", L"현재 시간 전송", strRevData);
	}
	m_stCurrentJob.vecData.clear();

	Close();
}

BOOL CReaderCommander::StartSendCommandJob(ST_SEND_CMD *pstSendCommand)
{
	USES_CONVERSION;
	CString strLog;

	if (COMM_ERROR_SUCCESS != ConnectToTerminal())
	{
		CString strData;
		if (m_nErrCode == COMM_ERROR_TIMEOUT)
		{
			strLog.Format(L"[idx: %d] Send DB Command Connect Time out 발생 - %s, %d", pstSendCommand->nCmdIdx, m_strServerIP, GetReaderPort());
			g_AgentWnd.AddLog(L"%s", strLog);

			if (g_AgentWnd.IsWindow())
			{
				g_AgentWnd.OnConnectFail(GetReaderIP(), GetReaderPort());
				g_AgentWnd.UpdateCommanderInList(GetReaderIP(), m_nSvrPort, m_strCtrlName, L"Timeout", L"T");
			}

			strData = L"TIMEOUT";
		}
		else
		{
			CString strErr;
			strErr.Format(L"%d", m_nErrCode);
			g_AgentWnd.UpdateCommanderInList(GetReaderIP(), m_nSvrPort, m_strCtrlName, L"DisConnected", strErr);

			strData = strErr;
		}

		CString strCmd;
		strCmd = L"ER";
			
		MAP_TERMINAL::iterator itrReaderSt = m_vTerminalID.find(pstSendCommand->nReaderID);
		if (itrReaderSt != m_vTerminalID.end())
		{
			itrReaderSt->second.bNeedSendStatus = TRUE;
		}

		EnterCriticalSection(&m_csSendCmd);
		
		m_dbManager.CESetResultSendCmd(pstSendCommand->nCmdIdx, strCmd.Left(2), strData, (COMM_ERROR_LRC != m_stCurrentJob.nErrCode));
		itrReaderSt->second.bNeedSendStatus = TRUE;
		g_AgentWnd.UpdateCommanderInList(GetReaderIP(), m_nSvrPort, m_strCtrlName, strCmd.Left(2), L"");

		Close();

		LeaveCriticalSection(&m_csSendCmd);

		return FALSE;
	}

	strLog.Format(L"[idx: %d] Socket Send Command to Reader %d, ip:%s, port:%d", pstSendCommand->nCmdIdx, pstSendCommand->nReaderID, GetReaderIP(), GetReaderPort());
	g_AgentWnd.AddLog(L"%s", strLog);

	SendCmdRequest(pstSendCommand->nReaderID, pstSendCommand->strCmd, pstSendCommand->strCmd.GetLength(), pstSendCommand->nCmdIdx);
	// Send and WaitFor Timeout

	CString strCmd;
	CString strResult;
	strResult.Format(L"%02x%02x", m_stCurrentJob.header.ACK[0], m_stCurrentJob.header.ACK[1]);

	CString strRevData;
	if (m_stCurrentJob.vecData.size()){
		for (int i=0; i<m_stCurrentJob.vecData.size(); i++)
		{
			CString strTemp;
			strTemp.Format(L"%02x", m_stCurrentJob.vecData[i]);
			strRevData.Append(strTemp, strTemp.GetLength());
		}

		g_AgentWnd.AddLog(L"[idx: %d] [%s] response data from terminal : %s", pstSendCommand->nCmdIdx, L"_SendDBCommand", strRevData);
	}

	EnterCriticalSection(&m_csSendCmd);

	m_dbManager.CESetResultSendCmd(pstSendCommand->nCmdIdx, strResult, strRevData, (COMM_ERROR_LRC != m_stCurrentJob.nErrCode));

	LeaveCriticalSection(&m_csSendCmd);

	return TRUE;
}

void CReaderCommander::SendTerminalCmd()
{		
	::EnterCriticalSection(&m_csSendCmd);

	for (VEC_SEND_CMD::iterator itr = m_VecSendCmd.begin(); itr != m_VecSendCmd.end(); itr++)
	{
		if (m_bEndJob) break;

		ST_SEND_CMD stCommand;
		if (itr == m_VecSendCmd.end())
		{
			break;
		}

		stCommand.nCmdIdx = itr->nCmdIdx;
		stCommand.nPort = itr->nPort;
		stCommand.nReaderID = itr->nReaderID;
		stCommand.strCmd = itr->strCmd;

		StartSendCommandJob(&stCommand);
	}
	m_VecSendCmd.clear();

	::LeaveCriticalSection(&m_csSendCmd);

	Close();
	//g_AgentWnd.UpdateCommanderInList(GetReaderIP(), m_nSvrPort, m_strCtrlName, L"DisConnected", L"Wait Command");
}