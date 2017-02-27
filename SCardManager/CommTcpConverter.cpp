#include "StdAfx.h"
#include "CommTcpConverter.h"
#include "AgentWnd.h"
#include <strsafe.h>
#include "SCardManagerApp.h"



//const char* CommTcpConverter::_GetCommandString(CMD_READER cmd)
//{
//	for(int i=0; i < sizeof(_cmd_name) / sizeof(_cmd_name[0]); i++)
//	{
//		if( _cmd_name[i].nCmd == cmd )
//		{
//			return _cmd_name[i].strCmd;
//		}
//	}
//
//	return "Unknown Command";
//}
//
//CMD_READER CommTcpConverter::_GetCommandFromString(DWORD dwCmd)
//{
//	USES_CONVERSION;
//	for(int i=0; i < sizeof(_cmd_name) / sizeof(_cmd_name[0]); i++)
//	{
//		if(dwCmd == _cmd_name[i].nCmd)
//		{
//			return _cmd_name[i].nCmd;
//		}
//	}
//
//	return CMD_UNKNOWN;
//}

CommTcpConverter::CommTcpConverter(void)
{
	InitializeCriticalSectionAndSpinCount(&m_csResp, 4000);  

	m_nErrCode = ERROR_SUCCESS;

	m_hEvent[0] = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hEvent[1] = CreateEvent(NULL, TRUE, FALSE, NULL);	

	ZeroMemory(&m_stCurrentJob, sizeof(ST_RECV_HEADER));
}

CommTcpConverter::~CommTcpConverter(void)
{
	SetEvent(m_hEvent[0]);
	WaitForSingleObject(m_hThread, 5000);

	Close();

	DeleteCriticalSection(&m_csResp);
}

void CommTcpConverter::SetSessionInfo(LPCTSTR szIP, int nPort)
{
	m_strServerIP = szIP;
	m_nSvrPort = nPort;
}

//STX
//R (1byte)
//ReaderID  (2byte)
//Length (command + data 길이  3byte)
//Command (/RV  3byte)
//Data (n byte)
//ETX (1byte)
//LRC (R ~ Data까지, 1byte)
//void CommTcpConverter::_MakeReqPacket(int nTerminalID, CMD_READER nCmd, BYTE* pData, int nLenData)
//{
//	BYTE STX = 0x02, ETX = 0x03;  //(1byte)
//
//	std::vector<char> vSendData;
//	vSendData.push_back('R');
//
//	char szReaderID[3];
//	StringCchPrintfA(szReaderID, _countof(szReaderID), "%02d", nTerminalID);
//	vSendData.insert(vSendData.end(), &szReaderID[0], &szReaderID[2]);
//
//	char szLenData[4];
//	StringCchPrintfA(szLenData, _countof(szLenData), "%03d", nLenData + 3);
//	vSendData.insert(vSendData.end(), &szLenData[0], &szLenData[3]);
//
//	// command1
//	
//
//	// command2
//	const char* Command = _GetCommandString(nCmd);
//	vSendData.insert(vSendData.end(), &Command[0], &Command[3]);
//
//	//data
//	if (pData)
//	{
//		vSendData.insert(vSendData.end(), &pData[0], &pData[nLenData]);
//	}
//
//	char LRC = MakeLRC((char*)&vSendData[0], vSendData.size());
//
//	///////////////// make send data /////////////////////////////////////
//	m_bufSend.push_back(STX);
//
//	for (int i=0; i<vSendData.size(); i++)
//	{
//		m_bufSend.push_back(vSendData[i]);
//	}
//
//	m_bufSend.push_back(LRC);
//	m_bufSend.push_back(ETX);
//}

void CommTcpConverter::SendCmdRequest(int nTerminalID, LPCTSTR pData, int nLenData, int nCmdIdx)
{
	USES_CONVERSION;

	if (m_stCurrentJob.vecData.size())
	{
		m_stCurrentJob.vecData.clear();
	}
	ZeroMemory(&m_stCurrentJob, sizeof(ST_RECV_HEADER));

	g_AgentWnd.AddLog(L"[%s][idx:%d] Try Send to Terminal %d bytes : %s", L"SendCmdRequest", nCmdIdx, nLenData, pData);

	VEC_SEND_BUF bufSend;
	_MakeCmdReqPacket(&bufSend, nTerminalID, pData, nLenData);
	
	int nTotalSent = 0;
	int nDataLen = bufSend.size();
	while (nTotalSent < bufSend.size())
	{
		int nSent= send(m_hSock, (char*)&bufSend[nTotalSent], nDataLen - nTotalSent, 0);
		nTotalSent += nSent;

		if (nSent == 0) break;
	}

	m_nErrCode = GetLastError();

	bufSend.clear();

	DWORD dwTimeOut = RESP_TIMEOUT;
	TCHAR szTimeOut[10];
	DWORD dwRet = GetPrivateProfileStringW(L"CONFIG", L"ConnTimeout", _T(""), szTimeOut, sizeof(szTimeOut), g_strRegPath);
	if (dwRet) dwTimeOut = _tstol(szTimeOut);

	g_AgentWnd.AddLog(L"[%s][idx: %d] Send OK %d, wait for timeout - %d", L"SendCmdRequest", nCmdIdx, nTotalSent, dwTimeOut);
	if (WAIT_TIMEOUT == WaitForSingleObject(m_hResponse, dwTimeOut))
	{
		g_AgentWnd.AddLog(L"[%s][idx: %d] wait timeout 발생 %d", L"SendCmdRequest", nCmdIdx, nTotalSent);
		m_nErrCode = COMM_ERROR_TIMEOUT;
	}
}

void CommTcpConverter::_ProcessResponse(BYTE Command1, BYTE Command2, ST_RECV_HEADER* pstRecvHeader, BOOL bValidation, BYTE* pRecvData, int nLenData, CClient *pClient)
{
	::EnterCriticalSection(&m_csResp);

	ZeroMemory(&m_stCurrentJob, sizeof(ST_RECV_HEADER));
	memcpy(&m_stCurrentJob, pstRecvHeader, sizeof(ST_RECV_HEADER));

	m_stCurrentJob.nLenData = nLenData;
	m_stCurrentJob.Command1 = Command1;
	m_stCurrentJob.Command2 = Command2;
	m_stCurrentJob.nErrCode = COMM_ERROR_SUCCESS;

	if (nLenData > 0)
	{
		m_stCurrentJob.vecData.insert(m_stCurrentJob.vecData.end(), pRecvData, pRecvData+nLenData);
	}

	if (bValidation)
	{
		m_stCurrentJob.nErrCode = ERROR_SUCCESS;
	}
	else
	{
		m_stCurrentJob.nErrCode = COMM_ERROR_LRC;
	}

	SetEvent(m_hResponse);

	::LeaveCriticalSection(&m_csResp);
}

// STX   0x02  1 byte
// 'P’  0x50  1 byte
// Reader ID ‘01 ~ 99’2 byte
// len   3byte
// command   '/'+2byte

// data
// ETX  1byte
// LRC  1byte
//('0x2 P 01 009 /RV 090506 0x03 I')
BOOL CommTcpConverter::OnReceive(int nErrorCode) 
{
	if (FALSE == m_bConnected) return FALSE;
	if (nErrorCode != COMM_ERROR_SUCCESS)
	{
		return FALSE;
	}

	BYTE buf[MAX_BUFFER];
	int nRecv;
	while(1)
	{
		nRecv = recv(m_hSock, (char*)buf, sizeof(buf), 0);
		if (FALSE == m_bConnected) return FALSE;
		if( nRecv == 0 )
		{
			Close();
			return FALSE;
		}

		if (nRecv == SOCKET_ERROR)
		{
			if (GetLastError() != WSAEWOULDBLOCK)
			{
				Close();
			}

			return FALSE;
		}

		if (ParseReceiveBuffer((char*)&(buf[0]), nRecv))
		{
			return TRUE;
		}
	}
}
