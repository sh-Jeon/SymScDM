#pragma once

#include "ConverterProtocol.h"
#include "Client.h"
#include <vector>

typedef std::vector<CClient*> VEC_CLIENT_CONTEXT;

class ConverterServer : public ConverterProtocol
{
public:
	ConverterServer(void);
	~ConverterServer(void);

	BOOL InitServer(void);

	void SetDBTime(CTime tDBTime){ 	m_tDBTime = tDBTime;	}

	BOOL AssociateWithIOCP(CClient *pClient);
	void RemoveFromClientListAndFreeMemory(CClient *pClient);

	BOOL ReserveRead(CClient *pClient, int OpCode);
	BOOL ReserveWrite(CClient *pClient, int OpCode);

	BOOL OnRecvCompelete(CClient* pClient, DWORD dwBytesTransferred);

	void Work();
	virtual void _ProcessResponse(BYTE Command1, BYTE Command2, ST_RECV_HEADER* pstRecvHeader, BOOL bValidation, BYTE* pRecvData, int nLenData, CClient *pClient=NULL);
	void _AdjustReaderTime(int nReaderID, LPCTSTR szTime);

	void CloseServer(void);
	void CleanClientList();

	int ReserveAccept();

	static unsigned int __stdcall AcceptThread(void *param);
	static unsigned int __stdcall ClientWorkerThread(void *param);

private:
	CADODB m_dbManager;

	int nServerPort;

	CTime m_tDBTime;

	DWORD m_dwTimeOut;

	VEC_CLIENT_CONTEXT m_vClientContext;
	CRITICAL_SECTION m_csClientList;

	HANDLE m_hListenIOCP;
	HANDLE m_hIOCompletionPort;

	HANDLE *m_phAcceptThreads;
	HANDLE *m_phWorkerThreads;
	HANDLE m_hShutdownEvent;

	SOCKET m_sockListen;
	//Network Event for Accept
	WSAEVENT m_hAcceptEvent;
	HANDLE m_hAcceptThread;

	int m_nMaxThreadCount;
};
