#include "StdAfx.h"
#include "AgentWnd.h"
#include "SCardManagerApp.h"
#include "resource.h"
#include <atlpath.h>
#include <stdlib.h>
#include <atlhttp.h>
#include <strsafe.h>

#include "SCardManagerApp.h"

#define TIMER_ICON_DISPLAY	100

int GetAnsiByteSize(__in LPCWSTR uniStr)
{
	return ::WideCharToMultiByte(CP_ACP, 0, uniStr, -1, NULL, 0, NULL, NULL);
}

std::string ToAnsiStr(__in LPCWSTR uniStr)
{
	int needLength = GetAnsiByteSize(uniStr);
	std::vector<CHAR> ansiBuf;
	ansiBuf.resize(needLength);
	CHAR* pAnsiStr = &ansiBuf[0];
	ZeroMemory(pAnsiStr, needLength);
	::WideCharToMultiByte(CP_ACP, 0, uniStr, EOF, pAnsiStr, needLength, "?", NULL);

	std::string ansiStrBuf = pAnsiStr;
	return ansiStrBuf;
}

CAgentWnd g_AgentWnd;

CAgentWnd::CAgentWnd(void)
{
	m_nIconIdx = 0;
	m_strLogFileName = "";
	s_hLogMutex = INVALID_HANDLE_VALUE;
}


CAgentWnd::~CAgentWnd(void)
{
}


void _WriteLog(LPCTSTR szFilePath, LPCTSTR strLog)
{
	SYSTEMTIME cTime;
	::GetLocalTime(&cTime);
	
	CString strLogTime;
	strLogTime.Format(L"[%08X][%02d:%02d:%02d] ", GetCurrentThreadId(), cTime.wHour, cTime.wMinute, cTime.wSecond);

	CString strText = strLogTime;
	strText += strLog;
	strText += L"\r\n";

	HANDLE hFile = INVALID_HANDLE_VALUE;
	if (FALSE == ATLPath::FileExists(szFilePath))
	{
		hFile = ::CreateFile(
			szFilePath,
			GENERIC_WRITE,
			FILE_SHARE_WRITE | FILE_SHARE_READ,
			NULL,
			CREATE_ALWAYS,
			0,
			NULL);
	}
	else
	{
		hFile = ::CreateFile(
				szFilePath,
				GENERIC_WRITE,
				FILE_SHARE_WRITE | FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);
		::SetFilePointer(hFile, 0, 0, FILE_END);
	}

	DWORD dwWritten=0;
	std::string s = ToAnsiStr(strText);
	(VOID)WriteFile(hFile, s.c_str(), (DWORD)s.size(), &dwWritten, 0);
	::FlushFileBuffers(hFile);
		
	CloseHandle(hFile);
}

void CAgentWnd::AddSvrLog(__format_string LPCWSTR fmt, ...)
{
	if (FALSE == ::IsWindow(m_hWnd)) return;
	va_list argList;
	va_start(argList, fmt);
	WCHAR szMsg[4096];
	StringCchVPrintfW(szMsg, _countof(szMsg), fmt, argList);
	va_end(argList);

	if (s_hLogMutex == INVALID_HANDLE_VALUE)
	{
		s_hLogMutex = CreateMutexW(NULL, FALSE, L"LogLock");
		if (s_hLogMutex == nullptr)
		{
			return;
		}
	}

	DWORD dwRet = WaitForSingleObject(s_hLogMutex, 10000);
	if (dwRet == WAIT_OBJECT_0)
	{
		CString strLogFilePath = m_strLogPath;
		CString strLogFileName;

		SYSTEMTIME cTime;
		::GetLocalTime(&cTime);

		strLogFileName.Format(L"%4d-%02d-%02d_%02d_server.log", cTime.wYear, cTime.wMonth, cTime.wDay, cTime.wHour);

		strLogFilePath += L"\\";
		strLogFilePath += strLogFileName;

		_WriteLog(strLogFilePath, szMsg);

	}
	ReleaseMutex(s_hLogMutex);
}
	
void CAgentWnd::AddLog(__format_string LPCWSTR fmt, ...)
{
	if (FALSE == ::IsWindow(m_hWnd)) return;
	va_list argList;
	va_start(argList, fmt);
	WCHAR szMsg[4096];
	StringCchVPrintfW(szMsg, _countof(szMsg), fmt, argList);
	va_end(argList);

	if (s_hLogMutex == INVALID_HANDLE_VALUE)
	{
		s_hLogMutex = CreateMutexW(NULL, FALSE, L"LogLock");
		if (s_hLogMutex == nullptr)
		{
			return;
		}
	}

	DWORD dwRet = WaitForSingleObject(s_hLogMutex, 10000);
	if (dwRet == WAIT_OBJECT_0)
	{
		CString strLogFilePath = m_strLogPath;
		CString strLogFileName;

		SYSTEMTIME cTime;
		::GetLocalTime(&cTime);

		strLogFileName.Format(L"%4d-%02d-%02d_%02d.log", cTime.wYear, cTime.wMonth, cTime.wDay, cTime.wHour);

		strLogFilePath += L"\\";
		strLogFilePath += strLogFileName;

		_WriteLog(strLogFilePath, szMsg);

	}
	ReleaseMutex(s_hLogMutex);
}

void CAgentWnd::SetGroupName(LPCTSTR szGroupName)
{
	if (FALSE == ::IsWindow(m_hWnd)) return;

	g_AgentWnd.SetWindowText(szGroupName);
	m_strGroupName = szGroupName;

	CString strTooltip;
	strTooltip.Format(L"%s 실행 중", m_strGroupName);
	SetTooltipText(strTooltip);
}

LRESULT CAgentWnd::OnLoggingMsg( UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/ )
{
	if (FALSE == ::IsWindow(m_hWnd)) return 0;

	TCHAR* pLog = (TCHAR*)lParam;
	if (pLog)
	{
		if (s_hLogMutex == INVALID_HANDLE_VALUE)
		{
			s_hLogMutex = CreateMutexW(NULL, FALSE, L"LogLock");
			if (s_hLogMutex == nullptr)
			{
				delete pLog;
				return 0;
			}
		}

		DWORD dwRet = WaitForSingleObject(s_hLogMutex, 10000);
		if (dwRet == WAIT_OBJECT_0)
		{
			CString strLogFilePath = m_strLogPath;
			CString strLogFileName;

			SYSTEMTIME cTime;
			::GetLocalTime(&cTime);
	
			strLogFileName.Format(L"%4d-%02d-%02d_%02d.log", cTime.wYear, cTime.wMonth, cTime.wDay, cTime.wHour);

			strLogFilePath += L"\\";
			strLogFilePath += strLogFileName;

			//if (m_strLogFileName != strLogFilePath)
			//{
			//	m_strLogFileName = strLogFilePath;
			//	return 0;
			//}

			CString strLogTime;
			strLogTime.Format(L"[%02d:%02d:%02d] ", cTime.wHour, cTime.wMinute, cTime.wSecond);

			CString strText = strLogTime;
			strText += pLog;
			strText += L"\r\n";

			HANDLE hFile = INVALID_HANDLE_VALUE;
			if (FALSE == ATLPath::FileExists(strLogFilePath))
			{
				hFile = ::CreateFile(
					strLogFilePath,
					GENERIC_WRITE,
					FILE_SHARE_WRITE | FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					0,
					NULL);
			}
			else
			{
				hFile = ::CreateFile(
						strLogFilePath,
						GENERIC_WRITE,
						FILE_SHARE_WRITE | FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);
				::SetFilePointer(hFile, 0, 0, FILE_END);
			}

			DWORD dwWritten=0;
			std::string s = ToAnsiStr(strText);
			(VOID)WriteFile(hFile, s.c_str(), (DWORD)s.size(), &dwWritten, 0);
			::FlushFileBuffers(hFile);
		
			CloseHandle(hFile);
		}
		ReleaseMutex(s_hLogMutex);

		delete [] pLog;
	}

	return 0;
}

void CAgentWnd::SetDBServerTime(LPCTSTR szDBServerTime)
{
	m_strServerTime = szDBServerTime;
	if (::IsWindow(m_hWnd)) PostMessage(WM_APP+703, 0, 0);
}

void CAgentWnd::SetDBLastCommand(LPCTSTR szLastCommand)
{
	m_strLastDBCommand = szLastCommand;
	if (::IsWindow(m_hWnd)) PostMessage(WM_APP+702, 0, 0);
}

void CAgentWnd::SetClientCount(int nClientCount)
{
	if (::IsWindow(m_hWnd))
		PostMessage(WM_APP+902, 0, nClientCount-1);
}

void CAgentWnd::SetServerCommand(LPCTSTR szServerCommand)
{
	m_strServerCommand = szServerCommand;
	if (::IsWindow(m_hWnd)) PostMessage(WM_APP+903, 0, 0);
}

LRESULT CAgentWnd::OnInitDialog( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
{
	if (::IsWindow(m_hWnd)) PostMessage(WM_TRAYICON, 0, 0);

	WCHAR szTempFolder[MAX_PATH] = { 0 };
	LPITEMIDLIST pidl;

	HRESULT hResult = SHGetSpecialFolderLocation(NULL, CSIDL_LOCAL_APPDATA, &pidl);
	if (hResult == S_OK)
	{
		SHGetPathFromIDList(pidl, szTempFolder);
		::CoTaskMemFree(pidl);

		m_strAppIniPath = szTempFolder;
		m_strAppIniPath += L"\\SCardManager";
		CreateDirectory(m_strAppIniPath, 0);		
		m_strAppIniPath += L"\\MgrPList_";
		m_strAppIniPath += g_strGroupID;
		m_strAppIniPath += L".ini";
	}

	TCHAR szLogPath[MAX_PATH];
	if (::GetPrivateProfileString(L"LOG", L"LogFilePath", _T(""), szLogPath, sizeof(szLogPath), m_strAppIniPath))
	{
		m_strLogPath = szLogPath;
	}
	else
	{
		CString sPath = szTempFolder;
		sPath += L"\\SCardManager\\Log";
		m_strLogPath = sPath;
	}

	CreateDirectory(m_strLogPath, 0);

	for (int i=0;i<4;i++)
	{
		m_hStIcon[i] = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_ST_ICON1 + i),
										IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	}

	CString strTooltip;
	strTooltip.Format(L"%s 실행 중", m_strGroupName);
	InstallIcon(strTooltip, m_hStIcon[0], IDR_MENU_TRAY);
	CenterWindow();
	
	m_Aboutdlg.Create(NULL);
	SetTimer(TIMER_ICON_DISPLAY, 500, NULL);
	
	//m_edtLog = GetDlgItem(IDC_EDIT_LOG);
	m_lstCommanderList = GetDlgItem(IDC_LIST_COMMANDER);

	TCHAR* pszColumnName[] = { L"IP", L"PORT", L"CtrlName", L"Last Response", L"State" };
	for (int i=0; i<5; i++)
	{
		LVCOLUMN lvc;
		lvc.mask	= LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.fmt		= LVCFMT_LEFT;
		lvc.pszText = pszColumnName[i];
		if (i==0) lvc.cx = 100;
		else if (i==1) lvc.cx = 50;
		else if (i==2) lvc.cx = 200;
		else if (i==3) lvc.cx = 200;
		else if (i==4) lvc.cx = 130;
		lvc.iSubItem = i;
		m_lstCommanderList.InsertColumn(i, &lvc);
	}

	CEdit edtLogPath = GetDlgItem(IDC_EDT_LOG_PATH);
	edtLogPath.SetWindowText(m_strLogPath);

	m_lstCommanderList.SetFocus();

	return 0;
}

void CAgentWnd::AddCommanderToList(LPCTSTR szServerIP, int nPort)
{
	if (FALSE == ::IsWindow(m_hWnd)) return;

	int nRow = m_lstCommanderList.GetItemCount();

	m_lstCommanderList.AddItem(nRow, 0, szServerIP, 0);

	CString strIP;
	strIP.Format(L"%d", nPort);
	m_lstCommanderList.AddItem(nRow, 1, strIP, 0);
	m_lstCommanderList.SetItemData(nRow, nRow);
}

void CAgentWnd::UpdateCommanderInList(LPCTSTR szServerIP, int nPort, LPCTSTR szCtrlName, LPCTSTR szCommander, LPCTSTR szState)
{
	if (FALSE == ::IsWindow(m_hWnd)) return;

	ST_READER_LIST_INFO *pstReaderInfo = new ST_READER_LIST_INFO;
	pstReaderInfo->nPort = nPort;
	pstReaderInfo->strCtrlName = szCtrlName;
	pstReaderInfo->strIP = szServerIP;
	pstReaderInfo->strCtrlName = szCtrlName;
	pstReaderInfo->szCommander = szCommander;
	pstReaderInfo->szState = szState;

	if (::IsWindow(m_hWnd)) PostMessage(WM_APP+704, 0, (LPARAM)pstReaderInfo);
}

LRESULT CAgentWnd::OnUserMessage( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/ )
{
	if (uMsg == WM_APP+701)
	{
		TCHAR* pszIP = (TCHAR*)lParam;
		int nPort = (int)wParam;
		if (pszIP)
		{
			CString strChkIP = pszIP;
			int nRow = m_lstCommanderList.GetItemCount();
	
			for (int i=0; i<nRow; i++)
			{
				TCHAR szListIP[16];
				if (m_lstCommanderList.GetItemText(i, 0, szListIP, 16))
				{
					TCHAR szPort[10];
					m_lstCommanderList.GetItemText(i, 1, szPort, 10);

					if (0 == strChkIP.CompareNoCase(szListIP) && nPort == _tstol(szPort))
					{
						m_lstCommanderList.SetItemText(i, 3, L"Fail to Connect"); 
						break;
					}
				}
			}
			delete pszIP;
		}
	}
	else if (uMsg == WM_APP+702)
	{
		static int chkRun = 0;
		if (FALSE == ::IsWindow(m_hWnd)) return 0;
		CStatic stDBCommand = GetDlgItem(IDC_ST_DB_COMMAND);
		if (stDBCommand.IsWindow())
		{
			CString strLastCommand = m_strLastDBCommand;
			strLastCommand += L" ";

			for (int i=0;i < chkRun % 5; i++)
			{
				strLastCommand += L".";
			}

			chkRun++;
			if (chkRun == 1000) chkRun = 0;

			stDBCommand.SetWindowText(strLastCommand);
		}

		SYSTEMTIME cTime;
		::GetLocalTime(&cTime);

		CString strTime;
		strTime.Format(L"%02d-%02d %02d:%02d:%02d", cTime.wMonth, cTime.wDay, cTime.wHour, cTime.wMinute, cTime.wSecond);

		GetDlgItem(IDC_ST_COMMAND_TIME).SetWindowText(strTime);
	} 
	else if (uMsg == WM_APP+703)
	{
		CStatic stDBTime = GetDlgItem(IDC_ST_DB_TIME);
		if (stDBTime.IsWindow())
		{
			stDBTime.SetWindowText(m_strServerTime);
		}
	}
	else if (uMsg == WM_APP+704)
	{
		ST_READER_LIST_INFO *pstReaderInfo = (ST_READER_LIST_INFO*)lParam; 
		CString strChkIP = pstReaderInfo->strIP;
		int nRow = m_lstCommanderList.GetItemCount();
	
		for (int i=0; i<nRow; i++)
		{
			TCHAR szListIP[16];
			if (m_lstCommanderList.GetItemText(i, 0, szListIP, 16))
			{
				TCHAR szPort[10];
				m_lstCommanderList.GetItemText(i, 1, szPort, 10);

				if (strChkIP == szListIP && pstReaderInfo->nPort == _tstol(szPort))
				{
					CString strPort;
					strPort.Format(L"%d", pstReaderInfo->nPort);
					m_lstCommanderList.SetItemText(i, 1, strPort);
					if (pstReaderInfo->strCtrlName) m_lstCommanderList.SetItemText(i, 2, pstReaderInfo->strCtrlName);
					if (pstReaderInfo->szCommander) m_lstCommanderList.SetItemText(i, 3, pstReaderInfo->szCommander);
					if (pstReaderInfo->szState) m_lstCommanderList.SetItemText(i, 4, pstReaderInfo->szState);

					break;
				}
			}
		}

		delete pstReaderInfo;
	}


	// 단말기 시간이 db 시간과 다름
	if (uMsg == WM_APP+901)
	{
		_theApp.SetTerminalTime(lParam);
	}
	else if (uMsg == WM_APP+902)
	{
		CStatic stClientCount = GetDlgItem(IDC_ST_CLIENT_COUNT);
		CString strCount;
		strCount.Format(L"%d", lParam);
		if (stClientCount.IsWindow())
		{
			stClientCount.SetWindowText(strCount);
		}
	}
	else if (uMsg == WM_APP+903)
	{
		CStatic stServerCommand = GetDlgItem(IDC_ST_SERVER_COMMAND);

		if (stServerCommand.IsWindow())
		{
			stServerCommand.SetWindowText(m_strServerCommand);
		}
	}

	return 0;
}

void CAgentWnd::OnConnectFail(LPCTSTR szIP, int nPort)
{
	if (::IsWindow(m_hWnd))
	{
		TCHAR* szCopyIP = new TCHAR[20];
		_tcsncpy(szCopyIP, szIP, 20);
		PostMessage(WM_APP+701, (WPARAM)nPort, (LPARAM)szCopyIP);
	}
}

void CAgentWnd::DeleteCommanderInList(LPCTSTR szServerIP, int nPort)
{
	if (FALSE == ::IsWindow(m_hWnd)) return;
	CString strChkIP = szServerIP;
	int nRow = m_lstCommanderList.GetItemCount();
	
	for (int i=0; i<nRow; i++)
	{
		TCHAR szListIP[16];
		if (m_lstCommanderList.GetItemText(i, 0, szListIP, 16))
		{
			TCHAR szPort[10];
			m_lstCommanderList.GetItemText(i, 1, szPort, 10);

			if (0 == strChkIP.CompareNoCase(szListIP) && nPort == _tstol(szPort))
			{
				m_lstCommanderList.DeleteItem(i);
				break;
			}
		}
	}
}

LRESULT CAgentWnd::OnBnClickedOk(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (IDYES == ::MessageBox(NULL, L"출입 통제 관리 데몬을 끝내면 단말기 제어가 종료됩니다.\r\n\r\n데몬 프로그램을 종료하시겠습니까?", L"주의", MB_YESNO))
	{
		_theApp.StopDaemon();
	}

	return 0;
}


LRESULT CAgentWnd::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	RemoveIcon();

	KillTimer(TIMER_ICON_DISPLAY);

	for (int i=0;i<4;i++)
	{
		DestroyIcon(m_hStIcon[i]);
	}

	m_Aboutdlg.DestroyWindow();

	return 0;
}

void CAgentWnd::SendAliveToService()
{
	static DWORD lastTick = GetTickCount();
	if (GetTickCount() - lastTick < 2000){
		return;
	}

	lastTick = GetTickCount();
	//Named pipe를 통해 process id를 전달하여 감시할 수 있게 한다.
	HANDLE hSVC = OpenNamedPipeHandle(MONITOR_SVC_PIPENAME, 10000);
	if (hSVC == INVALID_HANDLE_VALUE)
	{
		return;
	}

	DWORD dwBytesWritten;
	SVC_CMD svcCmd;
	svcCmd.Cmd = CMD_ALIVE;
	svcCmd.dwPid = _getpid();
	svcCmd.szManagerPath[0] = L'\0';
	BOOL bWrite = WriteFile(hSVC, &svcCmd, sizeof(svcCmd), &dwBytesWritten, 0);
	if (!bWrite)
	{
		CloseHandle(hSVC);
		return;
	}

	FlushFileBuffers(hSVC);

	DWORD dwRead;
	ZeroMemory(&svcCmd, sizeof(SVC_CMD));
	if (!ReadFile(hSVC, &svcCmd, sizeof(SVC_CMD), &dwRead, 0))
	{
		CloseHandle(hSVC);
		return;
	}

	CloseHandle(hSVC);
}


LRESULT CAgentWnd::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if (wParam == TIMER_ICON_DISPLAY)
	{
		ChangeIcon(m_hStIcon[m_nIconIdx++]);

		if (m_nIconIdx == 4)
		{
			m_nIconIdx = 0;
		}

		SendAliveToService();
	}		

	return 0;
}

LRESULT CAgentWnd::OnViewStatusWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	ShowWindow(SW_SHOW);

	return 0;
}

LRESULT CAgentWnd::OnViewVersoin(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	m_Aboutdlg.ShowWindow(SW_SHOW);

	return 0;
}

void CAgentWnd::OnTrayRButtonUp()
{
	// Get the menu position
	CPoint pos;
	GetCursorPos ( &pos );

	// Load the menu
	CMenu oMenu;
	if ( FALSE == oMenu.LoadMenu ( m_nid.uID ) )
	{
		return;
	}

	// Get the sub-menu
	CMenuHandle oPopup ( oMenu.GetSubMenu ( 0 ) );
	MENUITEMINFO minfo = {0};
	minfo.cbSize = sizeof(minfo);
	minfo.fMask = MIIM_FTYPE | MIIM_ID;

	SetForegroundWindow ( m_hWnd );

	// Track
	oPopup.TrackPopupMenu ( TPM_LEFTALIGN, pos.x, pos.y, m_hWnd );

	// BUGFIX: See "PRB: Menus for Notification Icons Don't Work Correctly"
	if (::IsWindow(m_hWnd)) PostMessage ( WM_NULL );

	// Done
	oMenu.DestroyMenu();
}

LRESULT CAgentWnd::OnBnClickedCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	ShowWindow(SW_HIDE);
	return 0;
}


LRESULT CAgentWnd::OnBnClickedBtnLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	WCHAR szLogFolder[MAX_PATH] = { 0 };
	
	TCHAR tzDisplay[MAX_PATH];   
	memset(&tzDisplay, 0x00, sizeof(tzDisplay));
	
	BROWSEINFO bi;    
	memset(&bi, 0, sizeof(BROWSEINFO));    
	bi.hwndOwner = m_hWnd;    
	bi.pidlRoot = NULL;    
	bi.pszDisplayName = tzDisplay;
	bi.lpszTitle      = L"";    
	bi.ulFlags        = BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS | BIF_DONTGOBELOWDOMAIN;    
	bi.lpfn           = NULL;   
	bi.lParam         = NULL;            
	 
	LPITEMIDLIST stPath;    
	stPath = SHBrowseForFolder(&bi); 

	SHGetPathFromIDList(stPath, szLogFolder);	

	if (lstrlen(szLogFolder) > 0)
	{
		m_strLogPath = szLogFolder;

		CEdit edtLogPath = GetDlgItem(IDC_EDT_LOG_PATH);
		edtLogPath.SetWindowText(m_strLogPath);

		::WritePrivateProfileString(
				L"LOG",
				L"LogFilePath", 
				m_strLogPath, 
				m_strAppIniPath);
	}

	return 0;
}

void CAgentWnd::SendSMS(LPCTSTR szSMSURL)
{
	CAtlNavigateData atlNaviData;
	CAtlHttpClient httpClient;
	if (FALSE == httpClient.Navigate(szSMSURL, &atlNaviData))
	{
		httpClient.Navigate(szSMSURL, &atlNaviData);
	}
}