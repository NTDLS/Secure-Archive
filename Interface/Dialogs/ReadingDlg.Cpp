#ifndef _READINGDLG_CPP
#define _READINGDLG_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "../Source/Package.H"
#include "../Source/Entry.H"
#include "../Source/Routines.H"
#include "../Source/Workers.H"

#include "../Dialogs/MainDlg.H"
#include "../Dialogs/OpenPassDlg.H"
#include "../Dialogs/NetLogoDlg.H"
#include "../Dialogs/ReadingDlg.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------(Variable Declarations)
HWND hReadingDialog = NULL;
HWND hReadingStatusText = NULL;
HWND hReadingStatusBar = NULL;
HWND hReadingMulButton = NULL;
PROGRESSTRAYINFO ReadingPTI;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ReadingDialog_Cancel(HWND hWnd)
{
    if(gdwCurrentJobType == 0)
    {
        EndDialog(hWnd, 0);
        DestroyWindow(hWnd);
    }
    else{
		SuspendThread(hWorkerProc);

		if(SafeMsgBox(hWnd, "Are you sure you want to cancel the current process?", gsTitleCaption, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
		{
			EnableWindow(hReadingMulButton, FALSE);
			Set_Text(hReadingStatusText, "Please wait... Canceling.");
			gdwCurrentJobType = JOB_TYPE_CANCEL;
		}

		ResumeThread(hWorkerProc);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT RunReadingDialog(HWND hOwner)
{
	return DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_READING), hOwner, ReadingDialog);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool EndReadingDialog(int iReturnCode)
{
	if(gTrayIcon.DoesIconExist())
	{
		SendMessage(gTrayIcon.hTrayHandle, WM_COMMAND, MAKELONG(ID_TRAY_RESTORE, 0), 0);
		Sleep(100);
	}

	bool bResult = (EndDialog(hReadingDialog, iReturnCode) > 0);
	hReadingDialog = NULL;
	return bResult;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK ReadingDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if(uMsg == WM_INITDIALOG) // Received A Initilize Dialog Message
    {
        hReadingDialog = hWnd;

        HMENU hSystemMenu = GetSystemMenu(hWnd, FALSE);
        AppendMenu(hSystemMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hSystemMenu, MF_STRING, WM_ABOUT_DIALOG, "About");

        SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Reading");
        SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN)) );

        hReadingStatusText = GetDlgItem(hWnd, IDC_STATUSTEXT);
        hReadingStatusBar = GetDlgItem(hWnd, IDC_PROGRESS);
        hReadingMulButton = GetDlgItem(hWnd, IDC_MULBUTTON);

        SendMessage(hReadingStatusBar, PBM_SETPOS, 0, (LPARAM) 0);

        CenterOverOwner(hWnd);

		if(!IsWindowVisible(GetParent(hWnd)))
		{
			ShowWindowInTaskbar(hWnd, true);
		}

        ResumeThread(hWorkerProc);

        SetTimer(hWnd, 0, 100, NULL);

        return TRUE; // Return TRUE to set the keyboard focus, Otherwise return FALSE
    }

    //--------------------------------------------------------------------------

    else if(uMsg == WM_TIMER)
    {
        SendMessage(hReadingStatusBar, PBM_SETPOS, (DWORD)fPackageProgress, (LPARAM) 0);
        return FALSE;
    }

    //--------------------------------------------------------------------------

    else if(uMsg == WM_COMMAND)
    {
        if(wParam == IDC_MULBUTTON || wParam == IDOK || wParam == IDCANCEL)
        {
			ReadingDialog_Cancel(hWnd);
            return TRUE;
        }

        return FALSE;
    }

    //--------------------------------------------------------------------------

	else if(uMsg == WM_PAINT)
    {
        HDC ThisHDC;
        PAINTSTRUCT ThisPS;

        ThisHDC = BeginPaint(hWnd, &ThisPS);

        // Any painting should be done here

        EndPaint(hWnd, &ThisPS);
        return TRUE;
    }

    //--------------------------------------------------------------------------

    else if(uMsg == WM_SYSCOMMAND) // Received A System Menu Message
    {
        if(LOWORD(wParam) == WM_ABOUT_DIALOG) //- About (SysMenu Command)
        {
            _AboutDialogInfo ADI;
    
            ADI.DisplayIcon  = LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN));
            ADI.TitleCaption = gsTitleCaption;
            ADI.FileVersion  = gsFileVersion;
            ADI.BuildDate    = __DATE__;
            ADI.BuildTime    = __TIME__;
            ADI.CopyRight    = gsAppCopyRight;
            ADI.OwnerHandle  = hWnd;

            NetLogo(&ADI);
            return TRUE;
        }

        return FALSE;
    }

    //--------------------------------------------------------------------------

    else if(uMsg == WM_CLOSE)  // Received Close Message
    {
		ReadingDialog_Cancel(hWnd);
        return TRUE;
    }

    //--------------------------------------------------------------------------

	else if(uMsg == WM_SIZE)
	{
		if(wParam == SIZE_MINIMIZED || wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
		{
			if(wParam == SIZE_MINIMIZED)
			{
				HMENU hPopupMenu = CreateMenu();
				AppendMenu(hPopupMenu, MF_STRING, ID_TRAY_RESTORE, "&Restore");

				memset(&ReadingPTI, 0, sizeof(PROGRESSTRAYINFO));
				ReadingPTI.hDialog = hWnd;
				ReadingPTI.hProgressBar = hReadingStatusBar;
				ReadingPTI.bParentInitialVisibility = IsWindowVisible(GetParent(hWnd)) > 0;
				gTrayIcon.UserData = &ReadingPTI;

				ShowWindow(hWnd, SW_HIDE);
				if(ReadingPTI.bParentInitialVisibility)
				{
					ShowWindow(GetParent(hWnd), SW_HIDE);
				}

				gTrayIcon.Create(IDI_TRAYSTAT_00, gsTitleCaption, hPopupMenu, &ProgressTrayIconProc);
			}
		}
	}

    //--------------------------------------------------------------------------

	Sleep(0);
	
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

