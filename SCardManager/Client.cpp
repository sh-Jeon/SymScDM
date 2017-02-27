// Client.cpp: implementation of the CClient class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Client.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destructionc
//////////////////////////////////////////////////////////////////////

CClient::CClient()
{
	opCode = 0;
	m_dwBufLen = MAX_BUFFER;

	m_bAccept = FALSE;
	m_hSocket = NULL;
	
	m_ReadDataBuffer = new BYTE[MAX_BUFFER];	
	ZeroMemory(m_ReadDataBuffer, MAX_BUFFER);

	m_WriteDataBuffer = new BYTE[MAX_BUFFER];	
	ZeroMemory(m_WriteDataBuffer, MAX_BUFFER);

	ZeroMemory(&readOverlapped, sizeof(WSAOVERLAPPED));
	ZeroMemory(&writeOverlapped, sizeof(WSAOVERLAPPED));

	wsaBuf.buf = (char*)m_ReadDataBuffer;
	wsaBuf.len = MAX_BUFFER;

	m_hSocket=socket(AF_INET,SOCK_STREAM,0);  //WSASocket(AF_INET,SOCK_STREAM,0,NULL,0,WSA_FLAG_OVERLAPPED); 
	if(m_hSocket==INVALID_SOCKET) 
	{
		//TRACE(0,"socket create error(we=%d)\n",WSAGetLastError());
		return;
	}

	int nZero=0;
	if(SOCKET_ERROR==setsockopt(m_hSocket,SOL_SOCKET,SO_RCVBUF,(const char*)&nZero,sizeof(int)))
	{
		ATLTRACE(L"Change Buffer Size Error, %d\n",WSAGetLastError());
		return;
	}
	nZero=0;
    if(SOCKET_ERROR==setsockopt(m_hSocket,SOL_SOCKET,SO_SNDBUF,(const char*)&nZero,sizeof(int)))
	{
		ATLTRACE(L"Change Buffer Size Error, %d\n",WSAGetLastError());
		return;
	}
}

void CClient::SetSocket(SOCKET socket)
{
	m_hSocket = socket;

	int nZero=0;
	if(SOCKET_ERROR==setsockopt(m_hSocket,SOL_SOCKET,SO_RCVBUF,(const char*)&nZero,sizeof(int)))
	{
		ATLTRACE(L"Change Buffer Size Error, %d\n",WSAGetLastError());
		return;
	}
	
	struct timeval tv;
	tv.tv_sec = 5; 
	setsockopt(m_hSocket, SOL_SOCKET, SO_RCVTIMEO,(const char*)&tv,sizeof(struct timeval));

	setsockopt(m_hSocket, SOL_SOCKET, SO_SNDTIMEO,(const char*)&tv,sizeof(struct timeval));

	nZero=0;
    if(SOCKET_ERROR==setsockopt(m_hSocket,SOL_SOCKET,SO_SNDBUF,(const char*)&nZero,sizeof(int)))
	{
		ATLTRACE(L"Change Buffer Size Error, %d\n",WSAGetLastError());
		return;
	}
}

CClient::~CClient()
{
	closesocket(m_hSocket);
	m_hSocket=INVALID_SOCKET;

	m_bAccept = FALSE;

	if (m_ReadDataBuffer) {
		delete [] m_ReadDataBuffer;
		m_ReadDataBuffer = NULL;
	}
	if (m_WriteDataBuffer) {
		delete [] m_WriteDataBuffer;	
		m_WriteDataBuffer = NULL;
	}
}

void CClient::SetDataBuffer(BYTE *data, DWORD len)
{
	if (len == 0){
		ZeroMemory(m_WriteDataBuffer, sizeof(char)*MAX_BUFFER);
	}
	else
	{
		if (len > MAX_BUFFER)
		{
			delete m_WriteDataBuffer;

			m_WriteDataBuffer = new BYTE[len];
		}

		memcpy(m_WriteDataBuffer, data, len);
	}

	wsaBuf.len = len;	
}

BOOL CClient::IsAccepted()
{
	return m_bAccept;
}
