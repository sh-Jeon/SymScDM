#pragma once

#define OP_READ     0
#define OP_WRITE    1

class CClient  
{
public:
	CClient();
	virtual ~CClient();

	int opCode;

	void SetAccept(){ m_bAccept = TRUE; }
	SOCKET GetSocket(){ return m_hSocket; }
	BOOL IsAccepted();

	void SetSocket(SOCKET socket);

	void SetDataBuffer(BYTE* data, DWORD len);

	void ResetSocketBuff()
	{
		ZeroMemory(&readOverlapped, sizeof(WSAOVERLAPPED));
		ZeroMemory(&writeOverlapped, sizeof(WSAOVERLAPPED));

		if (m_ReadDataBuffer) ZeroMemory(m_ReadDataBuffer, MAX_BUFFER);
		if (m_WriteDataBuffer) ZeroMemory(m_WriteDataBuffer, MAX_BUFFER);

		wsaBuf.buf = (char*)m_ReadDataBuffer;
		wsaBuf.len = MAX_BUFFER;
	}

	WSAOVERLAPPED* GetReadOverlap(){ return &readOverlapped; }
	WSAOVERLAPPED* GetWriteOverlap(){ return &writeOverlapped; }

	SOCKET GetSocketHandle() { return m_hSocket; }
	BYTE* GetReadDataBuffer() { return m_ReadDataBuffer; }
	BYTE* GetWriteDataBuffer() { return m_WriteDataBuffer; }

	WSABUF* GetSocketBuffer(int OpCode)
	{
		if (OpCode == OP_READ) wsaBuf.buf = (char*)m_ReadDataBuffer;
		else wsaBuf.buf = (char*)m_WriteDataBuffer;

		return &wsaBuf;
	}

private:
	DWORD m_dwBufLen;

	WSAOVERLAPPED readOverlapped;
	WSAOVERLAPPED writeOverlapped;
	WSABUF wsaBuf;	

	BOOL m_bAccept;

	SOCKET m_hSocket;

	BYTE* m_ReadDataBuffer;
	BYTE* m_WriteDataBuffer;
};
