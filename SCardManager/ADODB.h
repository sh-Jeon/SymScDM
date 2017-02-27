#pragma once

#include <comdef.h>
#include <atlcomtime.h>
#include <vector>

// ADO�� ���� ���Ͽ� dll�� import ������ 
#import ".\msado60.tlb" no_namespace rename ("EOF", "EndOfFile")

typedef struct _READER_STATUS_
{
	CString strReaderIP;
	CString strReaderID;
	CString strDoorStatus;		//�ܸ������
	CString strVersion;			//�ܸ������
	CString strReaderTime;		//�ܸ���ð�
	CString strRegIDCnt;		//���ī���ȣ��
	CString strEventsCnt;		//�ܸ����̺�Ʈ��
	CString TimeCodeCnt;		//�ܸ���TimeCode��
	CString DoorTime;			//���� �ð�
	CString ConnectStatus;		//�ܸ��⿬�����
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

	// Connection��ü ������  
	_ConnectionPtr	m_pConn;
 
	BOOL Open();
	void Close();
	CString ErrMsg();
	int IsConnect(void);

	CString			m_strDBIP;
	CString			m_strDBName;
	CString			m_strDBID;
	CString			m_strDBPass;
	CString			m_strErrorMessage;		// ���� ���� ���ڿ��� ���� ���� 

	// DB data ��ȸ
	void GetServiceEnv();
	BOOL GetServerTime(CTime &tDBTime);
	BOOL GetCardReaderList(std::vector<ST_READER_INFO> *pVecReaderList);
	BOOL GetSendCommand(LPCTSTR szReaderIP, int nPort, VEC_SEND_CMD* pVecSendCmd);

	////////////// �ܸ��� ��� ���
	BOOL CESetResultSendCmd(int cmdIndex, LPCTSTR szResult, LPCTSTR szRecvData, BOOL bValidation);
	BOOL CESetReaderEvent(LPCTSTR strReaderID, LPCTSTR szCmd, LPCTSTR szRecvData, BOOL bValidation);

public:
	BOOL SetServiceAlive(void);
};

