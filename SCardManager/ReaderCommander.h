#pragma once
#include "commtcpconverter.h"

#include <atltime.h>
#include <map>
#include "../Define.h"

typedef struct _send_gate_mod_tag
{
	int nReaderID;
	CString strStatus;
	CString strStartTime;
	CString strEndTime;
}ST_SEND_GATE_MODE;
typedef std::vector<ST_SEND_GATE_MODE> VEC_SEND_GATE_MODE;

typedef std::map<int, ST_RESULT_READER_STATUS> MAP_TERMINAL;

class CReaderCommander :
	public CommTcpConverter
{
public:
	CReaderCommander(void);
	~CReaderCommander(void);

	BOOL m_bEndJob;
	BOOL m_bUse;

	void StartJob(VEC_SEND_CMD* pVecSendCmd);

	int ConnectToTerminal();
	void CloseSession(void);

	void SetTerminalTime(int nReaderID, LPCTSTR szServerTime);

	void AddTerminalID(int nTerminalID)
	{	
		MAP_TERMINAL::iterator itr = m_vTerminalID.find(nTerminalID);
		if (itr == m_vTerminalID.end())
		{
			ATLTRACE(L"insert ReaderID %s - %d\n", GetReaderIP(), nTerminalID);

			ST_RESULT_READER_STATUS stRes_Status;
			stRes_Status.strDoorStatus = L"N/A";
			stRes_Status.strVersion = L"N/A";
			stRes_Status.strReaderTime = L"N/A";
			stRes_Status.strRegIDCnt = L"N/A";
			stRes_Status.strEventsCnt = L"N/A";
			stRes_Status.TimeCodeCnt = L"N/A";
			stRes_Status.DoorTime = L"N/A";
			stRes_Status.bNeedSendStatus = TRUE;

			m_vTerminalID[nTerminalID] = stRes_Status;	
		}
	}

	void SetCtrlName(LPCTSTR szCtrlName);

protected:
	CADODB m_dbManager;

	CRITICAL_SECTION m_csSendCmd;
	VEC_SEND_CMD m_VecSendCmd;

	HANDLE m_hTmThread;
	HANDLE m_hTmCmd;

	HANDLE m_hIdle;

	static unsigned int __stdcall TerminalCmdThread(void *param);
	void SendTerminalCmd();
	BOOL StartSendCommandJob(ST_SEND_CMD *pstSendCommand);
	void OnClose(int nErrorCode);

private:
	MAP_TERMINAL m_vTerminalID;
	CString m_strReaderTime;
};

