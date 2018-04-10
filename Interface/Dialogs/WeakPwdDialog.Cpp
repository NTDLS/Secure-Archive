#ifndef _WeakPwdDialog_CPP
#define _WeakPwdDialog_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "../Source/Package.H"
#include "../Source/Entry.H"
#include "../Source/Routines.H"

#include "../Dialogs/MainDlg.H"
#include "../Dialogs/WeakPwdDialog.H"
#include "../Dialogs/NetLogoDlg.H"

#include "../../../@Libraries/CNASCCL (Stream)/CNASCCL.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------(Variable Declarations)
int giKeyWeight = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK WeakPwdDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//--------------------------------------------------------------------------

    if(uMsg == WM_INITDIALOG) // Received A Initilize Dialog Message
    {
        HMENU hSystemMenu = GetSystemMenu(hWnd, FALSE);
        AppendMenu(hSystemMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hSystemMenu, MF_STRING, WM_ABOUT_DIALOG, "About");

        SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Weak Password");
        SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN)) );

		char sTemp[1024];

		sprintf_s(sTemp, sizeof(sTemp),
			"The password you entered is classified as \"%s\" which\r\n"
			"does not meet the complexity requirements that you have defined."
			"\r\n\r\nContinue anyway?", gsKeyWeights[giKeyWeight]);

		Set_Text(GetDlgItem(hWnd, IDC_MESSAGE), sTemp);

        CenterOverOwner(hWnd);

		if(!IsWindowVisible(GetParent(hWnd)))
		{
			ShowWindowInTaskbar(hWnd, true);
		}

		return TRUE; // Return TRUE to set the keyboard focus, Otherwise return FALSE
    }

    //--------------------------------------------------------------------------

    if(uMsg == WM_COMMAND)
    {
        if(wParam == IDC_CMD_YES || wParam == IDC_CMD_NO || wParam == IDC_CMD_CHANGE)
        {
			EndDialog(hWnd, wParam);
            return TRUE;
        }

        if(wParam == IDOK)
        {
			EndDialog(hWnd, IDC_CMD_YES);
            return TRUE;
        }
        if(wParam == IDCANCEL)
        {
			EndDialog(hWnd, IDC_CMD_NO);
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

