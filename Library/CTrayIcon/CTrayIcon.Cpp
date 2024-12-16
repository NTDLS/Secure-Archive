///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright © NetworkDLS 2002, All rights reserved
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _CTRAYICON_CPP
#define _CTRAYICON_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <Stdlib.H>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "CTrayIcon.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
Usage Example:

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TrayIconProc(CTrayIcon *lpTI, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg == WM_CREATE)
    {
		//Init code.
    }
	else if(uMsg == WM_COMMAND)
    {
		if(LOWORD(wParam) == ID_EXIT)
        {
            DestroyWindow(hWnd);
        }
		else if(LOWORD(wParam) == ID_CANCEL)
        {
			//Do nothing.
        }
    }
    else if(uMsg == WM_SHELLNOTIFY)
    {
		if(lParam == WM_LBUTTONDBLCLK)
		{
			if(IsWindow(hMainDialog))
			{
				SetForegroundWindow(hMainDialog);
				SetActiveWindow(hMainDialog);
			}
			else{
				DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDialog);
			}
		}
    }
    else if(uMsg == lpTI->WM_TASKBARCREATED)
    {
		//Windows explorer just started up.
		//The original icon and tip will be re-added, do what you need to do here to update it.
    }
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK CTrayIconProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CTrayIcon *lpTI = (CTrayIcon *) GetProp(hWnd, "ClassProperties");

    if(uMsg == WM_SHELLNOTIFY)
    {
		if(lParam == WM_RBUTTONUP)
        {
			POINT PT;
			GetCursorPos(&PT);
            SetForegroundWindow(hWnd);
            TrackPopupMenuEx(lpTI->hPopupMenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, PT.x, PT.y, hWnd, NULL);
            PostMessage(hWnd, WM_NULL, 0, 0);
        }
    }
    else if(uMsg == WM_DESTROY)
    {
		RemoveProp(hWnd, "ClassProperties");
		PostQuitMessage(0);
    }
    else if(uMsg == WM_CLOSE)
    {
        DestroyWindow(hWnd);
    }

	if(lpTI != NULL)
	{
		if(uMsg == lpTI->WM_TASKBARCREATED)
		{
			Shell_NotifyIcon(NIM_ADD, &lpTI->OriginalTrayNotify);
		}

		lpTI->pUserCallback(lpTI, hWnd, uMsg, wParam, lParam); 
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef UNICODE
bool CTrayIcon::Create(int iIcon, const WCHAR *sTitle, HMENU hInPopupMenu, UserCallback lpUserCallback)
#else
bool CTrayIcon::Create(int iIcon, const char *sTitle, HMENU hInPopupMenu, UserCallback lpUserCallback)
#endif
{
    WNDCLASS WC;
    MSG Msg;

	#ifdef UNICODE
	WCHAR sClass[255];
	wsprintf(sClass, L"CTrayIcon%s", sTitle);
	#else
	char sClass[255];
	sprintf_s(sClass, sizeof(sClass), "CTrayIcon%s", sTitle);
	#endif

	hAppInstance = (HINSTANCE) GetModuleHandle(NULL);

	this->WM_TASKBARCREATED = RegisterWindowMessage("TaskbarCreated");

	this->pUserCallback = lpUserCallback;

    WC.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	WC.lpfnWndProc   = CTrayIconProc;
    WC.cbClsExtra    = 0;
    WC.cbWndExtra    = 0;
    WC.hInstance     = hAppInstance;
    WC.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
    WC.hCursor       = LoadCursor(NULL, IDC_ARROW);
    WC.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    WC.lpszMenuName  = NULL;
    WC.lpszClassName = sClass;

    RegisterClass(&WC);

    hTrayHandle = CreateWindow(
		sClass,
		sTitle,
		WS_POPUP | WS_SYSMENU,
		0, 0, 0, 0,
		NULL,
		(HMENU) NULL,
		hAppInstance,
		NULL
	);

	SetProp(hTrayHandle, "ClassProperties", this);

	memset(&this->OriginalTrayNotify, 0, sizeof(this->OriginalTrayNotify));
	this->OriginalTrayNotify.cbSize			= sizeof(NOTIFYICONDATA);
    this->OriginalTrayNotify.hWnd				= hTrayHandle;
	this->OriginalTrayNotify.uID				= 0;
    this->OriginalTrayNotify.hIcon				= LoadIcon(hAppInstance, MAKEINTRESOURCE(iIcon) );
    this->OriginalTrayNotify.uFlags			= NIF_ICON | NIF_TIP | NIF_MESSAGE;
    this->OriginalTrayNotify.uCallbackMessage	= WM_SHELLNOTIFY;

	#ifdef UNICODE
	wcscpy_s(this->OriginalTrayNotify.szTip, sizeof(this->OriginalTrayNotify.szTip), sTitle);
	#else
	strcpy_s(this->OriginalTrayNotify.szTip, sizeof(this->OriginalTrayNotify.szTip), sTitle);
	#endif

    Shell_NotifyIcon(NIM_ADD, &this->OriginalTrayNotify);

	HMENU hMainMenu = CreateMenu();
	hPopupMenu = hInPopupMenu;
	#ifdef UNICODE
    InsertMenu(hMainMenu, 0, MF_POPUP, (UINT_PTR)hPopupMenu, L"invisible menu");
	#else
    InsertMenu(hMainMenu, 0, MF_POPUP, (UINT_PTR)hPopupMenu, "invisible menu");
	#endif

	this->pUserCallback(this, this->hTrayHandle, WM_CTRAYICON_INITIALIZE, 0, 0);

    while(GetMessage(&Msg, NULL, 0, 0))
    {
        if(!IsWindow(hTrayHandle) || !IsDialogMessage(hTrayHandle, &Msg))
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
		Sleep(1);
    }

	UnregisterClass(sClass, hAppInstance);
	Shell_NotifyIcon(NIM_DELETE, &this->OriginalTrayNotify);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CTrayIcon::Update(int iIcon)
{
	NOTIFYICONDATA TrayNotify;
	memset(&TrayNotify, 0, sizeof(TrayNotify));

	TrayNotify.cbSize= sizeof(NOTIFYICONDATA);
    TrayNotify.hWnd = hTrayHandle;
    TrayNotify.uFlags = NIF_ICON;
	TrayNotify.hIcon = LoadIcon(hAppInstance, MAKEINTRESOURCE(iIcon));

    return (Shell_NotifyIcon(NIM_MODIFY, &TrayNotify) != 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef UNICODE
bool CTrayIcon::Update(const WCHAR *sTitle)
#else
bool CTrayIcon::Update(const char *sTitle)
#endif
{
	NOTIFYICONDATA TrayNotify;
	memset(&TrayNotify, 0, sizeof(TrayNotify));

	TrayNotify.cbSize= sizeof(NOTIFYICONDATA);
    TrayNotify.hWnd = hTrayHandle;
    TrayNotify.uFlags = NIF_TIP;

	#ifdef UNICODE
	wcscpy_s(TrayNotify.szTip, sizeof(TrayNotify.szTip), sTitle);
	#else
	strcpy_s(TrayNotify.szTip, sizeof(TrayNotify.szTip), sTitle);
	#endif

    return (Shell_NotifyIcon(NIM_MODIFY, &TrayNotify) != 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef UNICODE
bool CTrayIcon::Update(int iIcon, const WCHAR *sTitle)
#else
bool CTrayIcon::Update(int iIcon, const char *sTitle)
#endif
{
	NOTIFYICONDATA TrayNotify;
	memset(&TrayNotify, 0, sizeof(TrayNotify));

	TrayNotify.cbSize= sizeof(NOTIFYICONDATA);
    TrayNotify.hWnd = hTrayHandle;
    TrayNotify.uFlags = NIF_ICON | NIF_TIP;
	TrayNotify.hIcon = LoadIcon(hAppInstance, MAKEINTRESOURCE(iIcon));

	#ifdef UNICODE
	wcsncpy_s(TrayNotify.szTip, sizeof(TrayNotify.szTip), sTitle, 63);
	#else
	strncpy_s(TrayNotify.szTip, sizeof(TrayNotify.szTip), sTitle, 63);
	#endif

	return (Shell_NotifyIcon(NIM_MODIFY, &TrayNotify) != 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CTrayIcon::Destroy(void)
{
	NOTIFYICONDATA TrayNotify;
	memset(&TrayNotify, 0, sizeof(TrayNotify));

	TrayNotify.cbSize= sizeof(NOTIFYICONDATA);
    TrayNotify.hWnd = hTrayHandle;

	SendMessage(hTrayHandle, WM_CLOSE, 0, 0);

	this->hTrayHandle = NULL;
	this->hPopupMenu = NULL;

	return (Shell_NotifyIcon(NIM_DELETE, &TrayNotify) != 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CTrayIcon::DoesIconExist(void)
{
	if(this->hTrayHandle)
	{
		return(IsWindow(this->hTrayHandle) > 0);
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
