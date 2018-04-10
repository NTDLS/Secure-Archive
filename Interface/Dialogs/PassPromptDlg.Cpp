#ifndef _PassPromptDlg_CPP
#define _PassPromptDlg_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "../Resources/Resource.H"

#include "../Source/Package.H"
#include "../Source/Entry.H"

#include "../Dialogs/MainDlg.H"
#include "../Dialogs/PassPromptDlg.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------(Variable Declarations)
HWND PassPromptDialog_hWnd = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK PassPromptDialog(HWND xHandle, UINT xMessage, WPARAM wParam, LPARAM lParam)
{
    //--------------------------------------------------------------------------

    static HMENU SystemMenu = NULL;

	static HWND Psssword1_hWnd = NULL;
	static HWND Psssword2_hWnd = NULL;

    static HWND PasswordBits_hWnd = NULL;
    static HWND PasswordBitsBar_hWnd = NULL;

	//--------------------------------------------------------------------------

    if(xMessage == WM_INITDIALOG) // Received A Initilize Dialog Message
    {
        PassPromptDialog_hWnd = xHandle;

        SystemMenu = GetSystemMenu(xHandle, FALSE);
        AppendMenu(SystemMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(SystemMenu, MF_STRING, 1000, "About");

        SendMessage(xHandle, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)gsTitleCaption);
        SendMessage(xHandle, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN)) );

        Psssword1_hWnd = GetDlgItem(xHandle, IDC_PWD1);
        Psssword2_hWnd = GetDlgItem(xHandle, IDC_PWD2);

        CenterWindow(xHandle);

		return TRUE; // Return TRUE to set the keyboard focus, Otherwise return FALSE
    }

    //--------------------------------------------------------------------------

    if(xMessage == WM_COMMAND)
    {
        if(wParam == IDC_OK)
        {
			int Length = Get_TextEx(Psssword1_hWnd, gsPassword, sizeof(gsPassword));
			if(Length <= 0 || Length > MAX_PATH)
			{
				MessageBox(xHandle, "You must apecify a valid password.", gsTitleCaption, MB_ICONINFORMATION);
				return TRUE;
			}
			
			EndDialog(xHandle, 1);
            return TRUE;
        }

        if(wParam == IDC_CANCEL)
        {
	        EndDialog(xHandle, 0);
			DestroyWindow(xHandle);
            return TRUE;
        }

		return FALSE;
    }

    //--------------------------------------------------------------------------

	if(xMessage == WM_PAINT)
    {
        HDC ThisHDC;
        PAINTSTRUCT ThisPS;

        ThisHDC = BeginPaint(xHandle, &ThisPS);

        // Any painting should be done here

        EndPaint(xHandle, &ThisPS);
        return TRUE;
    }

    //--------------------------------------------------------------------------

    if(xMessage == WM_SYSCOMMAND) // Received A System Menu Message
    {
        if(LOWORD(wParam) == 1000) //- About (SysMenu Command)
        {
            _AboutDialogInfo ADI;
    
            ADI.DisplayIcon  = LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN));
            ADI.TitleCaption = gsTitleCaption;
            ADI.FileVersion  = gsFileVersion;
            ADI.BuildDate    = __DATE__;
            ADI.BuildTime    = __TIME__;
            ADI.CopyRight    = gsAppCopyRight;
            ADI.OwnerHandle  = xHandle;

            NetLogo(&ADI);
            return TRUE;
        }

        return FALSE;
    }

    //--------------------------------------------------------------------------

    if(xMessage == WM_CLOSE)  // Received Close Message
    {
        EndDialog(xHandle, 0);
        DestroyWindow(xHandle);
        return TRUE;
    }

    //--------------------------------------------------------------------------

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

