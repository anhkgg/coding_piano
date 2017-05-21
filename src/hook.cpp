//#include "stdafx.h"
#include "hook.h"
#include <stdio.h>

#pragma data_seg(".CODING_PIANO")
HHOOK g_Hook = NULL;
#pragma data_seg()
#pragma comment(linker, "/section:.CODING_PIANO,rws")

typedef struct _KeyboardLL_Msg
{
	int nCode;
	WPARAM wParam;
	KBDLLHOOKSTRUCT lParam;
}KeyboardLL_Msg, *PKeyboardLL_Msg;

bool keydown = true;
HWND FindFreepiano()
{
	HWND hwnd = FindWindow("FreePianoMainWindow", "Wispow Freepiano 2");
	if(hwnd == NULL)
	{
		hwnd =  FindWindow("FreePianoMainWindow", NULL);
		if(hwnd == NULL)
		{
			hwnd =  FindWindow(NULL, "Wispow Freepiano 2");
		}
	}

	return hwnd;
}

typedef
LRESULT (CALLBACK* pfn_LowLevelKeyboardProc)(  int nCode,     // hook code
									  WPARAM wParam, // message identifier  
									  LPARAM lParam  // message data
									  );

pfn_LowLevelKeyboardProc g_LowLevelKeyboardProc = (pfn_LowLevelKeyboardProc)0x49b60;

typedef LRESULT (WINAPI* pfn_WindProc)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

pfn_WindProc g_WndProc = NULL;


LRESULT WINAPI fakeWindowProc(
					   HWND hWnd,              // handle to window
					   UINT Msg,               // message
					   WPARAM wParam,          // first message parameter
					   LPARAM lParam           // second message parameter
					   )
{

	if(Msg == WM_COPYDATA)
	{
		COPYDATASTRUCT* CopyData = (COPYDATASTRUCT*)lParam;	
		//if(CopyData->cbData == sizeof(KeyboardLL_Msg))
		{
			KeyboardLL_Msg* Msg = (KeyboardLL_Msg*)CopyData->lpData;
			g_LowLevelKeyboardProc(Msg->nCode, Msg->wParam, (LPARAM)&Msg->lParam);
		}
	}
	return g_WndProc(hWnd, Msg, wParam, lParam);
}

void HookWinProc()
{
	while(1)
	{
		HWND hwnd = FindFreepiano();
		if(hwnd)
		{
			g_WndProc = (pfn_WindProc)GetWindowLong(hwnd, GWL_WNDPROC);
			if(g_WndProc)
			{
				SetWindowLong(hwnd, GWL_WNDPROC, (LONG)fakeWindowProc);
				break;
			}
		}
	}
}

LRESULT CALLBACK LowLevelKeyboardProc(  int nCode,     // hook code
									  WPARAM wParam, // message identifier  
									  LPARAM lParam  // message data
									  )
{

	COPYDATASTRUCT CopyData = {0};
	KeyboardLL_Msg Msg = {0};

	Msg.nCode = nCode;
	Msg.wParam = wParam;
	memcpy(&Msg.lParam, (char*)lParam, sizeof(KBDLLHOOKSTRUCT));

	CopyData.cbData = sizeof(KeyboardLL_Msg);
	CopyData.dwData = 0;
	CopyData.lpData = &Msg;

	HWND hwnd = FindFreepiano();
	if(hwnd)
	{
		SendMessage(hwnd, WM_COPYDATA, (WPARAM)hwnd, (LPARAM)&CopyData);
	}

	return CallNextHookEx(g_Hook, nCode, wParam, lParam);
}

BOOL Hook(HMODULE hMod)
{
	if(g_Hook== NULL)
	{
		HMODULE hExe = GetModuleHandle(NULL);

		g_LowLevelKeyboardProc = (pfn_LowLevelKeyboardProc)((DWORD)hExe + (DWORD)g_LowLevelKeyboardProc);

		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)HookWinProc, NULL, 0, NULL);
	}

	g_Hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hMod, 0);

	return g_Hook?TRUE:FALSE;
}

VOID Unhook()
{
	if(g_Hook)
	{
		UnhookWindowsHookEx(g_Hook);
	}
}
