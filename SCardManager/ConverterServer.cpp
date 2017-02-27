#include "StdAfx.h"
#include "ConverterServer.h"
#include "AgentWnd.h"
#include "SCardManagerApp.h"
#include <Mswsock.h>

#define DEF_PORT_SVCSVR	 20450
#define WAIT_TIMEOUT_INTERVAL 100
#define STOP_ACCEPT	2

ConverterServer::ConverterServer(void)
{
	nServerPort = DEF_PORT_SVCSVR;
	m_nMaxThreadCount = 5;
	m_hListenIOCP = NULL;
	m_hIOCompletionPort = NULL;
	m_phWorkerThreads = NULL;

	CTime t(1999, 1, 1, 0, 0, 0); 
	m_tDBTime = t;
	InitializeCriticalSection(&m_csClientList);
}


ConverterServer::~ConverterServer(void)
{
	CloseServer();

	DeleteCriticalSection(&m_csClientList);
}

int ConverterServer::ReserveAccept()
{
	int nClientCount = 0;
	EnterCriticalSection(&m_csClientList);

	CClient* pClient = new CClient;
	m_vClientContext.push_back(pClient);
	nClientCount = m_vClientContext.size();

	LeaveCriticalSection(&m_csClientList);

	//reserve accept
	WSAOVERLAPPED* pOL = pClient->GetReadOverlap();
	DWORD dwBytes;
	pOL->OffsetHigh = (DWORD)pClient;
	if(!AcceptEx(m_sockListen, pClient->GetSocketHandle(),
		pClient->GetReadDataBuffer(), 0,
		sizeof(struct sockaddr_in)+16, 
		sizeof(struct sockaddr_in)+16,
		&dwBytes, 
		pOL))
	{
		DWORD err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			g_AgentWnd.AddSvrLog(L"%s - %d", L"ReserveAccept 오류 발생", err);
			return -1;
		}
	}	

	return nClientCount;
}

BOOL ConverterServer::InitServer(void)
{
	m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	m_dbManager.Open();

	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) 
	{
		g_AgentWnd.AddSvrLog(L"%s", L"네트워크를 사용할 수 없습니다");
		return FALSE;
	}

	TCHAR szThreadCount[10];
	DWORD dwRet = GetPrivateProfileStringW(L"SERVER", L"ThreadPool", _T(""), szThreadCount, sizeof(szThreadCount), g_strRegPath);
	if (dwRet) m_nMaxThreadCount = _tstol(szThreadCount);

	m_sockListen = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if(m_sockListen == INVALID_SOCKET)
	{
		g_AgentWnd.AddSvrLog(L"%s", L"네트워크를 사용할 수 없습니다");
		return FALSE;
	}

	sockaddr_in local;
	local.sin_family=AF_INET;
	local.sin_addr.s_addr=INADDR_ANY;

	TCHAR szPort[10];
	dwRet = GetPrivateProfileStringW(L"SERVER", L"Port", _T(""), szPort, sizeof(szPort), g_strRegPath);
	if (dwRet) nServerPort = _tstol(szPort);
	local.sin_port = htons((u_short)nServerPort);
	if (bind(m_sockListen, (sockaddr*)&local, sizeof(local)) !=0 )
	{
		closesocket(m_sockListen);
		g_AgentWnd.AddSvrLog(L"%s", L"해당 Port를 사용할 수 없습니다");

		return FALSE;
	}

	if (listen(m_sockListen, SOMAXCONN) != ERROR_SUCCESS)
	{
		g_AgentWnd.AddSvrLog(L"%s", L"서버를 시작할 수 없습니다");
		return FALSE;
	}

	//Listen IOCP핸들 생성
	m_hListenIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,m_nMaxThreadCount);
    if(m_hListenIOCP == NULL)
	{        
		DWORD Errcode=GetLastError();
		if (Errcode != NULL){
			return FALSE; 
		}
	}

	m_hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, m_nMaxThreadCount);

	//Listen소켓과 IOCP핸들 연결
	CreateIoCompletionPort((HANDLE)m_sockListen, m_hListenIOCP, 0, m_nMaxThreadCount);

	m_dwTimeOut = RESP_TIMEOUT;
	TCHAR szTimeOut[10];
	if (GetPrivateProfileStringW(L"CONFIG", L"ConnTimeout", _T(""), szTimeOut, sizeof(szTimeOut), g_strRegPath))
	{
		m_dwTimeOut = _tstol(szTimeOut);
	}

	unsigned int nThreadID;
	m_phWorkerThreads = new HANDLE[m_nMaxThreadCount];
	m_phAcceptThreads = new HANDLE[m_nMaxThreadCount];
	for (int i = 0; i < m_nMaxThreadCount; i++)
	{
		m_phWorkerThreads[i] = (HANDLE)_beginthreadex(0, 0, ClientWorkerThread, this, 0, &nThreadID);
		m_phAcceptThreads[i] = (HANDLE)_beginthreadex(0, 0, AcceptThread, this, 0, &nThreadID);
	}

	ReserveAccept();

	return TRUE;
}

void ConverterServer::CleanClientList()
{
	EnterCriticalSection(&m_csClientList);

	VEC_CLIENT_CONTEXT::iterator IterClientContext;

	for (IterClientContext = m_vClientContext.begin(); IterClientContext != m_vClientContext.end( ); IterClientContext++)
	{
		//i/o will be cancelled and socket will be closed by destructor.
		delete *IterClientContext;
	}

	m_vClientContext.clear();

	LeaveCriticalSection(&m_csClientList);
}

void ConverterServer::CloseServer(void)
{
	EnterCriticalSection(&m_csClientList);
	m_dbManager.Close();
	LeaveCriticalSection(&m_csClientList);

	for(int i=0;i<m_nMaxThreadCount;i++)
	{
		BOOL ret=PostQueuedCompletionStatus(m_hListenIOCP, 2, STOP_ACCEPT, NULL); 
	}
	closesocket(m_sockListen);
	m_sockListen = INVALID_SOCKET;

	//Ask all threads to start shutting down
	SetEvent(m_hShutdownEvent);

	//Let Accept thread go down
	WaitForSingleObject(m_hAcceptThread, INFINITE);

	for (int i = 0; i < m_nMaxThreadCount; i++)
	{
		//Help threads get out of blocking - GetQueuedCompletionStatus()
		PostQueuedCompletionStatus(m_hIOCompletionPort, 0, (DWORD) NULL, NULL);
	}

	//Let Worker Threads shutdown
	WaitForMultipleObjects(m_nMaxThreadCount, m_phWorkerThreads, TRUE, INFINITE);

	//We are done with this event
	//WSACloseEvent(m_hAcceptEvent);

	CleanClientList();

	//Delete the Client List Critical Section.
	DeleteCriticalSection(&m_csClientList);

	if (m_hListenIOCP) CloseHandle(m_hListenIOCP);

	//Cleanup IOCP.
	CloseHandle(m_hIOCompletionPort);

	//Clean up the event.
	CloseHandle(m_hShutdownEvent);

	//Clean up memory allocated for the storage of thread handles
	delete [] m_phWorkerThreads;
	delete [] m_phAcceptThreads;

	//Cleanup Winsock
	WSACleanup();
}

unsigned int __stdcall ConverterServer::AcceptThread(void *param)
{
	USES_CONVERSION;

	ConverterServer* pConverterServer = (ConverterServer*)param;

	int ErrCode=0;
	int sockaddr_size = sizeof(SOCKADDR_IN);    
	DWORD dwIoBytes, dwKey;
	WSAOVERLAPPED* pOL;
	CClient* pClient = NULL;

	//SOCKET ListenSocket = pConverterServer->m_sockListen;
	//WSANETWORKEVENTS WSAEvents;

	//Accept thread will be around to look for accept event, until a Shutdown event is not Signaled.
	while(WAIT_OBJECT_0 != WaitForSingleObject(pConverterServer->m_hShutdownEvent, 0))
	{
		BOOL res = GetQueuedCompletionStatus(pConverterServer->m_hListenIOCP,
											 &dwIoBytes,
											 &dwKey,
											 reinterpret_cast<LPOVERLAPPED*>(&pOL),
											 INFINITE);
		if (res && dwKey == STOP_ACCEPT) {
			g_AgentWnd.AddSvrLog(L"AcceptThread exit - %d, %d", 
						res, dwKey);	
			break;	
		}

		pClient = (CClient*)pOL->OffsetHigh;
		if (0 == res)
		{
			if(pOL) 
			{
				pConverterServer->RemoveFromClientListAndFreeMemory(pClient);
				continue;										
			}
			else 
			{
				Sleep(0);
				continue;
			}
		}

		if (pClient){
			// IOCP 커널 객체와 연결	
			pClient->SetAccept();
			if (pConverterServer->AssociateWithIOCP(pClient))
			{
				// 초기 Recv 예약..
				pConverterServer->ReserveRead(pClient, OP_READ);
			}
			else
			{
				g_AgentWnd.AddSvrLog(L"Error occurred while executing CreateIoCompletionPort().");
				//Let's not work with this client
				pConverterServer->RemoveFromClientListAndFreeMemory(pClient);
			}
		}

		int nClientCount = pConverterServer->ReserveAccept();
		g_AgentWnd.SetClientCount(nClientCount);
	}

	return 0;
}

unsigned int __stdcall ConverterServer::ClientWorkerThread(void *param)
{
	ConverterServer *pConverter = (ConverterServer*)param;
	if (pConverter)
	{
		pConverter->Work();
	}

	g_AgentWnd.AddSvrLog(L"End ClientWorkerThread");

	return 0;
}

void ConverterServer::Work()
{
	OVERLAPPED       *pOverlapped = NULL;
	CClient			 *pClient = NULL;
	DWORD            dwBytesTransfered = 0;
	int nBytesRecv = 0;
	int nBytesSent = 0;
	DWORD             dwBytes = 0, dwFlags = 0;

	//Worker thread will be around to process requests, until a Shutdown event is not Signaled.
	while (WAIT_OBJECT_0 != WaitForSingleObject(m_hShutdownEvent, 0))
	{
		dwBytesTransfered = 0;
		BOOL bReturn = GetQueuedCompletionStatus(
			m_hIOCompletionPort,
			&dwBytesTransfered,
			(LPDWORD)&pClient,
			&pOverlapped,
			INFINITE);

		if (NULL == pClient)
		{
			//We are shutting down
			g_AgentWnd.AddSvrLog(L"NULL == pClient");
			break;
		}

		if ((FALSE == bReturn) || ((TRUE == bReturn) && (0 == dwBytesTransfered)))
		{
			// 클라이언트에서 연결 끊음
			RemoveFromClientListAndFreeMemory(pClient);
			continue;
		}
		
		DWORD opCode = pClient->opCode;
		if (opCode == OP_READ)
		{
			if (pClient->IsAccepted())
			{
				OnRecvCompelete(pClient, dwBytesTransfered);
			}
			else ReserveRead(pClient, OP_READ);
		}
		else if (opCode == OP_WRITE)
		{
			pClient->ResetSocketBuff();
			ReserveRead(pClient, OP_READ);
		}
		//else
		//{
		//	closesocket(pClient->GetSocket());
		//	//pClient->ResetSocketBuff();
		//}
	} // while
}

BOOL ConverterServer::OnRecvCompelete(CClient* pClient, DWORD dwBytesTransferred)
{
	// ACK 전송
	BOOL bProcessed = FALSE;
	//VEC_RECV_BUF bufRecv;
	BYTE* pDataBuffer = pClient->GetReadDataBuffer();
	//bufRecv.insert(bufRecv.end(), &pDataBuffer[0], &pDataBuffer[dwBytesTransferred]);
	
	if (ParseReceiveBuffer((char*)pDataBuffer, dwBytesTransferred, FALSE, pClient))
	{
		bProcessed = TRUE;
	}
	//bufRecv.clear();

	return bProcessed;
}

void ConverterServer::_ProcessResponse(BYTE Command1, BYTE Command2, ST_RECV_HEADER* pstRecvHeader, BOOL bValidation, BYTE* pRecvData, int nLenData, CClient *pClient)
{
	CString strReaderID;
	strReaderID.Format(L"%02x%02x", pstRecvHeader->Address[0], pstRecvHeader->Address[1]);

	CString strRevData;
	for (int i=0; i<nLenData; i++)
	{
		CString strTemp;
		strTemp.Format(L"%02x", *(pRecvData + i));
		strRevData.Append(strTemp, strTemp.GetLength());
	}

	CString strCommand;
	strCommand.Format(L"%02x%02x", Command1, Command2);
	g_AgentWnd.SetServerCommand(strCommand);

	if (Command1 != CMD_EVENT || Command2 != CMD_STATUS_TERMINAL){
		g_AgentWnd.AddSvrLog(L"[%s] Command : %s, data : %s", L"_ProcessResponse", strCommand, strRevData);
	}
	
	CString strACK;

	BOOL bSetDB = TRUE;

	TCHAR *stop;
	DWORD dec = _tcstoul(strReaderID, &stop, 16);

	CString strReadNo;
	strReadNo.Format(L"%d", dec);

	EnterCriticalSection(&m_csClientList);
	bSetDB = m_dbManager.CESetReaderEvent(strReadNo, strCommand, strRevData, bValidation);
	if (bSetDB){
		// ack data  -- 'AC or ER (2byte) + Command1(1byte), Command2(1byte)
		if (Command1 == CMD_EVENT && Command2 == CMD_STATUS_TERMINAL)
		{
			strACK.Format(L"%s0004%02x%02x%s", 
				strReaderID, (BYTE)(int)'A', (BYTE)(int)'C', strCommand);
		}
		else if (Command1 == CMD_EVENT && Command2 == CMD_EVENT_DATA)
		{
			CString strData;
			for (int i=0; i<3; i++)
			{
				CString strTemp;
				strTemp.Format(L"%02x", *(pRecvData + 6 + i));  // address + length + command 제외
				strData.Append(strTemp, strTemp.GetLength());
			}
 
			strACK.Format(L"%s0007%02x%02x%s%s", 
				strReaderID, (BYTE)(int)'A', (BYTE)(int)'C', strCommand, strData);

			g_AgentWnd.AddSvrLog(L"[%s][%s] %s", L"ConverterServer::prepare ACK", strCommand, strACK);
		}
	}
	else {
		strACK.Format(L"%s0004%02x%02x%s", 
				strReaderID, (BYTE)(int)'E', (BYTE)(int)'R', strCommand);
	}
	pClient->ResetSocketBuff();
	LeaveCriticalSection(&m_csClientList);

	if (Command1 == CMD_EVENT && Command2 == CMD_STATUS_TERMINAL)
	{
		//CString strBodyLen;
		//strBodyLen.Format(L"%02x%02x", pstRecvHeader->lenData[0], pstRecvHeader->lenData[1]);
		//DWORD bodyLen = _tcstoul(strBodyLen, &stop, 16);

		//int nTimePos = sizeof(ST_RECV_HEADER) - 2 + bodyLen - 18;

		//CString msg;
		//msg.Format(L"time recv body : %s, %d, pos - %d\n", strRevData, bodyLen, nTimePos);
		//g_AgentWnd.AddLog(msg);

		//CString readerTime = strRevData.Mid(nTimePos * 2, 14);
		//_theApp.SetTerminalTime(dec, readerTime);
		/*
		////////////////// 시간 체크
		if (m_dbManager.GetServerTime(m_tDBTime))
		{
			CTime t(
				m_tDBTime.GetYear(),
				m_tDBTime.GetMonth(),
				m_tDBTime.GetDay(),
				m_tDBTime.GetHour(),
				m_tDBTime.GetMinute(),
				m_tDBTime.GetSecond());	
			CString strServerTime = t.Format(L"%Y-%m-%d %H:%M:%S");
			g_AgentWnd.SetDBServerTime(strServerTime);

			CString strBodyLen;
			strBodyLen.Format(L"%02x%02x", pstRecvHeader->lenData[0], pstRecvHeader->lenData[1]);
			DWORD bodyLen = _tcstoul(strBodyLen, &stop, 16);

			int nTimePos = sizeof(ST_RECV_HEADER) - 2 + bodyLen - 18;

			CString msg;
			msg.Format(L"time recv body : %s, %d, pos - %d\n", strRevData, bodyLen, nTimePos);
			g_AgentWnd.AddLog(msg);

			CString readerTime = strRevData.Mid(nTimePos * 2, 14);
			//for (int i=0; i<7; i++)
			//{
			//	CString strTemp;
			//	strTemp.Format(L"%02x", *(pRecvData + nTimePos + i));
			//	readerTime.Append(strTemp, strTemp.GetLength());
			//}

			msg.Format(L"time recv time : %s\n", readerTime);
			g_AgentWnd.AddLog(msg);

			_AdjustReaderTime(dec, readerTime);
		}
		*/
		////////////////// 시간 체크
	}

	VEC_SEND_BUF bufSend;
	_MakeCmdReqPacket(&bufSend, _tstol(strReaderID), strACK, strACK.GetLength());

	pClient->SetDataBuffer(&(bufSend[0]), bufSend.size());				
	ReserveWrite(pClient, OP_WRITE);	

	bufSend.clear();
}

void ConverterServer::_AdjustReaderTime(int nReaderID, LPCTSTR szTime)
{
	if (m_tDBTime.GetYear() == 1999) return;

	BOOL bCheckTime=TRUE;

	CString strTime = szTime;

	int nParseTime=0;
	
	int nYear = _tstoi(strTime.Left(4));
	nParseTime += 4;
	
	int nMonth = _tstoi(strTime.Mid(nParseTime, 2));
	nParseTime += 2;

	int nDay = _tstoi(strTime.Mid(nParseTime, 2));
	nParseTime += 2;

	int nHour = _tstoi(strTime.Mid(nParseTime, 2));
	nParseTime += 2;

	int nMin = _tstoi(strTime.Mid(nParseTime, 2));
	nParseTime += 2;

	int nSec = _tstoi(strTime.Mid(nParseTime, 2));

	int nChkYear = _tstoi(m_tDBTime.Format(L"%Y"));
	if (nYear != nChkYear ||
		nMonth != m_tDBTime.GetMonth() ||
		nDay != m_tDBTime.GetDay() ||
		nHour != m_tDBTime.GetHour() ||
		nMin != m_tDBTime.GetMinute() ||
		nSec != m_tDBTime.GetSecond())
	{
		DWORD dwTimeDiff = 3000;
		TCHAR szTimeDiff[10];
		DWORD dwRet = GetPrivateProfileStringW(L"CONFIG", L"SetTimeDiff", _T(""), szTimeDiff, sizeof(szTimeDiff), g_strRegPath);
		if (dwRet) dwTimeDiff = _tstol(szTimeDiff);
		if (dwTimeDiff == 0) return;

		CString msg;
		msg.Format(L"terminal sec : %d, db sec : %d, %d\n", nSec, m_tDBTime.GetSecond(), dwTimeDiff / 1000);
		g_AgentWnd.AddLog(msg);

		if (abs(nSec - m_tDBTime.GetSecond()) < dwTimeDiff / 1000) return;
		// 3초 이상 차이날 경우 갱신 함.
		// OutputDebugString(L"SetTerminalTime\n");
		_theApp.SetTerminalTime(nReaderID);
		//g_AgentWnd.PostMessage(WM_APP+901, 0, (LPARAM)nReaderID);
	}
}

BOOL ConverterServer::ReserveWrite(CClient *pClient, int OpCode)
{
	DWORD dwSendBytes=0;
	DWORD dwFlags=0;

	WSAOVERLAPPED* pOL = pClient->GetWriteOverlap();
	ZeroMemory(pOL, sizeof(WSAOVERLAPPED));

	WSABUF* sockBuffer = pClient->GetSocketBuffer(OpCode);
	pClient->opCode = OpCode;

	int ret = WSASend(pClient->GetSocketHandle(),
			sockBuffer,1,
			&dwSendBytes, dwFlags, pOL, NULL);
	if(SOCKET_ERROR==ret)
	{
		int ErrCode=WSAGetLastError();
		if(ErrCode != WSA_IO_PENDING)
		{
			g_AgentWnd.AddSvrLog(L"Send Request Error(WSASend Function Failed): %d", ErrCode);
			RemoveFromClientListAndFreeMemory(pClient);
		}
	}

	return TRUE;
}

BOOL ConverterServer::ReserveRead(CClient *pClient, int OpCode)
{
	DWORD dwRecvBytes=0;
	DWORD dwFlags=0;

	WSAOVERLAPPED* pOL = pClient->GetReadOverlap();
	ZeroMemory(pOL, sizeof(WSAOVERLAPPED));

	WSABUF* sockBuffer = pClient->GetSocketBuffer(OpCode);
	sockBuffer->len = MAX_BUFFER;
	pClient->opCode = OpCode;

	if (SOCKET_ERROR == WSARecv(pClient->GetSocketHandle(), sockBuffer, 1,
									 &dwRecvBytes, &dwFlags, pOL, NULL))
	{
		int ErrCode=WSAGetLastError();
		if(ErrCode != WSA_IO_PENDING)
		{			
			ATLTRACE("[%d] Recv Request Error(WSARecv Function Failed): %d\n",GetTickCount(),ErrCode);
			RemoveFromClientListAndFreeMemory(pClient);

			return FALSE;					
		}
	}		

	return TRUE;
}

BOOL ConverterServer::AssociateWithIOCP(CClient *pClient)
{
	//Associate the socket with IOCP
	HANDLE hTemp = CreateIoCompletionPort((HANDLE)pClient->GetSocket(), m_hIOCompletionPort, (DWORD)pClient, 0);
	if (NULL == hTemp)
	{
		g_AgentWnd.AddSvrLog(L"Error occurred while executing CreateIoCompletionPort().");
		//Let's not work with this client
		RemoveFromClientListAndFreeMemory(pClient);

		return FALSE;
	}

	return TRUE;
}


//This function will allow to remove one single client out of the list
void ConverterServer::RemoveFromClientListAndFreeMemory(CClient *pClient)
{
	int nClientCount = 0;
	EnterCriticalSection(&m_csClientList);

	VEC_CLIENT_CONTEXT::iterator IterClient;

	//Remove the supplied ClientContext from the list and release the memory
	for (IterClient = m_vClientContext.begin(); IterClient != m_vClientContext.end(); IterClient++)
	{
		if (pClient == *IterClient)
		{
			//i/o will be cancelled and socket will be closed by destructor.
			if (pClient) 
			{
				delete pClient;
				pClient = NULL;
			}
			m_vClientContext.erase(IterClient);
			nClientCount = m_vClientContext.size();

			break;
		}
	}

	LeaveCriticalSection(&m_csClientList);

	g_AgentWnd.SetClientCount(nClientCount);
}
