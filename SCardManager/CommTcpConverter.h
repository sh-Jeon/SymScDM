#pragma once

#include "sockcomm.h"
#include "ConverterProtocol.h"

#include <vector>

class CommTcpConverter :
	public CSockComm, public ConverterProtocol
{
public:
	CommTcpConverter(void);
	~CommTcpConverter(void);

	void SetSessionInfo(LPCTSTR szIP, int nPort);

protected:
	ST_RESP_JOB m_stCurrentJob;

	HANDLE m_hResponse;

	void SendRequest(int nTerminalID, CMD_READER nCmd, BYTE* pData=NULL, int nLenData=0);
	void SendCmdRequest(int nTerminalID, LPCTSTR pData, int nLenData, int nCmdIdx);
	virtual BOOL OnReceive(int nErrorCode);

	const char* _GetCommandString(CMD_READER cmd);

private:
	HANDLE m_hThread;
	HANDLE m_hEvent[2];

	CRITICAL_SECTION m_csResp;

	// Request
	void _MakeReqPacket(int nTerminalID, CMD_READER nCmd, BYTE* pData, int nLenData);

	//CMD_READER _GetCommandFromString(DWORD dwCmd);
	virtual void _ProcessResponse(BYTE Command1, BYTE Command2, ST_RECV_HEADER* pstRecvHeader, BOOL bValidation, BYTE* pRecvData, int nLenData, CClient *pClient=NULL);
	void _CommTimeOut();
};

