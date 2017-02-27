// SCardManager.cpp : main source file for SCardManager.exe
//

#include "stdafx.h"

#include "resource.h"
#include "AgentWnd.h"

#include "aboutdlg.h"
#include "SCardManagerApp.h"
#include <atlpath.h>
#include "../Define.h"

CAppModule _Module;

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CString strLauncher;
	TCHAR szModulePath[MAX_PATH];
	::GetModuleFileName(_Module.m_hInst, szModulePath, MAX_PATH);
	ATLPath::RemoveFileSpec(szModulePath);
	strLauncher = szModulePath;
	strLauncher += L"\\SCardMgrLauncher.exe";

	HANDLE hLauncher = OpenNamedPipeHandle(LAUNCHER_PIPENAME, 10000);
	if (hLauncher == INVALID_HANDLE_VALUE)
	{
		ShellExecute(GetDesktopWindow(),L"open",strLauncher,NULL,NULL,SW_SHOWNORMAL); 
	}	
	else
	{
		CloseHandle(hLauncher);
	}

	g_AgentWnd.Create(NULL);
	g_AgentWnd.ShowWindow(SW_SHOW);
	if (IsWindow(g_AgentWnd.m_hWnd))
	{
		::BringWindowToTop(g_AgentWnd.m_hWnd);
	}
	ATLTRACE(L"ShowWindow %d", IsWindow(g_AgentWnd.m_hWnd));

	_theApp.StartDeamon();

	int nRet = theLoop.Run();

	if (IsWindow(g_AgentWnd.m_hWnd))
		g_AgentWnd.DestroyWindow();

	_Module.RemoveMessageLoop();

	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
