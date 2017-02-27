#pragma once

#include "ADODB.h"

#include <vector>

#define MAKEDWORD(x,y) ((WORD)x | (DWORD)y<<16)

typedef enum _cmd_converter_tag_
{
	CMD_UNKNOWN = -1,
	CMD_EVENT_DATA = 0x01,
	CMD_STATUS_TERMINAL = 0x02,
	CMD_EVENT = 0x54    // ('T')
}CMD_READER;

typedef struct _recv_header_
{
	BYTE STX;
	BYTE Address[2];
	BYTE lenData[2];
	BYTE ACK[2];
}ST_RECV_HEADER;

typedef struct _resp_job_tag_
{
	ST_RECV_HEADER header;

	BYTE Command1;
	BYTE Command2;

	int nLenData;
	std::vector<BYTE> vecData;

	int nErrCode;
}ST_RESP_JOB;

typedef std::vector<BYTE> VEC_RECV_BUF;
typedef std::vector<BYTE> VEC_SEND_BUF;

class CClient;
class ConverterProtocol
{
public:
	ConverterProtocol(void);
	~ConverterProtocol(void);

	void _MakeCmdReqPacket(VEC_SEND_BUF *bufSend, int nTerminalID, LPCTSTR pData, int nLenData);

	char MakeLRC(char* pSendBuf, int nLen);
	BOOL ParseReceiveBuffer(char *bufRecv, int nLenData, BOOL isACK = TRUE, CClient *pClient = NULL);

	virtual void _ProcessResponse(BYTE Command1, BYTE Command2, ST_RECV_HEADER* pstRecvHeader, BOOL bValidation, BYTE* pRecvData, int nLenData, CClient *pClient=NULL) = 0;
};

