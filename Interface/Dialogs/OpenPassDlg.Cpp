#ifndef _OPENPASSDLG_CPP
#define _OPENPASSDLG_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "../Source/Package.H"
#include "../Source/Entry.H"
#include "../Source/Routines.H"
#include "../Source/HardKey.H"

#include "../Dialogs/MainDlg.H"
#include "../Dialogs/OpenPassDlg.H"
#include "../Dialogs/NetLogoDlg.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 
using namespace NSWFL::String; 
using namespace NSWFL::Conversion; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------(Variable Declarations)
HWND hOpenPassDialog = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK OpenPassDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //--------------------------------------------------------------------------

    static HWND hPassword = NULL;
    static HWND hPasswordBits    = NULL;
	static char sHardKeyFile[MAX_PATH];

    //--------------------------------------------------------------------------

    if(uMsg == WM_INITDIALOG) // Received an Initialize Dialog Message
    {
        hOpenPassDialog = hWnd;

        HMENU hSystemMenu = GetSystemMenu(hWnd, FALSE);
        AppendMenu(hSystemMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hSystemMenu, MF_STRING, WM_ABOUT_DIALOG, "About");

        SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Password");
        SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN)) );

        hPassword = GetDlgItem(hWnd, IDC_PASSWORD);
        hPasswordBits = GetDlgItem(hWnd, IDC_PASSWORDBITS1);

		SendMessage(hPassword, EM_LIMITTEXT, 0, 0);

        if(gPassword.iLength > 0)
		{
			Set_Text(hPassword, gPassword.sPassword);
		}

        CenterOverOwner(hWnd);

		if(!IsWindowVisible(GetParent(hWnd)))
		{
			ShowWindowInTaskbar(hWnd, true);
		}

        return TRUE; // Return TRUE to set the keyboard focus, Otherwise return FALSE
    }

    //--------------------------------------------------------------------------

    if(uMsg == 307) // Key Down
    {
        char lsTemp0[64];
        char lsTemp1[64];

        sprintf_s(lsTemp0, sizeof(lsTemp0), " (%s Bits)",
			FormatInteger(lsTemp1, sizeof(lsTemp1), (long)(Get_TextLength(hPassword) * 8)));
        Set_Text(hPasswordBits, lsTemp0);
    }

    //--------------------------------------------------------------------------

    if(uMsg == WM_COMMAND)
    {
        if(wParam == IDC_OK || wParam == IDOK)
        {
            int iLength = Get_TextLength(hPassword);
            if(iLength <= 0)
            {
                SafeMsgBox(hWnd,
					"You must enter the password that was used to encrypt this data.",
					gsTitleCaption, MB_ICONINFORMATION);
                return TRUE;
            }
			else{
				if(IsDlgButtonChecked(hWnd, IDC_USEGENERATEDKEY) == BST_CHECKED)
				{
					PASSWORD Pwd;
					memset(&Pwd, 0, sizeof(Pwd));

					SetPassword(&Pwd, hPassword); //Get the password from the Textbox, or free RAM if empty.

					SetPassword(&gPassword, NULL, 0); //Free RAM.
					bool bResult = ReadHardKey(hWnd, sHardKeyFile, Pwd.sPassword, Pwd.iLength, &gPassword);
					SetPassword(&Pwd, NULL, 0); //Free RAM.
					if(!bResult)
					{
						return TRUE;
					}
				}
				else{
					SetPassword(&gPassword, hPassword); //Get the password from the Textbox, or free RAM if empty.
				}		
			}

            EndDialog(hWnd, 1);
            return TRUE;
        }

		if(wParam == IDC_USEGENERATEDKEY)
        {
			bool bEnable = true;

			if((bEnable = (IsDlgButtonChecked(hWnd, IDC_USEGENERATEDKEY) == BST_CHECKED)))
			{
				char *Filters = "HardKey-Files\0*" DEFAULTHARDKEYEXT "\0All-Files\0*.*\0\0";
				char sInitialDirectory[MAX_PATH];

				GetLastPath(sInitialDirectory, sizeof(sInitialDirectory));

				memset(sHardKeyFile, 0, sizeof(sHardKeyFile));

				if(OSFileDialog(hWnd, sHardKeyFile, sizeof(sHardKeyFile), DEFAULTHARDKEYEXT,
					"Select a previously generated HardKey file.",
					Filters, sInitialDirectory, OFD, OFN_EXPLORER + OFN_LONGNAMES + OFN_PATHMUSTEXIST))
				{
					SetLastPath(sHardKeyFile);
					/*
					if((dwKeySz = Get_FileSize(sHardKeyFile)) <= 0)
					{
						SafeMsgBox(hWnd, "Failed to open the key file that you selected.", gsTitleCaption, MB_ICONINFORMATION);
						CheckDlgButton(hWnd, IDC_USEGENERATEDKEY, BST_UNCHECKED);
					}
					*/
				}
				else{
					CheckDlgButton(hWnd, IDC_USEGENERATEDKEY, BST_UNCHECKED);
				}
			}

			return TRUE;
		}

		if(wParam == IDC_CANCEL || wParam == IDCANCEL)
        {
            EndDialog(hWnd, 0);
            DestroyWindow(hWnd);
            return TRUE;
        }

#ifndef _SELFEXTRACTOR_APP
        if(wParam == IDC_HELPBUTTON)
        {
			OpenHelpFile("Decrypting/WalkThrough.html");
            return TRUE;
        }
#endif		
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

