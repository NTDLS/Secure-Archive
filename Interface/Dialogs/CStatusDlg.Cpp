#ifndef _CStatusDlg_CPP
#define _CStatusDlg_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <Stdlib.H>
#include <CommCtrl.h>

#include "../Source/Entry.H"
#include "CStatusDlg.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CStatusDlg::CStatusDlg()
{
	hEvent = NULL;
	hDialogHandle = NULL;
	hOwnerHandle = NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CStatusDlg::~CStatusDlg()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI CStatusDlg_Thread(LPVOID lpData)
{
	CStatusDlg *lpDlg = ((CStatusDlg *) lpData);
	
    BOOL bResult = FALSE;
    MSG MyMsg;

	memset(&MyMsg, 0, sizeof(MyMsg));

	lpDlg->hDialogHandle = CreateDialog(ghAppInstance, MAKEINTRESOURCE(IDD_STATUSDLG), lpDlg->hOwnerHandle, CStatusDlgProc);
	
	SetEvent(lpDlg->hEvent);

	if(lpDlg->hDialogHandle == NULL)
	{
		MessageBox(GetActiveWindow(), "Failed to load the status dialog box.", gsTitleCaption, 0);
		return 0;
	}

	EnableWindow(lpDlg->hOwnerHandle, FALSE);

	while( (bResult = GetMessage(&MyMsg, lpDlg->hDialogHandle, 0, 0)) != 0 )
    {
        if(bResult == -1)
        {
    		MessageBox(NULL, "GetMessage Error.", gsTitleCaption, MB_SYSTEMMODAL);
            lpDlg->hDialogHandle = NULL;
			return 0;
            // handle the error and possibly exit.
        }
        else if(!IsWindow(lpDlg->hDialogHandle) || !IsDialogMessage(lpDlg->hDialogHandle, &MyMsg))
        {
            TranslateMessage(&MyMsg);
            DispatchMessage(&MyMsg);
        }
    }

	EnableWindow(lpDlg->hOwnerHandle, TRUE);
	SetForegroundWindow(lpDlg->hOwnerHandle);

	lpDlg->hDialogHandle = NULL;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CStatusDlg::SetButtonText(char *sText)
{
	Set_Text(hCmdCancel, sText);
	return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CStatusDlg::EnableButton(void)
{
	EnableWindow(hCmdCancel, true);
	return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CStatusDlg::DisableButton(void)
{
	EnableWindow(hCmdCancel, false);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CStatusDlg::SetProgressPos(long lPos)
{
	SendMessage(hProgress, (UINT) PBM_SETPOS, lPos, 0);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CStatusDlg::SetProgressRange(int iMin, int iMax)
{
	SendMessage(hProgress, (UINT) PBM_SETRANGE32, iMin, iMax);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CStatusDlg::SetText(char *sText)
{
	Set_Text(hText, sText);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CStatusDlg::Unload(void)
{
	EndDialog(hDialogHandle, 0);
	
	while(IsWindow(hDialogHandle))
	{
		Sleep(1);
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HWND CStatusDlg::Load(HWND hOwner, char *sTitle, int iIcon, bool bAllowCancel)
{
	hOwnerHandle = hOwner;
	
	hEvent = CreateEvent(NULL, TRUE, FALSE, "CStatusDlg_Thread_Function");

	CStatusDlg_Thread_Handle = CreateThread(NULL, 0, CStatusDlg_Thread, (LPVOID)this, 0, &CStatusDlg_Thread_ID);

    if(WaitForSingleObject(hEvent, 5000) == WAIT_TIMEOUT)
    {
        CloseHandle(hEvent);
        return NULL;
    }

    SendMessage(hDialogHandle, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)sTitle);
    SendMessage(hDialogHandle, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(iIcon)));

	hText      = GetDlgItem(hDialogHandle, IDC_STATTEXT);
	hProgress  = GetDlgItem(hDialogHandle, IDC_STATPROGRESS);
	hCmdCancel = GetDlgItem(hDialogHandle, IDC_CANCEL);

	if(!bAllowCancel)
	{
		EnableWindow(hCmdCancel, false);
	}

	CloseHandle(hEvent);

	return hDialogHandle;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CStatusDlg::SetIcon(int iIcon)
{
	SendMessage(hDialogHandle, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(iIcon)));
	return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CStatusDlg::SetCaption(char *sCaption)
{
	SendMessage(hDialogHandle, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)sCaption);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CStatusDlg::Show(void)
{
	return ShowWindow(hDialogHandle, SW_SHOW) == TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CStatusDlg::IsDlg(void)
{
	return IsWindow(hDialogHandle) == TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CStatusDlg::IsDlgVisible(void)
{
	if(IsWindowVisible(hDialogHandle))
	{
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CStatusDlg::Hide(void)
{
	if(ShowWindow(hDialogHandle, SW_HIDE))
	{
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK CStatusDlgProc(HWND xHandle, UINT xMessage, WPARAM wParam, LPARAM lParam)
{
    if(xMessage == WM_INITDIALOG)
    {
		CenterWindow(xHandle);
        return TRUE;
    }

    if(xMessage == WM_COMMAND)
    {
		if(wParam == IDC_CANCEL)
		{
			EndDialog(xHandle, 0);
			PostQuitMessage(0);
			return TRUE;
		}

		return FALSE;
    }

	if(xMessage == WM_PAINT)
    {
        HDC ThisHDC;
        PAINTSTRUCT ThisPS;

        ThisHDC = BeginPaint(xHandle, &ThisPS);

        // Any painting should be done here

        EndPaint(xHandle, &ThisPS);
        return TRUE;
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

