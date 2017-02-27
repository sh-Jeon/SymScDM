// SockComm.h: interface for the CSockComm class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SOCKCOMM_H__637658D0_37C6_4D51_B8F3_AE445FAFC11C__INCLUDED_)
#define AFX_SOCKCOMM_H__637658D0_37C6_4D51_B8F3_AE445FAFC11C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define RESP_TIMEOUT 1000 * 3

typedef enum _comm_error_tag_
{
	ERROR_SOCKET = -1,
	COMM_ERROR_SUCCESS = 0,
	COMM_ERROR_TIMEOUT,
	COMM_ERROR_LRC,
	ERROR_SHUTDOWN
}COMM_ERROR;

typedef enum _comm_control_cmd_
{
	CMD_CONNECT = 1,
	CMD_SEND
}COMM_CONTROL;

#include <winsock2.h>
#include <process.h>
#include <vector>

class CSockComm  
{
public:	
	DWORD m_cmd;
	BOOL IsConnected();

	static unsigned int __stdcall CommThreadProc(void *param);

	void NotifySockEvent(WSANETWORKEVENTS* pNetEvents);

	BOOL Init();
	int ConnectEx();

	CString GetReaderIP(){	return m_strServerIP;	}
	int GetReaderPort(){ return m_nSvrPort; }

	virtual void OnConnect(int ErrCode);
	virtual BOOL OnReceive(int nErrorCode) = 0;
	virtual void OnSend(int nErrorCode){}
	virtual void OnClose(int nErrorCode);	
	void Close();

	CSockComm();
	virtual ~CSockComm();

protected:
	int m_nErrCode;
	SOCKET m_hSock;
	BOOL m_bConnected;

	CString m_strServerIP;
	int m_nSvrPort;
	CString m_strCtrlName;

private:
	HANDLE m_hWaitConnect;

	HANDLE m_hThread;
	WSAEVENT hEventArray[2];

	void _TryConnect();
};

#endif // !defined(AFX_SOCKCOMM_H__637658D0_37C6_4D51_B8F3_AE445FAFC11C__INCLUDED_)
