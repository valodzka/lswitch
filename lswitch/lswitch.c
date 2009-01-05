#define _WIN32_WINNT 0x500

#include <windows.h>
#include <tchar.h>

TCHAR	g_prog_dir[MAX_PATH*2];
DWORD	g_prog_dir_len;
HHOOK	g_khook;
HANDLE  g_hEvent;
UINT	g_key=VK_APPS;

LRESULT CALLBACK KbdHook(int nCode,WPARAM wParam,LPARAM lParam) {
	if (nCode<0)
		return CallNextHookEx(g_khook,nCode,wParam,lParam);
	if (nCode==HC_ACTION) {
		KBDLLHOOKSTRUCT   *ks=(KBDLLHOOKSTRUCT*)lParam;
		if (ks->vkCode==g_key) {
			if (wParam==WM_KEYDOWN) {
				HWND hWnd=GetForegroundWindow();
				if (hWnd)
					PostMessage(hWnd,WM_INPUTLANGCHANGEREQUEST,0,(LPARAM)HKL_NEXT);
			}
			return 1;
		}
	}

	return CallNextHookEx(g_khook,nCode,wParam,lParam);
}

void  failedx(const TCHAR *msg) {
	MessageBox(NULL,msg,_T("Error"),MB_OK|MB_ICONERROR);
	ExitProcess(1);
}

void  failed(const TCHAR *msg) {
	DWORD		fm;
	TCHAR		*msg1,*msg2;
	const TCHAR	*args[2];

	fm=FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,GetLastError(),0,(LPTSTR)&msg1,0,NULL);
	if (fm==0)
		ExitProcess(1);
	args[0]=msg;
	args[1]=msg1;
	fm=FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING|
		FORMAT_MESSAGE_ARGUMENT_ARRAY,
		_T("%1: %2"),0,0,(LPTSTR)&msg2,0,(va_list*)&args[0]);
	if (fm==0)
		ExitProcess(1);
	MessageBox(NULL,msg2,_T("Error"),MB_OK|MB_ICONERROR);
	ExitProcess(1);
}

void CALLBACK TimerCb(HWND hWnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime) {
	if (WaitForSingleObject(g_hEvent,0)==WAIT_OBJECT_0)
		PostQuitMessage(0);
}

int xMain() {
	MSG	  msg;
	DWORD	  sz;
	BOOL	  fQuit=FALSE;
	TCHAR	  *cmd;

	cmd=GetCommandLine();
	sz=lstrlen(cmd);
	if (sz>2 && lstrcmp(cmd+sz-2,_T(" q"))==0)
		fQuit=TRUE;
	else {
		UINT    nk=0;
		TCHAR   *qq=cmd+sz;

		while (qq>cmd && qq[-1]>=_T('0') && qq[-1]<=_T('9'))
			--qq;
		while (*qq)
			nk=nk*10+*qq++-_T('0');
		if (nk)
			g_key=nk;
	}

	g_hEvent=CreateEvent(NULL,TRUE,FALSE,_T("HaaliLSwitch"));
	if (g_hEvent==NULL)
		failed(_T("CreateEvent()"));
	if (GetLastError()==ERROR_ALREADY_EXISTS) {
		if (fQuit) {
			SetEvent(g_hEvent);
			goto quit;
		}
		failedx(_T("LSwitch is already running!"));
	}

	if (fQuit)
		failedx(_T("LSwitch is not running!"));

	sz=GetModuleFileName(NULL,g_prog_dir,MAX_PATH);
	if (sz==0)
		failed(_T("GetModuleFileName()"));
	if (sz==MAX_PATH)
		failedx(_T("Module file name is too long."));
	while (sz>0 && g_prog_dir[sz-1]!=_T('\\'))
		--sz;
	g_prog_dir_len=sz;

	if (SetTimer(NULL,0,500,TimerCb)==0)
		failed(_T("SetTimer()"));

	g_khook=SetWindowsHookEx(WH_KEYBOARD_LL,KbdHook,GetModuleHandle(0),0);
	if (g_khook==0)
		failed(_T("SetWindowsHookEx()"));

	while (GetMessage(&msg,0,0,0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnhookWindowsHookEx(g_khook);
quit:
	CloseHandle(g_hEvent);

	ExitProcess(0);
}
