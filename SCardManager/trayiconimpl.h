// Implementation of the CNotifyIconData class and the CTrayIconImpl template.
#pragma once

#include <atltypes.h>
#include <atlwin.h>

// Wrapper class for the Win32 NOTIFYICONDATA structure
class CNotifyIconData : public NOTIFYICONDATA
{
public:	
	CNotifyIconData()
	{
		Clear();
	}

	void Clear()
	{
		memset(this, 0, sizeof(*this));
		cbSize = NOTIFYICONDATA_V2_SIZE;
	}
};

// Template used to support adding an icon to the taskbar.
// This class will maintain a taskbar icon and associated context menu.
template <class T>
class CTrayIconImpl
{
protected:
	CNotifyIconData	m_nid;
	
	BOOL m_bInstalled;
	CONST UINT WM_TRAYICON;
	CONST UINT WM_TASKBAR_CREATED;

public:	
	CTrayIconImpl() :
	m_bInstalled(FALSE),
	WM_TRAYICON(::RegisterWindowMessage(_T("wm_trayicon"))),
	WM_TASKBAR_CREATED(::RegisterWindowMessage(_T("TaskbarCreated")))
	{
	}
	
	~CTrayIconImpl()
	{
		// Remove the icon
		RemoveIcon();
	}

	// Install a taskbar icon
	// 	lpszToolTip 	- The tooltip to display
	//	hIcon 		- The icon to display
	// 	nID		- The resource ID of the context menu
	// returns true on success
	BOOL InstallIcon(LPCTSTR lpszToolTip, HICON hIcon, UINT nMenuResID)
	{
		T* pT = static_cast<T*>(this);

		if( FALSE == ::IsWindow(pT->m_hWnd) )
		{
			return FALSE;

		}

		m_nid.Clear();
		// Fill in the data
		m_nid.hWnd		= pT->m_hWnd;
		m_nid.uID		= nMenuResID;
		m_nid.uFlags	= NIF_ICON | NIF_MESSAGE | NIF_TIP;
		m_nid.uCallbackMessage	= WM_TRAYICON;
		m_nid.hIcon		= hIcon;
		_tcscpy_s(m_nid.szTip, lpszToolTip);
		
		// Install
		m_bInstalled = ::Shell_NotifyIcon(NIM_ADD, &m_nid);

		// Done
		return m_bInstalled;
	}

	// Remove taskbar icon
	// returns true on success
	BOOL RemoveIcon()
	{
		if( FALSE == m_bInstalled )
		{
			return FALSE;
		}

		m_bInstalled = FALSE;

		// Remove
		//m_nid.uFlags = 0;
		return ::Shell_NotifyIcon(NIM_DELETE, &m_nid);
	}

	// Set the icon tooltip text
	// returns true on success
	BOOL SetTooltipText(LPCTSTR pszTooltipText)
	{
		if( FALSE == m_bInstalled )
		{
			return FALSE;
		}

		if( NULL == pszTooltipText )
		{
			return FALSE;
		}

		// Fill the structure
		m_nid.uFlags = NIF_TIP;
		_tcscpy_s(m_nid.szTip, pszTooltipText);
		
		// Set
		return ::Shell_NotifyIcon(NIM_MODIFY, &m_nid);
	}

	BOOL ShowTooltipInfoText(
		__in_opt LPCTSTR pszInfoTitle,
		__in_opt LPCTSTR pszInfo
		)
	{
		if( FALSE == m_bInstalled )
		{
			return FALSE;
		}

		m_nid.uFlags		|= NIF_INFO;
		m_nid.uTimeout		= 10;
		m_nid.dwInfoFlags	= NIIF_INFO | NIIF_NOSOUND;
		m_nid.uCallbackMessage = WM_TRAYICON;
		_tcscpy_s( m_nid.szInfoTitle, (pszInfoTitle ? pszInfoTitle : __TEXT("")) );
		_tcscpy_s( m_nid.szInfo, (pszInfo ? pszInfo : __TEXT("")) );

		return ::Shell_NotifyIcon(NIM_MODIFY, &m_nid);
	}

	BOOL ChangeIcon(HICON hIcon)
	{
		if( FALSE == m_bInstalled )
		{
			return FALSE;
		}

		m_nid.hIcon	= hIcon;
		m_nid.uFlags = NIF_ICON;

		return ::Shell_NotifyIcon(NIM_MODIFY, &m_nid);
	}


	BEGIN_MSG_MAP(CTrayIconImpl)
		MESSAGE_HANDLER(WM_TRAYICON, OnNotificationFromTray)
		MESSAGE_HANDLER(WM_TASKBAR_CREATED, OnTaskbarCreated)
	END_MSG_MAP()

	virtual LRESULT OnNotificationFromTray(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		//T* pT = static_cast<T*>(this);

		// Is this the ID we want?
		if( wParam != m_nid.uID )
		{
			return 0;
		}

		// Was the right-button clicked?
		if( LOWORD(lParam) == WM_RBUTTONUP )
		{
			OnTrayRButtonUp();
		}
		else if (LOWORD(lParam) == WM_LBUTTONDBLCLK)
		{
			OnTrayLButtonDblClk();
		}
		else if (LOWORD(lParam) == NIN_BALLOONUSERCLICK)
		{
			OnBalloonTipClicked();
		}

		return 0;
	}

	LRESULT OnTaskbarCreated(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		if( TRUE == m_bInstalled )
		{
			m_nid.uFlags	= NIF_ICON | NIF_MESSAGE | NIF_TIP;
			::Shell_NotifyIcon(NIM_ADD, &m_nid);
		}

		return 0;
	}


protected:

	// Allow the menu items to be enabled/checked/etc.
	virtual void OnTrayRButtonUp()
	{
		// stub
	}

	virtual void OnTrayLButtonDblClk()
	{
		// stub
	}

	virtual VOID OnBalloonTipClicked()
	{
	}
};
