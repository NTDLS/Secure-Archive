#ifndef _WeakPwdNotificationDialog_CPP
#define _WeakPwdNotificationDialog_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "../Source/Package.H"
#include "../Source/Entry.H"
#include "../Source/Routines.H"

#include "../Dialogs/MainDlg.H"
#include "../Dialogs/WeakPwdNotificationDlg.H"
#include "../Dialogs/NetLogoDlg.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK WeakPwdNotificationDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//--------------------------------------------------------------------------

	static HWND hClasses = NULL;

	//--------------------------------------------------------------------------

    if(uMsg == WM_INITDIALOG) // Received an Initialize Dialog Message
    {
        HMENU hSystemMenu = GetSystemMenu(hWnd, FALSE);
        AppendMenu(hSystemMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hSystemMenu, MF_STRING, WM_ABOUT_DIALOG, "About");

        SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Password Strength");
        SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN)) );

		hClasses = GetDlgItem(hWnd, IDC_CLASSIFICATION);

		for(int iPos = 0; gsKeyWeights[iPos]; iPos++)
		{
			SendMessage(hClasses, (UINT) CB_ADDSTRING, 0, (WPARAM)(char *)gsKeyWeights[iPos]);
		}

		LRESULT iIndex = 0;

		if(gdwWeakEncryptionWarnLevel >= KW_UNACCEPTABLE && gdwWeakEncryptionWarnLevel <= KW_EXCELLENT)
		{
			iIndex = SendMessage(hClasses, (UINT) CB_FINDSTRINGEXACT, 0, (WPARAM)gsKeyWeights[gdwWeakEncryptionWarnLevel]);
		}
		else{
			iIndex = SendMessage(hClasses, (UINT) CB_FINDSTRINGEXACT, 0, (WPARAM)gsKeyWeights[KW_ACCEPTABLE]);
		}
		SendMessage(hClasses, (UINT) CB_SETCURSEL, iIndex, 0);

        CenterOverOwner(hWnd);

		ShowWindowInTaskbar(hWnd, false);

		return TRUE; // Return TRUE to set the keyboard focus, Otherwise return FALSE
    }

    //--------------------------------------------------------------------------

    if(uMsg == WM_COMMAND)
    {
        if(wParam == IDC_CMD_CANCEL || wParam == IDCANCEL)
        {
			EndDialog(hWnd, 0);
            return TRUE;
        }

        if(wParam == IDC_CMD_OK || wParam == IDOK)
        {
			gdwWeakEncryptionWarnLevel = (DWORD)SendMessage(hClasses, (UINT) CB_GETCURSEL, 0, 0);
			EndDialog(hWnd, 0);
            return TRUE;
        }

		return FALSE;
    }

    //--------------------------------------------------------------------------

	if(uMsg == WM_PAINT)
    {
        HDC ThisHDC;
        PAINTSTRUCT ThisPS;

        ThisHDC = BeginPaint(hWnd, &ThisPS);

        // Any painting should be done here

        EndPaint(hWnd, &ThisPS);
        return TRUE;
    }

    //--------------------------------------------------------------------------

    if(uMsg == WM_SYSCOMMAND) // Received A System Menu Message
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

    if(uMsg == WM_CLOSE)  // Received Close Message
    {
        EndDialog(hWnd, 0);
        DestroyWindow(hWnd);
        return TRUE;
    }

    //--------------------------------------------------------------------------

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

