///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright © NetworkDLS 2010, All rights reserved
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _DIALOGS_TABLookAndFeel_CPP
#define _DIALOGS_TABLookAndFeel_CPP
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

#include "../Dialogs/WeakPwdNotificationDlg.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK TabLookAndFeelDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND hSortItem = NULL;
    static HWND hSortDirection = NULL;

    if(uMsg == WM_INITDIALOG)
    {
		hSortItem			= GetDlgItem(hWnd, IDC_SORTITEM);
        hSortDirection		= GetDlgItem(hWnd, IDC_SORTDIRECTION);
 
		SendMessage(hSortItem, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"Name");
		SendMessage(hSortItem, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"Type");
		SendMessage(hSortItem, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"Modified");
		SendMessage(hSortItem, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"Size");
		SendMessage(hSortItem, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"Ratio");
		SendMessage(hSortItem, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"Packed");
		SendMessage(hSortItem, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"CRC");
		SendMessage(hSortItem, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"Attributes");
		SendMessage(hSortItem, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"Path");
		SendMessage(hSortItem, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"Position");

		SendMessage(hSortDirection, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"Ascending");
        SendMessage(hSortDirection, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"Descending");

		SendMessage(hSortItem, (UINT)CB_SETCURSEL, (WPARAM)gdwDefaultSortColumn, (LPARAM)0);
		SendMessage(hSortDirection, (UINT)CB_SETCURSEL, (WPARAM)gdwDefaultSortDirection - 1, (LPARAM)0);

		if(gbKeepSelectionVisible)
		{
			CheckDlgButton(hWnd, IDC_KEEPSELVISIBLEWHILESORTING, BST_CHECKED);
		}
		else{
			CheckDlgButton(hWnd, IDC_KEEPSELVISIBLEWHILESORTING, BST_UNCHECKED);
		}

		if(gbShowGridIcons)
		{
			CheckDlgButton(hWnd, IDC_SHOWGRIDICONS, BST_CHECKED);
		}
		else{
			CheckDlgButton(hWnd, IDC_SHOWGRIDICONS, BST_UNCHECKED);
		}

		if(gbAlwaysRunInBackground)
		{
			CheckDlgButton(hWnd, IDC_ALWAYSINBACKGROUND, BST_CHECKED);
		}
		else{
			CheckDlgButton(hWnd, IDC_ALWAYSINBACKGROUND, BST_UNCHECKED);
		}

		if(gbShowGridLines)
		{
			CheckDlgButton(hWnd, IDC_ENABLEGRIDLINES, BST_CHECKED);
		}
		else{
			CheckDlgButton(hWnd, IDC_ENABLEGRIDLINES, BST_UNCHECKED);
		}

		if(gbEnableFullRowSelect)
		{
			CheckDlgButton(hWnd, IDC_ENABLEFULLROWSELECT, BST_CHECKED);
		}
		else{
			CheckDlgButton(hWnd, IDC_ENABLEFULLROWSELECT, BST_UNCHECKED);
		}

		if(gbEnableDragDrop)
		{
			CheckDlgButton(hWnd, IDC_ENABLEDRAGDROP, BST_CHECKED);
		}
		else{
			CheckDlgButton(hWnd, IDC_ENABLEDRAGDROP, BST_UNCHECKED);
		}

		if(gbWarnAboutNoEncryption)
		{
			CheckDlgButton(hWnd, IDC_WARNABOUTNOENCRYPTION, BST_CHECKED);
		}
		else{
			CheckDlgButton(hWnd, IDC_WARNABOUTNOENCRYPTION, BST_UNCHECKED);
		}

		return TRUE;
    }
    else if(uMsg == WM_COMMAND)
    {
		if(wParam == IDC_PASSWORDLEVEL)
        {
			DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_WEAKPWDNOTIFY), hWnd, WeakPwdNotificationDialog);
		}
        else if(wParam == IDC_HELPBUTTON)
        {
			OpenHelpFile("Configuration/Look & Feel.Html");
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
			char sSelection[255];

			if(!Get_Text(hSortItem, sSelection, sizeof(sSelection)))
			{
				return true;
			}
			gdwDefaultSortColumn = ColumnNameToPosition(sSelection);
			
			if(!Get_Text(hSortDirection, sSelection, sizeof(sSelection)))
			{
				return true;
			}
			if(_strcmpi(sSelection, "Ascending") == 0)
			{
				gdwDefaultSortDirection = SORT_DIR_ASCENDING;
			}
			else{
				gdwDefaultSortDirection = SORT_DIR_DESCENDING;
			}

			gbKeepSelectionVisible = (IsDlgButtonChecked(hWnd, IDC_KEEPSELVISIBLEWHILESORTING) > 0);
			gbShowGridIcons = (IsDlgButtonChecked(hWnd, IDC_SHOWGRIDICONS) > 0);
			gbShowGridLines = (IsDlgButtonChecked(hWnd, IDC_ENABLEGRIDLINES) > 0);
			gbEnableFullRowSelect = (IsDlgButtonChecked(hWnd, IDC_ENABLEFULLROWSELECT) > 0);
			gbEnableDragDrop = (IsDlgButtonChecked(hWnd, IDC_ENABLEDRAGDROP) > 0);
			gbAlwaysRunInBackground = (IsDlgButtonChecked(hWnd, IDC_ALWAYSINBACKGROUND) > 0);
			gbWarnAboutNoEncryption = (IsDlgButtonChecked(hWnd, IDC_WARNABOUTNOENCRYPTION) > 0);

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
