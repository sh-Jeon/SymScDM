#include "StdAfx.h"
#include "ADODB.h"
#include "SCardManagerApp.h"
#include "AgentWnd.h"


CADODB::CADODB(void)
{
	m_pConn = NULL;

	m_strErrorMessage = _T("");
}


CADODB::~CADODB(void)
{
	if (IsConnect()) Close();

	if (m_pConn)
	{
		m_pConn.Release();
		m_pConn = NULL;
		::CoUninitialize();
	}
}

int CADODB::IsConnect(void)
{
	if (m_pConn  == NULL || (m_pConn != NULL && m_pConn->State == 0))
	{
		//return Open();
		return FALSE;
	}

	return TRUE;
}

BOOL CADODB::Open()
{
	if (IsConnect()) return TRUE;
	TCHAR szProvider[255], szIP[20], szSource[50], szUID[25];
	ZeroMemory(szProvider, 255);
	ZeroMemory(szIP, 20);
	ZeroMemory(szSource, 50);
	ZeroMemory(szUID, 25);

	DWORD dwRet = GetPrivateProfileStringW(L"DB_INFO", L"Provider", _T(""), szProvider, sizeof(szProvider), g_strRegPath);
	dwRet = GetPrivateProfileStringW(L"DB_INFO", L"DataSource", _T(""), szIP, sizeof(szIP), g_strRegPath);
	dwRet = GetPrivateProfileStringW(L"DB_INFO", L"Initial Catalog", _T(""), szSource, sizeof(szSource), g_strRegPath);
	dwRet = GetPrivateProfileStringW(L"DB_INFO", L"User ID", _T(""), szUID, sizeof(szUID), g_strRegPath);

	CString strServer;
	strServer.Format(L"Provider=%s;Persist Security Info=True;Data Source=%s;Initial Catalog=%s;",
		szProvider, szIP, szSource);
	strServer += L"User ID=";
	strServer += szUID;
	strServer += ";Password=!@#$daemon;";

	g_AgentWnd.AddLog(L"Try Connect to db");

	_bstr_t strCnn(strServer);

	::CoInitialize(NULL);
	if FAILED(m_pConn.CreateInstance(__uuidof(Connection)))
	{
		m_strErrorMessage="Fail to Create Instance for ADO Connect ptr";
		g_AgentWnd.AddLog(L"%s", m_strErrorMessage);
		return FALSE;
	}

	try
	{
		m_pConn->Open(strCnn, L"", L"", NULL);
	}
	catch(_com_error &e)
	{
		CString s = (LPCTSTR)e.ErrorMessage();
		m_strErrorMessage="Open 에러 (" + s + ")\n";
		m_pConn = NULL;
		g_AgentWnd.AddLog(L"%s", m_strErrorMessage);

		return FALSE;
	}

	return TRUE;
}

void CADODB::Close()
{
	if (m_pConn)
	{
		try {
			m_pConn->Close();
			m_pConn.Release();
			m_pConn = NULL;
		}
		catch(_com_error &e)
		{
			::CoUninitialize();
		}
	}
}

CString CADODB::ErrMsg()
{
	return m_strErrorMessage;
}

void CADODB::GetServiceEnv()
{
	if(IsConnect() != TRUE)
	{
		if (FALSE == Open())
		{
			return;
		}
	}

	g_stReqFlag.sFlagDoorSt = L"N";
	g_stReqFlag.sFlagEventsCnt = L"N";
	g_stReqFlag.sFlagIdCnt = L"N";
	g_stReqFlag.sFlagTime = L"N";
	g_stReqFlag.sFlagTimeCode = L"N";
	g_stReqFlag.sFlagVer = L"N";

	CString strQuery;
	strQuery.Format(L"exec SP_GetServiceEnv \'%s\'", g_strGroupID);
	g_AgentWnd.AddLog(L"%s", strQuery);
	try
	{
		_bstr_t	bstrQuery(strQuery);
		_variant_t	vRecsAffected2(0L);
		
		CString strSendCmdPollingTime;
		CString strChkIdPollingTime;

		// Recordset개체 포인터 
		_RecordsetPtr	pRS;
		pRS = m_pConn->Execute(bstrQuery, &vRecsAffected2, adOptionUnspecified);
		while(!(pRS->EndOfFile))
		{
			_variant_t vTempValue;

			vTempValue = pRS->GetCollect(L"SendCmdPollingTime");
			if(vTempValue.vt != VT_NULL)
			{
				strSendCmdPollingTime = (LPCTSTR)vTempValue.bstrVal;
				g_dwCmdPollingTime = _tstol(strSendCmdPollingTime);
			}

			CString strGroupName;
			vTempValue = pRS->GetCollect(L"groupname");
			if(vTempValue.vt != VT_NULL)
			{
				strGroupName = (LPCTSTR)vTempValue.bstrVal;
				g_AgentWnd.SetGroupName(strGroupName);
			}

			pRS->MoveNext();
		}	

		pRS->Close();
	}
	catch(_com_error &e)
	{
		CString s = (LPCTSTR)e.Description();

		if (IsConnect()) Close();

		m_strErrorMessage="GetServiceEnv 에러 (" + s + ")\n";
	}
}

BOOL CADODB::GetServerTime(CTime &tDBTime)
{
	if(IsConnect() != TRUE)
	{
		if (FALSE == Open())
		{
			return FALSE;
		}
	}

	CString strQuery = L"SP_GetTime";
	try
	{
		_bstr_t	bstrQuery(strQuery);
		_variant_t	vRecsAffected2(0L);
		
		_RecordsetPtr	pRS;
		pRS = m_pConn->Execute(bstrQuery, &vRecsAffected2, adOptionUnspecified);
		if (!(pRS->EndOfFile))
		{
			_variant_t vTempValue;

			vTempValue = pRS->GetCollect(L"SERVERTIME");
			if(vTempValue.vt != VT_NULL)
			{
				COleDateTime dbTime = COleDateTime(vTempValue);
				CTime t(
					dbTime.GetYear(),
					dbTime.GetMonth(),
					dbTime.GetDay(),
					dbTime.GetHour(),
					dbTime.GetMinute(),
					dbTime.GetSecond());
				tDBTime = t;
			}
		}	

		pRS->Close();
	}
	catch(_com_error &e)
	{
		CString s = (LPCTSTR)e.Description();

		if (IsConnect()) Close();

		m_strErrorMessage="GetServerTime 에러 (" + s + ")\n";

		return FALSE;
	}

	return TRUE;
}

BOOL CADODB::GetCardReaderList(std::vector<ST_READER_INFO> *pVecReaderList)
{
	if(IsConnect() != TRUE)
	{
		if (FALSE == Open())
		{
			return FALSE;
		}
	}

	CString strQuery;
	strQuery.Format(L"exec SP_GetReaderList \'%s\'", g_strGroupID);
	try
	{
		_bstr_t	bstrQuery(strQuery);
		_variant_t	vRecsAffected2(0L);
		
		_RecordsetPtr	pRS;
		pRS = m_pConn->Execute(bstrQuery, &vRecsAffected2, adOptionUnspecified);
		while(!(pRS->EndOfFile))
		{
			_variant_t vTempValue;

			CString strCtrlIP;
  			vTempValue = pRS->GetCollect(L"TcpCtrlIP");
			if(vTempValue.vt != VT_NULL)
			{
				strCtrlIP = (LPCTSTR)vTempValue.bstrVal;
			}

			int nPort = 4001;
			vTempValue = pRS->GetCollect(L"TcpCtrlPort");
			if(vTempValue.vt != VT_NULL)
			{
				nPort = _tstol ((LPCTSTR)vTempValue.bstrVal);
			}

			CString strCtrlName;
			vTempValue = pRS->GetCollect(L"CtrlName");
			if(vTempValue.vt != VT_NULL)
			{
				strCtrlName = (LPCTSTR)vTempValue.bstrVal;
			}
			
			CString strReaderID;
			vTempValue = pRS->GetCollect(L"ReaderID");
			if(vTempValue.vt != VT_NULL)
			{
				strReaderID = (LPCTSTR)vTempValue.bstrVal;
			}

			ST_READER_INFO stReaderInfo;
			stReaderInfo.strIP = strCtrlIP;
			stReaderInfo.nPort = nPort;
			stReaderInfo.strCtrlName = strCtrlName;
			stReaderInfo.strReaderID = strReaderID;

			pVecReaderList->push_back(stReaderInfo);

			pRS->MoveNext();
		}	

		pRS->Close();
	}
	catch(_com_error &e)
	{
		CString s = (LPCTSTR)e.Description();

		if (IsConnect()) Close();

		m_strErrorMessage="SelectCardList 에러 (" + s + ")\n";

		return FALSE;
	}

	g_AgentWnd.AddLog(L"%s", strQuery);

	return TRUE;
}

BOOL CADODB::SetServiceAlive(void)
{
	if(IsConnect() != TRUE)
	{
		if (FALSE == Open())
		{
			return FALSE;
		}
	}

	CString strQuery;
	strQuery.Format(L"exec SP_ServiceAlive \'%s\'", g_strGroupID);
	g_AgentWnd.AddLog(L"%s", strQuery);
	try
	{
		_bstr_t	bstrQuery(strQuery);
		_variant_t	vRecsAffected2(0L);
		
		if (m_pConn)
			m_pConn->Execute(bstrQuery, &vRecsAffected2, adOptionUnspecified);
	}
	catch(_com_error &e)
	{
		CString s = (LPCTSTR)e.Description();

		if (IsConnect()) Close();

		m_strErrorMessage="SP_ServiceAlive 에러 (" + s + ")\n";
		m_strErrorMessage += strQuery;
		m_strErrorMessage += L"\n";

		return FALSE;
	}

	return TRUE;
}

BOOL CADODB::GetSendCommand(LPCTSTR szReaderIP, int nPort, VEC_SEND_CMD* pVecSendCmd)
{
	if(IsConnect() != TRUE)
	{
		if (FALSE == Open())
		{
			return FALSE;
		}
	}

	CString strQuery;
	strQuery.Format(L"exec SP_GetSendCmd \'%s\', \'%s\', \'%d\'", g_strGroupID, szReaderIP, nPort);
	try
	{
		_bstr_t	bstrQuery(strQuery);
		_variant_t	vRecsAffected2(0L);
		
		_RecordsetPtr	pRS;
		pRS = m_pConn->Execute(bstrQuery, &vRecsAffected2, adOptionUnspecified);
		while(!(pRS->EndOfFile))
		{
			_variant_t vTempValue;

			CString strCmdIdx;
			vTempValue = pRS->GetCollect(L"CMDINDEX");
			if(vTempValue.vt != VT_NULL)
			{
				strCmdIdx = vTempValue.bstrVal;
			}

			CString strCtrlIP;
			vTempValue = pRS->GetCollect(L"TcpCtrlIP");
			if(vTempValue.vt != VT_NULL)
			{
				strCtrlIP = (LPCTSTR)vTempValue.bstrVal;
			}

			int nPort = 4001;
			vTempValue = pRS->GetCollect(L"TcpCtrlPort");
			if(vTempValue.vt != VT_NULL)
			{
				nPort = vTempValue.intVal;
			}
			
			CString strReaderID;
			vTempValue = pRS->GetCollect(L"ReaderID");
			if(vTempValue.vt != VT_NULL)
			{
				strReaderID = (LPCTSTR)vTempValue.bstrVal;
			}

			CString strCommand;
			vTempValue = pRS->GetCollect(L"SndCmd");
			if(vTempValue.vt != VT_NULL)
			{
				strCommand = (LPCTSTR)vTempValue.bstrVal;
			}

			ST_SEND_CMD stSendCmd;
			stSendCmd.nCmdIdx = _tstol(strCmdIdx);
			stSendCmd.nPort = nPort;
			stSendCmd.nReaderID = _tstol(strReaderID);
			stSendCmd.strCmd = strCommand;
			pVecSendCmd->push_back(stSendCmd);

			pRS->MoveNext();
		}	
		pRS->Close();
	}
	catch(_com_error &e)
	{
		CString s = (LPCTSTR)e.Description();

		if (IsConnect()) Close();

		m_strErrorMessage="SP_GetSendCmd 에러 (" + s + ")\n";

		return FALSE;
	}

	if (pVecSendCmd->size())
		g_AgentWnd.AddLog(L"%s cmd count : %d", strQuery, pVecSendCmd->size());

	return TRUE;
}

BOOL CADODB::CESetResultSendCmd(int cmdIndex, LPCTSTR szResult, LPCTSTR szRecvData, BOOL bValidation)
{
	if(IsConnect() != TRUE)
	{
		if (FALSE == Open())
		{
			return FALSE;
		}
	}

	CString strQuery;
	strQuery.Format(L"exec SP_ResultSendCmd %d, \'%s\', \'%s\', \'%c\'", cmdIndex, szResult, szRecvData, bValidation ? L'Y':L'N');

	try
	{
		_bstr_t	bstrQuery(strQuery);
		_variant_t	vRecsAffected2(0L);
		
		g_AgentWnd.AddLog(L"%s", strQuery);
		m_pConn->Execute(bstrQuery, &vRecsAffected2, adOptionUnspecified);
	}
	catch(_com_error &e)
	{
		CString s = (LPCTSTR)e.Description();

		if (IsConnect()) Close();

		m_strErrorMessage ="SP_ResultSendCmd 에러 (" + s + ")\n";
		m_strErrorMessage += strQuery;
		m_strErrorMessage += L"\n";

		return FALSE;
	}

	return TRUE;
}

BOOL CADODB::CESetReaderEvent(LPCTSTR strReaderID, LPCTSTR szCmd, LPCTSTR szRecvData, BOOL bValidation)
{
	CString chkcmd = szCmd;
	if (chkcmd.CompareNoCase(L"5402")){
		g_AgentWnd.AddSvrLog(L"CESetReaderEvent readerID: %s, cmd: %s, recvData: %s, validation: %d", 
						strReaderID, szCmd, szRecvData, bValidation);
	}

	if(IsConnect() != TRUE)
	{
		if (FALSE == Open())
		{
			return FALSE;
		}
	}

	BOOL bResult = FALSE;

	try
	{
		_CommandPtr pCommand;
		pCommand.CreateInstance(__uuidof(Command));

		pCommand->ActiveConnection = m_pConn; 
		pCommand->CommandText = _bstr_t(L"SP_ResultGetReaderEvents");
		pCommand->CommandType = adCmdStoredProc;
		pCommand->CommandTimeout = 5;

		//DWORD nLen = lstrlen(szServerIP);
		//_variant_t varParam1 = _bstr_t(szServerIP);
		//_ParameterPtr pParam1 = pCommand->CreateParameter(_bstr_t(L"TcpCtrlIP"), adVarChar, adParamInput, nLen, &varParam1);
		//pCommand->Parameters->Append(pParam1);

		DWORD nLen = lstrlen(strReaderID);
		_variant_t varParam2 = _bstr_t(strReaderID);
		_ParameterPtr pParam2 = pCommand->CreateParameter(_bstr_t(L"ReaderID"), adVarChar, adParamInput, nLen, &varParam2);
		pCommand->Parameters->Append(pParam2);

		nLen = lstrlen(szCmd);
		_variant_t varParam3 = _bstr_t(szCmd);
		_ParameterPtr pParam3 = pCommand->CreateParameter(_bstr_t(L"eventType"), adVarChar, adParamInput, nLen, &varParam3);
		pCommand->Parameters->Append(pParam3);

		nLen = lstrlen(szRecvData);
		_variant_t varParam4 = _bstr_t(szRecvData);
		_ParameterPtr pParam4 = pCommand->CreateParameter(_bstr_t(L"RecvData"), adVarChar, adParamInput, nLen, &varParam4);
		pCommand->Parameters->Append(pParam4);

		CString szValidation = bValidation ? L"Y":L"N";
		nLen = lstrlen(szValidation);
		_variant_t varParam5 = _bstr_t(szValidation);
		_ParameterPtr pParam5 = pCommand->CreateParameter(_bstr_t(L"LRC"), adVarChar, adParamInput, nLen, &varParam5);
		pCommand->Parameters->Append(pParam5);

		_ParameterPtr pParamReturn = pCommand->CreateParameter(_bstr_t("Result"), adBSTR, adParamReturnValue, 2048);
		pCommand->Parameters->Append(pParamReturn);

		//_RecordsetPtr pRS = 
		pCommand->Execute(NULL, NULL, adCmdStoredProc);

		CString strResult;
		_variant_t vTempValue = pParamReturn->Value;
		if(vTempValue.vt != VT_NULL)
		{
			strResult = (LPCTSTR)vTempValue.bstrVal;
			if (0 != strResult.CompareNoCase(L"Y"))
			{
				bResult = TRUE;
				//g_AgentWnd.SendSMS(strResult);
			}
		}

		pCommand.Release();
	}
	catch(_com_error &e)
	{
		CString s = (LPCTSTR)e.Description();

		if (IsConnect()) Close();

		m_strErrorMessage ="SP_ResultGetReaderEvents 에러 (" + s + ")\n";
		m_strErrorMessage += L"\n";

		return FALSE;
	}

	return bResult;
}