///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright © NetworkDLS 2010, All rights reserved
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOGS_TabAdvanced_CPP
#define _DIALOGS_TabAdvanced_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <PrSht.H>
#include <Stdio.H>
#include <CommCtrl.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "../Source/Entry.H"
#include "../Source/Routines.H"
#include "../Source/ListView.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 
using namespace NSWFL::File; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK TabAdvancedDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND hTempFileLocation = NULL;
    static HWND hSecurePasses = NULL;

    if(uMsg == WM_INITDIALOG)
    {
        hTempFileLocation	= GetDlgItem(hWnd, IDC_TEMPFILELOCATION);
        hSecurePasses		= GetDlgItem(hWnd, IDC_SECUREPASSES);
 
		Set_LongFormatted(hSecurePasses, gdwSecureDeletePasses);
		SendMessage(GetDlgItem(hWnd, IDC_SECUREPASSES_SPIN),(UINT)UDM_SETBUDDY, (WPARAM)hSecurePasses, (LPARAM)NULL);
		SendMessage(GetDlgItem(hWnd, IDC_SECUREPASSES_SPIN),(UINT)UDM_SETRANGE, (WPARAM)NULL,(LPARAM)MAKELONG(100, 0));

		if(strlen(gsTempFilePath) == 0)
		{
			GetTempPath(sizeof(gsTempFilePath), gsTempFilePath);
		}
		Set_Text(hTempFileLocation, gsTempFilePath);

		if(gbUsePreAllocation)
		{
			CheckDlgButton(hWnd, IDC_USEPREALLOCATION, BST_CHECKED);
		}
		else{
			CheckDlgButton(hWnd, IDC_USEPREALLOCATION, BST_UNCHECKED);
		}

		if(gbDragDropCopyInsteadOfMove)
		{
			CheckDlgButton(hWnd, IDC_DRAGDROPCOPYINSTEADOFMOVE, BST_CHECKED);
		}
		else{
			CheckDlgButton(hWnd, IDC_DRAGDROPCOPYINSTEADOFMOVE, BST_UNCHECKED);
		}

		return TRUE;
    }
	else if(uMsg == WM_COMMAND)
    {
		if(wParam == IDC_TEMPFILELOCATION_CMD)
		{
			char sTempFolder[MAX_PATH];
			memset(sTempFolder, 0, sizeof(sTempFolder));

			if(BrowseDirectory(hWnd,
				"Select the folder that you would like Secure Archive to use for temporary storage.",
				BIF_RETURNONLYFSDIRS|BIF_USENEWUI, sTempFolder, sizeof(sTempFolder)))
			{
				Set_Text(hTempFileLocation, sTempFolder);
			}
			return TRUE;
		}
        else if(wParam == IDC_HELPBUTTON)
        {
			OpenHelpFile("Configuration/Advanced.Html");
            return TRUE;
        }
	}
    else if(uMsg == WM_NOTIFY)
    {
        LPNMHDR pNMH = (LPNMHDR)lParam;
        if(pNMH->code == PSN_KILLACTIVE)
        {
            return FALSE;
        }
        if(pNMH->code == PSN_APPLY) // Ok
        {
			if(Get_Text(hTempFileLocation, gsTempFilePath, sizeof(gsTempFilePath)) >= sizeof(gsTempFilePath))
			{
                SafeMsgBox(hWnd,
					"You must enter a valid temporary path between 1 and 255 characters.\r\n"
					"You can also leave this box blank to have Secure Archive use the windows default.",
					gsTitleCaption, MB_ICONASTERISK);
                SetWindowLongPtr(hWnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE); // Prevent window from closing
                return TRUE;
			}
			else{
				if(strlen(gsTempFilePath) == 0)
				{
					GetTempPath(sizeof(gsTempFilePath), gsTempFilePath);
				}
				else if(!IsDirectory(gsTempFilePath))
				{
					SafeMsgBox(hWnd,
						"The temporary path you entered either does not exist or you do not sufficient  permission.\r\n"
						"You can leave this box blank to have Secure Archive use the windows default.",
						gsTitleCaption, MB_ICONASTERISK);
					SetWindowLongPtr(hWnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE); // Prevent window from closing
					return TRUE;
				}
			}

			gbDragDropCopyInsteadOfMove = (IsDlgButtonChecked(hWnd, IDC_DRAGDROPCOPYINSTEADOFMOVE) > 0);
			gbUsePreAllocation = (IsDlgButtonChecked(hWnd, IDC_USEPREALLOCATION) > 0);

			gdwSecureDeletePasses = Get_Long(hSecurePasses);
			if(gdwSecureDeletePasses < 0 || gdwSecureDeletePasses > 100)
			{
				gdwSecureDeletePasses = 1;
			}

			return TRUE;
		}
        if(pNMH->code == PSN_RESET) // Cancel
        {
            return FALSE;
        }
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
