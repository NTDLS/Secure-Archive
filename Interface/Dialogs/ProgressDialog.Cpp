#ifndef _ProgressDialog_CPP
#define _ProgressDialog_CPP
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
#include "../Dialogs/ProgressDialog.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------(Variable Declarations)
HWND hProgressDialog = NULL;
HWND hPackingStatusText = NULL;
HWND hPackingActionText = NULL;
HWND hProgressStatusBar  = NULL;
HWND hProgressMultiButton  = NULL;
HWND hProgressFileStatusBar = NULL;
PROGRESSTRAYINFO ProgressPTI;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ProgressDialog_Cancel(HWND hWnd, bool bForce)
{
	SuspendThread(hWorkerProc);

	int iResult = IDYES;

	if(!bForce)
	{
		iResult = SafeMsgBox(hWnd,
			"Are you sure you want to cancel the current process?",
			gsTitleCaption, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);
	}

	if(iResult == IDYES)
	{
		EnableWindow(hProgressMultiButton, FALSE);
		Set_Text(hPackingStatusText, "Please wait... Canceling.");
		gdwCurrentJobType = JOB_TYPE_CANCEL;
	}

	ResumeThread(hWorkerProc);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT RunProgressDialog(HWND hOwner)
{
	return DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_PROGRESS), hOwner, ProgressDialog);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool EndProgressDialog(int iReturnCode)
{
	if(gTrayIcon.DoesIconExist())
	{
		if(gbAutoClose)
		{
			gTrayIcon.Destroy();
		}
		else{
			SendMessage(gTrayIcon.hTrayHandle, WM_COMMAND, MAKELONG(ID_TRAY_RESTORE, 0), 0);
		}
		Sleep(100);
	}

	bool bResult = (EndDialog(hProgressDialog, iReturnCode) > 0);
	hProgressDialog = NULL;
	return bResult;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK ProgressDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //--------------------------------------------------------------------------

#ifndef _SELFEXTRACTOR_APP
	static HWND hAnimation = NULL;
#endif

    //--------------------------------------------------------------------------

    if(uMsg == WM_INITDIALOG) // Received A Initilize Dialog Message
    {
        hProgressDialog = hWnd;

		fFileProgress = 0;

        HMENU hSystemMenu = GetSystemMenu(hWnd, FALSE);
        AppendMenu(hSystemMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hSystemMenu, MF_STRING, WM_ABOUT_DIALOG, "About");

        SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Please wait...");
        SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN)) );

#ifndef _SELFEXTRACTOR_APP
        hAnimation				= GetDlgItem(hWnd, IDC_AVIFRAME);
#endif
        hPackingActionText		= GetDlgItem(hWnd, IDC_ACTION);
        hProgressMultiButton	= GetDlgItem(hWnd, IDC_MULBUTTON);
        hProgressStatusBar		= GetDlgItem(hWnd, IDC_PROGRESS);
        hProgressFileStatusBar	= GetDlgItem(hWnd, IDC_FILEPROGRESS);
		hPackingStatusText		= GetDlgItem(hWnd, IDC_STATUSTEXT);

		SendMessage(hProgressFileStatusBar, PBM_SETRANGE32, 0, (LPARAM) 100);
		
		if(gdwCurrentJobType == JOB_TYPE_ADDING_FILES)
		{
			Set_Text(hProgressMultiButton, "Stop");
		}

#ifndef _SELFEXTRACTOR_APP
		if(gdwCurrentJobType == JOB_TYPE_PACKING_FOLDER
			|| gdwCurrentJobType == JOB_TYPE_PACKING_FILE
			|| gdwCurrentJobType == JOB_TYPE_ADDING_FILES)
        {
	        SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Encrypting");
			Animate_Open(hAnimation, MAKEINTRESOURCE(IDR_ENCRYPTAVI));
        }
        else if(gdwCurrentJobType == JOB_TYPE_DELETE_SELECTED) {
	        SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Deleting");
            Animate_Open(hAnimation, MAKEINTRESOURCE(IDR_DELETEAVI));
        }
		else if(gdwCurrentJobType == JOB_TYPE_EXTRACT_ALL
			|| gdwCurrentJobType == JOB_TYPE_EXTRACT_SELECTED)
		{
			SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Decrypting");
            Animate_Open(hAnimation, MAKEINTRESOURCE(IDR_DECRYPTAVI));
		}
		else if(gdwCurrentJobType == JOB_TYPE_CREATE_SELF_EXTRACTOR)
		{
	        SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Merging");
            Animate_Open(hAnimation, MAKEINTRESOURCE(IDR_ENCRYPTAVI));
		}
		else if(gdwCurrentJobType == JOB_TYPE_TEST_ARCHIVE)
		{
	        SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Testing");
            Animate_Open(hAnimation, MAKEINTRESOURCE(IDR_DECRYPTAVI));
		}
        else{
			SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Working");
            Animate_Open(hAnimation, MAKEINTRESOURCE(IDR_DECRYPTAVI));
        }

		ShowWindow(hAnimation, SW_SHOW);
#else
		SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Decrypting");
#endif

        SendMessage(hProgressStatusBar, PBM_SETPOS, 0, (LPARAM) 0);

        CenterOverOwner(hWnd);

		if(!IsWindowVisible(GetParent(hWnd)))
		{
			ShowWindowInTaskbar(hWnd, true);
		}

		if(gbAlwaysRunInBackground)
		{
			CheckDlgButton(hWnd, IDC_RUNINBACKGROUND, BST_CHECKED);
			SetThreadPriority(hWorkerProc, THREAD_PRIORITY_IDLE);
		}

		fPackageProgress = 0;
		fFileProgress = 0;

		ResumeThread(hWorkerProc);

        SetTimer(hWnd, 0, 100, NULL);

        return TRUE; // Return TRUE to set the keyboard focus, Otherwise return FALSE
    }

    //--------------------------------------------------------------------------

	/*
    else if(uMsg == WM_CTLCOLORSTATIC)
    {

		if((HANDLE)lParam == hAnimation)
		{
			static HBRUSH ReUsableBrush = NULL;
			DWORD dwBackground = GetSysColor(COLOR_3DFACE);
			if(ReUsableBrush)
			{
				DeleteObject(ReUsableBrush);
			}

			ReUsableBrush = CreateSolidBrush(dwBackground);

			//SetBkMode((HDC) wParam, TRANSPARENT);
			SetBkColor((HDC)wParam, dwBackground);

			return (LRESULT)ReUsableBrush;
		}
        return FALSE;
    }
	*/
	
	//--------------------------------------------------------------------------

    else if(uMsg == WM_QUERYENDSESSION)
    {
		ProgressDialog_Cancel(hWnd, true);
		Sleep(2500);
		PostQuitMessage(0);
		return FALSE;
    }

	//--------------------------------------------------------------------------

    else if(uMsg == WM_TIMER)
    {
		SendMessage(hProgressStatusBar, PBM_SETPOS, (DWORD)fPackageProgress, (LPARAM) 0);
		SendMessage(hProgressFileStatusBar, PBM_SETPOS, (DWORD)fFileProgress, (LPARAM) 0);
        return FALSE;
    }

    //--------------------------------------------------------------------------

    else if(uMsg == WM_COMMAND)
    {
        if(wParam == IDC_MULBUTTON || wParam == IDOK || wParam == IDCANCEL)
        {
			ProgressDialog_Cancel(hWnd, false);
			return TRUE;
		}
        else if(wParam == IDC_RUNINBACKGROUND)
        {
			if(IsDlgButtonChecked(hWnd, IDC_RUNINBACKGROUND))
			{
				SetThreadPriority(hWorkerProc, THREAD_PRIORITY_IDLE);
			}
			else{
				SetThreadPriority(hWorkerProc, THREAD_PRIORITY_NORMAL);
			}
			
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
		ProgressDialog_Cancel(hWnd, false);
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

				memset(&ProgressPTI, 0, sizeof(PROGRESSTRAYINFO));
				ProgressPTI.hDialog = hWnd;
				ProgressPTI.hProgressBar = hProgressStatusBar;
				ProgressPTI.bParentInitialVisibility = IsWindowVisible(GetParent(hWnd)) > 0;
				gTrayIcon.UserData = &ProgressPTI;

				ShowWindow(hWnd, SW_HIDE);
				if(ProgressPTI.bParentInitialVisibility)
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

