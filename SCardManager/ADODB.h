#pragma once

#include <comdef.h>
#include <atlcomtime.h>
#include <vector>

// ADO를 쓰기 위하여 dll을 import 시켜줌 
#import ".\msado60.tlb" no_namespace rename ("EOF", "EndOfFile")

typedef struct _READER_STATUS_
{
	CString strReaderIP;
	CString strReaderID;
	CString strDoorStatus;		//단말기운영모드
	CString strVersion;			//단말기버젼
	CString strReaderTime;		//단말기시간
	CString strRegIDCnt;		//등록카드번호수
	CString strEventsCnt;		//단말기이벤트수
	CString TimeCodeCnt;		//단말기TimeCode수
	CString DoorTime;			//도어 시간
	CString ConnectStatus;		//단말기연결상태
	BOOL bNeedSendStatus;
}ST_RESULT_READER_STATUS;

typedef struct _Reader_Info_
{
	CString strIP;
	int nPort;
	CString strCtrlName;
	CString strReaderID;
}ST_READER_INFO;

typedef struct _send_cmd_list_tag
{
	int nCmdIdx;
	int nReaderID;
	int nPort;
	CString strCmd;
}ST_SEND_CMD;
typedef std::vector<ST_SEND_CMD> VEC_SEND_CMD;

typedef std::vector<CString> VEC_READER_IP;

class CADODB
{
public:
	CADODB(void);
	~CADODB(void);

	// Connection개체 포인터  
	_ConnectionPtr	m_pConn;
 
	BOOL Open();
	void Close();
	CString ErrMsg();
	int IsConnect(void);

	CString			m_strDBIP;
	CString			m_strDBName;
	CString			m_strDBID;
	CString			m_strDBPass;
	CString			m_strErrorMessage;		// 에러 관련 문자열을 담을 변수 

	// DB data 조회
	void GetServiceEnv();
	BOOL GetServerTime(CTime &tDBTime);
	BOOL GetCardReaderList(std::vector<ST_READER_INFO> *pVecReaderList);
	BOOL GetSendCommand(LPCTSTR szReaderIP, int nPort, VEC_SEND_CMD* pVecSendCmd);

	////////////// 단말기 명령 결과
	BOOL CESetResultSendCmd(int cmdIndex, LPCTSTR szResult, LPCTSTR szRecvData, BOOL bValidation);
	BOOL CESetReaderEvent(LPCTSTR strReaderID, LPCTSTR szCmd, LPCTSTR szRecvData, BOOL bValidation);

public:
	BOOL SetServiceAlive(void);
};

