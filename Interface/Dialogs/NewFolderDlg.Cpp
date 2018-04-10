#ifndef _NewFolderDlg_CPP
#define _NewFolderDlg_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "../Source/Package.H"
#include "../Source/Entry.H"
#include "../Source/Routines.H"

#include "../Dialogs/MainDlg.H"
#include "../Dialogs/NewFolderDlg.H"
#include "../Dialogs/NetLogoDlg.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------(Variable Declarations)
HWND hNewFolderDialog = NULL;

char sNewFolderName[MAX_PATH + 1];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK NewFolderDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //--------------------------------------------------------------------------

	static HWND hFolderName = NULL;

    //--------------------------------------------------------------------------

    if(uMsg == WM_INITDIALOG) // Received A Initilize Dialog Message
    {
        hNewFolderDialog = hWnd;

        HMENU hSystemMenu = GetSystemMenu(hWnd, FALSE);
        AppendMenu(hSystemMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hSystemMenu, MF_STRING, WM_ABOUT_DIALOG, "About");

        SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"New Folder");
        SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN)) );

        hFolderName = GetDlgItem(hWnd, IDC_FOLDERNAME);

        CenterOverOwner(hWnd);

		ShowWindowInTaskbar(hWnd, false);

		return TRUE; // Return TRUE to set the keyboard focus, Otherwise return FALSE
    }

    //--------------------------------------------------------------------------

    if(uMsg == WM_COMMAND)
    {
        if(wParam == IDC_OK || wParam == IDOK)
        {
			int iLength = Get_Text(hFolderName, sNewFolderName, MAX_PATH);
			if(iLength <= 0 || iLength >= MAX_PATH)
			{
				SafeMsgBox(hWnd, "You must apecify a valid folder name.", gsTitleCaption, MB_ICONINFORMATION);
				return TRUE;
			}

			if(DoesPathContainUnicode(sNewFolderName, iLength))
			{
				SafeMsgBox(hWnd,
					"The name of the folder you are trying to create contains\r\n"
					"UNICODE characters, which are not supported.",
					gsTitleCaption, MB_ICONASTERISK);
					return TRUE;
			}
			
			EndDialog(hWnd, 1);
            return TRUE;
        }

        if(wParam == IDC_CANCEL || wParam == IDCANCEL)
        {
	        EndDialog(hWnd, 0);
			DestroyWindow(hWnd);
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

