#include "StdAfx.h"
#include "ConverterProtocol.h"
#include "AgentWnd.h"
#include "Client.h"

ConverterProtocol::ConverterProtocol(void)
{
}


ConverterProtocol::~ConverterProtocol(void)
{
}

void ConverterProtocol::_MakeCmdReqPacket(VEC_SEND_BUF *bufSend, int nTerminalID, LPCTSTR pData, int nLenData)
{
	BYTE STX = 0x02, ETX = 0x03;  //(1byte)

	BYTE* transByteData = new BYTE[nLenData];
	int transDataLen = 0;
	for (int i=0; i<nLenData; i++)
	{
		if (i % 2 == 0){
			TCHAR transData[2];
			
			transData[0] = *(pData + i);
			transData[1] = *(pData + i + 1);

			TCHAR *stop;
			DWORD hexa = _tcstol(transData, &stop, 16);
			transByteData[transDataLen++] = hexa;
		}
	}

	char LRC = MakeLRC((char*)transByteData, transDataLen);

	bufSend->push_back(STX);
	for (int i=0; i<transDataLen; i++)
	{
		bufSend->push_back(*(transByteData + i));
	}
	bufSend->push_back(LRC);
	bufSend->push_back(ETX);

	int nSendDataLen = bufSend->size();
	int *sendlog = new int[nSendDataLen];

	CString strDexStream;
	for (int i=0;i<nSendDataLen; i++)
	{
		CString strTemp;
		strTemp.Format(L"%d", (*bufSend)[i]);
		strDexStream += strTemp;
	}

	delete transByteData;
}

char ConverterProtocol::MakeLRC(char* pSendBuf, int nLen)
{
	char ch = *pSendBuf;

	int nLRC = ch;

	for (int i=1; i < nLen; i++)
	{
		nLRC ^= *(pSendBuf + i);
	}

	return nLRC;
}

BOOL ConverterProtocol::ParseReceiveBuffer(char *bufRecv, int nLenData, BOOL isACK, CClient *pClient)
{
	// Header Size : 9 byte
	// data : datalen + 2byte
	ST_RECV_HEADER stRecvHeader;
	int nSizeHeader = sizeof(ST_RECV_HEADER);
	if (FALSE == isACK) nSizeHeader -= 2; //ACK 패킷이 아닌 경우 ACK길이를 제외함.

	ZeroMemory(&stRecvHeader, nSizeHeader);
	void *pRecv = bufRecv;
	memcpy(&stRecvHeader, pRecv, nSizeHeader);

	if (nLenData < nSizeHeader)  //ack 제외 
	{
		return FALSE; //헤더 도착하지 않음
	}

    int nBodyLen = 0;
    nBodyLen |= stRecvHeader.lenData[0] & 0xFF;
    nBodyLen <<= 8;
    nBodyLen |= stRecvHeader.lenData[1] & 0xFF;

	if (nLenData < nSizeHeader + nBodyLen)
	{
		return FALSE;  //body 도착하지 않음.
	}

	if (stRecvHeader.STX != 0x02) return FALSE;

	BOOL bACK = FALSE;
	if (isACK){
		if (stRecvHeader.ACK[0] == 'A' && stRecvHeader.ACK[1] == 'C'){
			bACK = TRUE; //error
		}
	}

	if (isACK && bACK == FALSE){
		// send error to db
		g_AgentWnd.AddLog(L"[%s] ACK Error : %c, %c", L"ParseReceiveBuffer", stRecvHeader.ACK[0], stRecvHeader.ACK[1]);
	}

	char Command1 = *((BYTE*)pRecv + nSizeHeader);
	char Command2 = *((BYTE*)pRecv + nSizeHeader+1);

	CString strCommand;
	strCommand.Format(L"%02x%02x", Command1, Command2);

	
	CString strRevData;
	if (strCommand.CompareNoCase(L"5402"))
	{
		for (int i=0; i<nLenData; i++)
		{
			CString strTemp;
			strTemp.Format(L"%02x", *((BYTE*)pRecv+i));
			strRevData.Append(strTemp, strTemp.GetLength());
		}
		if (isACK) g_AgentWnd.AddLog(L"response data : %s, %d", strRevData, nLenData);
		else g_AgentWnd.AddSvrLog(L"response data : %s, %d", strRevData, nLenData);
	}

	// validation check
	BOOL bCheckLRC = FALSE;	
	//std::vector<char> vSendData;

	//// Address
	//vSendData.push_back(stRecvHeader.Address[0]);
	//vSendData.push_back(stRecvHeader.Address[1]);
	//// Length
	//vSendData.push_back(stRecvHeader.lenData[0]);
	//vSendData.push_back(stRecvHeader.lenData[1]);

	//if (isACK){
	//	// ACK
	//	vSendData.push_back(stRecvHeader.ACK[0]);
	//	vSendData.push_back(stRecvHeader.ACK[1]);
	//}

	//int nCheckLRC = nSizeHeader + nBodyLen;
	//if (isACK) nCheckLRC -= 2;   //nBodyLen에 ACK길이가 추가되어 있음.
	//if (nLenData > 0)
	//{
	//	vSendData.insert(vSendData.end(), (BYTE*)pRecv + nSizeHeader, (BYTE*)pRecv + nCheckLRC);
	//}

	//char LRC = MakeLRC((char*)&vSendData[0], vSendData.size());
	int nChkLRC = (nSizeHeader-1) + nBodyLen;
	if (isACK) nChkLRC -= 2; 
	char LRC = MakeLRC(bufRecv + 1, nChkLRC);
	char RecvLRC = bufRecv[nLenData - 2];
	bCheckLRC = (LRC == RecvLRC);

	if (strCommand.CompareNoCase(L"5402"))
	{
		if (isACK)
		{
			g_AgentWnd.AddLog(L"LRC Check - len:%d, %02x~%02x, recvLRC: %02x, make LRC: %02x", nChkLRC, *((BYTE*)pRecv+nSizeHeader), *((BYTE*)pRecv + nChkLRC), RecvLRC, LRC);
		}
	}

	// SP_ResultSendCmd
	//_ProcessResponse(Command1, Command2, &stRecvHeader, bCheckLRC, (BYTE*)vSendData.data(), vSendData.size(), pClient);
	_ProcessResponse(Command1, Command2, &stRecvHeader, bCheckLRC, (BYTE*)bufRecv + 1, nChkLRC, pClient);

	return TRUE;
}

