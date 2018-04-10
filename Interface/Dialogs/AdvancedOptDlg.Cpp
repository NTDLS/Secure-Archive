#ifndef _AdvancedOpt_CPP
#define _AdvancedOpt_CPP
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK AdvancedOptDlg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//--------------------------------------------------------------------------

    if(uMsg == WM_INITDIALOG) // Received A Initilize Dialog Message
    {
        HMENU hSystemMenu = GetSystemMenu(hWnd, FALSE);
        AppendMenu(hSystemMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hSystemMenu, MF_STRING, WM_ABOUT_DIALOG, "About");

        SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Advanced Options");
        SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN)) );

        if(gdwCurrentJobType == JOB_TYPE_PACKING_FILE)
		{
			CheckDlgButton(hWnd, IDC_CHKSAVEPATHS, BST_UNCHECKED);
			EnableWindow(GetDlgItem(hWnd, IDC_CHKSAVEPATHS), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_CHKPATHATTRIBUTES), FALSE);
		}
		else{
			CheckDlgButton(hWnd, IDC_CHKSAVEPATHS, (giPkgAttribs &ATTRIB_SAVEPATHS));

			if(!IsDlgButtonChecked(hWnd, IDC_CHKSAVEPATHS))
			{
				EnableWindow(GetDlgItem(hWnd, IDC_CHKPATHATTRIBUTES), FALSE);
			}
		}

		CheckDlgButton(hWnd, IDC_CHKTIMESTAMPS, (giPkgAttribs &ATTRIB_DATETIME));
		CheckDlgButton(hWnd, IDC_CHKATTRIBUTES, (giPkgAttribs &ATTRIB_ATTRIBUTES));
		CheckDlgButton(hWnd, IDC_CHKCRCS, (giPkgAttribs &ATTRIB_CRCCHECK));
		CheckDlgButton(hWnd, IDC_CHKINCLUDEHIDDEN, (giPkgAttribs &ATTRIB_INCLUDEHIDDEN));
		CheckDlgButton(hWnd, IDC_CHKDELETESOURCE, (giPkgOptions &OPTION_DELETESOURCE));

        CenterOverOwner(hWnd);

        ShowWindowInTaskbar(hWnd, false);

        return TRUE; // Return TRUE to set the keyboard focus, Otherwise return FALSE
    }

    //--------------------------------------------------------------------------

    if(uMsg == WM_COMMAND)
    {
		if(wParam == IDC_CHKSAVEPATHS)
		{
			UINT uChecked = IsDlgButtonChecked(hWnd, IDC_CHKSAVEPATHS);
			EnableWindow(GetDlgItem(hWnd, IDC_CHKPATHATTRIBUTES), uChecked);
			CheckDlgButton(hWnd, IDC_CHKPATHATTRIBUTES, uChecked);
		}
		//--------------------------------------------------------------------------
        else if(wParam == IDC_OK || wParam == IDOK)
        {
			if(IsDlgButtonChecked(hWnd, IDC_CHKSAVEPATHS))
			{
				giPkgAttribs |= ATTRIB_SAVEPATHS;
			}
			else{
				giPkgAttribs &= ~ATTRIB_SAVEPATHS;
			}

			if(IsDlgButtonChecked(hWnd, IDC_CHKTIMESTAMPS))
			{
				giPkgAttribs |= ATTRIB_DATETIME;
			}
			else{
				giPkgAttribs &= ~ATTRIB_DATETIME;
			}

			if(IsDlgButtonChecked(hWnd, IDC_CHKATTRIBUTES))
			{
				giPkgAttribs |= ATTRIB_ATTRIBUTES;
			}
			else{
				giPkgAttribs &= ~ATTRIB_ATTRIBUTES;
			}

			if(IsDlgButtonChecked(hWnd, IDC_CHKCRCS))
			{
				giPkgAttribs |= ATTRIB_CRCCHECK;
			}
			else{
				giPkgAttribs &= ~ATTRIB_CRCCHECK;
			}

			if(IsDlgButtonChecked(hWnd, IDC_CHKINCLUDEHIDDEN))
			{
				giPkgAttribs |= ATTRIB_INCLUDEHIDDEN;
			}
			else{
				giPkgAttribs &= ~ATTRIB_INCLUDEHIDDEN;
			}

			if(IsDlgButtonChecked(hWnd, IDC_CHKDELETESOURCE))
			{
				giPkgOptions |= OPTION_DELETESOURCE;
			}
			else{
				giPkgOptions &= ~OPTION_DELETESOURCE;
			}

			if(IsDlgButtonChecked(hWnd, IDC_SAVEASDEFAULT))
			{
				gbDfltChkTimeStamps = (giPkgAttribs &ATTRIB_DATETIME) > 0;
				gbDfltChkAttributes = (giPkgAttribs &ATTRIB_ATTRIBUTES) > 0;
				gbDfltChkCRCs = (giPkgAttribs &ATTRIB_CRCCHECK) > 0;
				gbIncludeHiddenFiles = (giPkgAttribs &ATTRIB_INCLUDEHIDDEN) > 0;
				gbDeleteSourceFiles = (giPkgOptions &OPTION_DELETESOURCE) > 0;

				if(gdwCurrentJobType != JOB_TYPE_PACKING_FILE)
				{
					gbDfltChkSavePaths = (giPkgAttribs &ATTRIB_SAVEPATHS) > 0;
				}
			}

			EndDialog(hWnd, 1);
            return TRUE;
        }
        else if(wParam == IDC_CANCEL || wParam == IDCANCEL)
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
