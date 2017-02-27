#pragma once

#include "resource.h"
#include "trayiconimpl.h"
#include "AboutDlg.h"

#define SCARD_CLASS_NAME		_T("SCardManagerWndClass")

typedef struct _Reader_List_Info_
{
	CString strIP;
	int nPort;
	CString strCtrlName;
	CString strReaderID;
	CString szCommander;
	CString szState;
}ST_READER_LIST_INFO;

class CAgentWnd : 
	public CDialogImpl<CAgentWnd>,
	public CMessageFilter,
	public CTrayIconImpl<CAgentWnd>
{
public:
	CAgentWnd(void);
	~CAgentWnd(void);

	enum { IDD = IDD_APP_STATUS };

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return IsDialogMessage(pMsg);
	}

	BEGIN_MSG_MAP(CAgentWnd)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_HANDLER(IDOK, BN_CLICKED, OnBnClickedOk)
		COMMAND_ID_HANDLER(ID_TRAY_EXIT, OnBnClickedOk)
		COMMAND_ID_HANDLER(ID_TRAY_VIEWSTAT, OnViewStatusWindow)
		COMMAND_ID_HANDLER(ID_TRAY_VERSION, OnViewVersoin)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_APP+700, OnLoggingMsg)
		MESSAGE_HANDLER(WM_APP+701, OnUserMessage)
		MESSAGE_HANDLER(WM_APP+702, OnUserMessage)
		MESSAGE_HANDLER(WM_APP+703, OnUserMessage)
		MESSAGE_HANDLER(WM_APP+704, OnUserMessage)
		MESSAGE_HANDLER(WM_APP+901, OnUserMessage)
		MESSAGE_HANDLER(WM_APP+902, OnUserMessage)
		MESSAGE_HANDLER(WM_APP+903, OnUserMessage)
		COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnBnClickedCancel)
		CHAIN_MSG_MAP(CTrayIconImpl<CAgentWnd>)
		COMMAND_HANDLER(IDC_BTN_LOG, BN_CLICKED, OnBnClickedBtnLog)
	END_MSG_MAP()

	LRESULT OnInitDialog( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ );
	LRESULT OnLoggingMsg( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ );
	LRESULT OnUserMessage( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ );
	LRESULT OnBnClickedOk(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnViewStatusWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewVersoin(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedBtnLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);


	virtual void OnTrayRButtonUp();

	void SetGroupName(LPCTSTR szGroupName);

	void AddLog(__format_string LPCWSTR fmt, ...);
	void AddSvrLog(__format_string LPCWSTR fmt, ...);

	void SetDBServerTime(LPCTSTR szDBServerTime);
	void SetDBLastCommand(LPCTSTR szLastCommand);

	void SetClientCount(int nClientCount);
	void SetServerCommand(LPCTSTR szServerCommand);

	void AddCommanderToList(LPCTSTR szServerIP, int nPort);
	void UpdateCommanderInList(LPCTSTR szServerIP, int nPort, LPCTSTR szCtrlName, LPCTSTR szCommander, LPCTSTR szState);
	void DeleteCommanderInList(LPCTSTR szServerIP, int nPort);
	void OnConnectFail(LPCTSTR szIP, int nPort);
	void SendSMS(LPCTSTR szSMSURL);

private:
	HANDLE s_hLogMutex;
	HICON m_hStIcon[4];
	int m_nIconIdx;
	CAboutDlg m_Aboutdlg;

	CString m_strLogPath;
	CString m_strAppIniPath;

	CString m_strGroupName;

	CString m_strLogFileName;
	CString m_strServerCommand;

	CString m_strLastDBCommand;
	CString m_strServerTime;

	CListViewCtrl m_lstCommanderList;

	void SendAliveToService();
};

extern CAgentWnd g_AgentWnd;
