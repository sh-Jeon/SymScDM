#pragma once

#include <vector>
#include <map>
#include "ReaderCommander.h"
#include "ConverterServer.h"
#include "ADODB.h"

#define PLIST_INI _T("AppPLIST.ini")

typedef struct _reader_status_req_flag
{
	CString sFlagDoorSt;
	CString sFlagVer;
	CString sFlagTime;
	CString sFlagIdCnt;
	CString sFlagEventsCnt;
	CString sFlagTimeCode;
	CString sFlagDoorTime;
}ST_REQ_FLAG;
extern ST_REQ_FLAG g_stReqFlag;

typedef std::map<CString, CReaderCommander*> MAP_COMMANDER;
typedef std::map<int, CString> MAP_READER_TIME;

extern CString g_strGroupID;
extern DWORD g_dwCmdPollingTime;

class CSCardManagerApp
{
public:
	CSCardManagerApp(void);
	~CSCardManagerApp(void);

	void StartDeamon(void);
	void StopDaemon();

	DWORD GetListFetchTime()
	{
		return m_dwListFetch;
	}
	void SetTerminalTime(int nReaderID);
	void SetTerminalTime(int nReaderId, LPCTSTR strReaderTime);

protected:
	static unsigned int __stdcall MgrEnvThread(void* param); 
	static unsigned int __stdcall DBCommandCheckThread(void* param); 

private:
	CADODB m_ADODB;
	CRITICAL_SECTION m_csADODB;

	std::vector<ST_READER_INFO> m_vec_reader_info;  
	// 현재 실행되고 있는 commander 객체의 map
	MAP_COMMANDER m_mapCommander;
	MAP_READER_TIME m_mapReaderTime;
	CRITICAL_SECTION m_csReaderTime;

	HANDLE m_hThread;
	HANDLE m_hThreadEvent;

	HANDLE m_hCmdChkThread;
	HANDLE m_hCmdChkThreadEvent;

	DWORD m_dwListFetch;

	void _ClearConveterOBJ();
	void _CheckReaderList();
	void _LoadListFetchTime();
	void RunDbCommand();

	ConverterServer m_converterServer;

	// 모니터에 등록
	void SetProcessIDToCardService();
	// 모니터에 정상 종료를 알림
	void StopCardService();
};

extern CRITICAL_SECTION g_crCommander;
extern CSCardManagerApp _theApp;	
extern CString g_strRegPath;
