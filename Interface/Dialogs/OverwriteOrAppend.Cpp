#ifndef _OverwriteOrAppend_CPP
#define _OverwriteOrAppend_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "../Source/Package.H"
#include "../Source/Entry.H"
#include "../Source/Routines.H"

#include "../Dialogs/MainDlg.H"
#include "../Dialogs/NetLogoDlg.H"
#include "../Dialogs/OverwriteOrAppend.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK OverwriteOrAppendDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //--------------------------------------------------------------------------

    if(uMsg == WM_INITDIALOG) // Received A Initilize Dialog Message
    {
        HMENU hSystemMenu = GetSystemMenu(hWnd, FALSE);
        AppendMenu(hSystemMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hSystemMenu, MF_STRING, WM_ABOUT_DIALOG, "About");

        SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Overwrite / Append");
        SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN)) );

        CenterOverOwner(hWnd);

		ShowWindowInTaskbar(hWnd, false);

		return TRUE; // Return TRUE to set the keyboard focus, Otherwise return FALSE
    }

    //--------------------------------------------------------------------------

    if(uMsg == WM_COMMAND)
    {
        if(wParam == IDC_OVERWRITE)
        {
			if(SafeMsgBox(hWnd, "Are you sure you want to overwrite the existing file?",
				gsTitleCaption, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
			{
				EndDialog(hWnd, RESULT_OVERWRITE);
			}
            return TRUE;
        }
		else if(wParam == IDC_APPEND || wParam == IDOK)
        {
			EndDialog(hWnd, RESULT_APPEND);
            return TRUE;
        }
        else if(wParam == IDC_CANCEL || wParam == IDCANCEL)
        {
	        EndDialog(hWnd, RESULT_CANCEL);
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
        EndDialog(hWnd, RESULT_CANCEL);
        DestroyWindow(hWnd);
        return TRUE;
    }

    //--------------------------------------------------------------------------

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

