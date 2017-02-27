// SockComm.cpp: implementation of the CSockComm class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
//#include "PMLSock.h"

#include "SockComm.h"
#include "AgentWnd.h"
#include "SCardManagerApp.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
BOOL setsockaddr_in(struct sockaddr_in *psai, TCHAR* pcszurl, DWORD nport)
{
	USES_CONVERSION;

	LPHOSTENT phe;
	if(psai==NULL) return FALSE;

	ZeroMemory(psai,sizeof(*psai));
	psai->sin_family=AF_INET;
	psai->sin_port=htons((unsigned short)nport);
	if(pcszurl!=NULL) 
	{
		psai->sin_addr.s_addr = inet_addr(T2A(pcszurl));
		if(psai->sin_addr.s_addr==INADDR_NONE) {
			phe=gethostbyname(T2A(pcszurl));
			if(phe!=NULL)
				psai->sin_addr.s_addr=((LPIN_ADDR)phe->h_addr)->s_addr;
			else{
				int wserr = WSAGetLastError();
				if (wserr == WSANOTINITIALISED) ATLTRACE("WSANOTINITIALISED");
				if (wserr == WSAENETDOWN) ATLTRACE("WSAENETDOWN");
				if (wserr == WSAHOST_NOT_FOUND) ATLTRACE("WSAHOST_NOT_FOUND");
				if (wserr == WSATRY_AGAIN) ATLTRACE("WSATRY_AGAIN");
				if (wserr == WSANO_RECOVERY) ATLTRACE("WSANO_RECOVERY");
				if (wserr == WSANO_DATA) ATLTRACE("WSANO_DATA");
				if (wserr == WSAEINPROGRESS) ATLTRACE("WSAEINPROGRESS");
				if (wserr == WSAEFAULT) ATLTRACE("WSAEFAULT");
				if (wserr == WSAEINTR) ATLTRACE("WSAEINTR");

				return FALSE;
			}
		}
	}
	else {
		psai->sin_addr.s_addr=INADDR_ANY;
	}

	return TRUE;
}

CSockComm::CSockComm()
{
	m_hSock = INVALID_SOCKET;
	m_bConnected = FALSE;
	m_hThread = NULL;
	m_cmd = ERROR_SOCKET;

	m_hWaitConnect = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CSockComm::~CSockComm()
{
	SetEvent(m_hWaitConnect);
	if (m_hThread)
	{
		WSASetEvent(hEventArray[0]);  //end CommThreadProc
		WaitForSingleObject(m_hThread, 3000);

		for (int i=0;i<2;i++){
			WSACloseEvent(hEventArray[i]);
			hEventArray[i] = NULL;
		}	
	}
	CloseHandle(m_hWaitConnect);
}

BOOL CSockComm::Init()
{
	if (m_bConnected) return TRUE;

	m_hSock = INVALID_SOCKET;
	m_hSock = socket(AF_INET, SOCK_STREAM, 0);
	if (m_hSock == INVALID_SOCKET) return FALSE;

	if (m_hThread == NULL)
	{				
		hEventArray[0] = WSACreateEvent();
		WSAResetEvent(hEventArray[0]);    //To End Thread
		hEventArray[1] = WSACreateEvent();
		WSAResetEvent(hEventArray[1]);    		

		ATLTRACE("start commThread\n");
		unsigned int thread_addr;
		m_hThread = (HANDLE)_beginthreadex(0,0,CommThreadProc,this,0,&thread_addr);

		WSAEventSelect(m_hSock, hEventArray[1], FD_CONNECT | FD_READ | FD_CLOSE);
	}

	return TRUE;
}

void CSockComm::Close()
{
	if (m_hSock != INVALID_SOCKET)
	{		
		ATLTRACE(L"close socket\n");
		closesocket(m_hSock);
		m_hSock = INVALID_SOCKET;
	}
	m_bConnected = FALSE;

	if (m_hThread)
	{
		WSASetEvent(hEventArray[0]);
		WaitForSingleObject(m_hThread, 3000);
		m_hThread = NULL;
	}
}

void CSockComm::_TryConnect()
{
	sockaddr_in psai;
	if(!setsockaddr_in(&psai, m_strServerIP.GetBuffer(0), m_nSvrPort))
	{
		m_cmd = ERROR_SOCKET;
		OnConnect(ERROR_SOCKET);
		ATLTRACE("return conntthread fail\n");

		return;
	}

	ATLTRACE("try connect\n");
	u_long iMode = 1;
	ioctlsocket(m_hSock, FIONBIO, &iMode);

	connect(m_hSock, (struct sockaddr*)&psai, sizeof(struct sockaddr_in));
}

unsigned int __stdcall CSockComm::CommThreadProc(void *param)
{
	CSockComm* pSockComm = (CSockComm*)param;
	
	WSANETWORKEVENTS netEvents;
	
	while(1)
	{		
		DWORD index = WSAWaitForMultipleEvents(2, pSockComm->hEventArray, FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_EVENT_0) break;
		index = index - WSA_WAIT_EVENT_0;
		if (index == WSA_WAIT_FAILED) continue;	
		if (index == 1)
		{
			WSAEnumNetworkEvents(pSockComm->m_hSock, pSockComm->hEventArray[1], &netEvents);
			pSockComm->NotifySockEvent(&netEvents);
		}		
	}
	CloseHandle(pSockComm->m_hThread);
	pSockComm->m_hThread = NULL;
	ATLTRACE("exit CommThreadProc\n");

	return 0;
}

int CSockComm::ConnectEx()
{
	if (m_bConnected) return COMM_ERROR_SUCCESS;

	Init();
	
	m_cmd = 0;

	DWORD dwTimeOut = RESP_TIMEOUT;
	TCHAR szTimeOut[10];
	DWORD dwRet = GetPrivateProfileStringW(L"CONFIG", L"ConnTimeout", _T(""), szTimeOut, sizeof(szTimeOut), g_strRegPath);
	if (dwRet) dwTimeOut = _tstol(szTimeOut);

	_TryConnect();
	DWORD dwIdx = WaitForSingleObject(m_hWaitConnect, dwTimeOut);
	if (dwIdx == WAIT_TIMEOUT)
	{
		Close();
		m_nErrCode = COMM_ERROR_TIMEOUT;
		return COMM_ERROR_TIMEOUT;
	}

	return m_nErrCode;
}

void CSockComm::OnConnect(int ErrCode)
{
	m_nErrCode = ErrCode;

	if (ErrCode == ERROR_SUCCESS)
	{
		m_cmd = 0;
		m_bConnected = TRUE;
		ATLTRACE("return conntthread ok\n");
	}

	SetEvent(m_hWaitConnect);
}

void CSockComm::NotifySockEvent(WSANETWORKEVENTS *pNetEvents)
{
	if (pNetEvents->lNetworkEvents & FD_CONNECT){		
		OnConnect(pNetEvents->iErrorCode[FD_CONNECT_BIT]);		
	}
	if (pNetEvents->lNetworkEvents & FD_CLOSE){
		OnClose(pNetEvents->iErrorCode[FD_CLOSE_BIT]);	
		m_bConnected = FALSE;
	}			
	if (pNetEvents->lNetworkEvents & FD_READ){
		OnReceive(pNetEvents->iErrorCode[FD_READ_BIT]);
	}			
}

BOOL CSockComm::IsConnected()
{
	return m_bConnected;
}

void CSockComm::OnClose(int nErrorCode)
{
	CString strErr;
	strErr.Format(L"%d", nErrorCode);
	//g_AgentWnd.UpdateCommanderInList(m_strServerIP, m_nSvrPort, L"N/A", L"DisConnected", strErr);

	m_nErrCode = nErrorCode;

	if (m_hThread)
	{
		WSASetEvent(hEventArray[0]);  //end CommThreadProc
		WaitForSingleObject(m_hThread, 3000);

		for (int i=0;i<2;i++){
			WSACloseEvent(hEventArray[i]);
			hEventArray[i] = NULL;
		}	
	}
}