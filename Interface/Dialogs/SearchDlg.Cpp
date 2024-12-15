#ifndef _SearchDlg_CPP
#define _SearchDlg_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <CommCtrl.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "../Source/Package.H"
#include "../Source/Entry.H"
#include "../Source/Routines.H"
#include "../Source/ListView.H"

#include "../Dialogs/MainDlg.H"
#include "../Dialogs/SearchDlg.H"
#include "../Dialogs/NetLogoDlg.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 
using namespace NSWFL::String; 
using namespace NSWFL::Conversion; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------(Variable Declarations)
HWND hSearchDialog = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int IsMatch(const char *sSearchFor, int sSearchForSz, const char *sInBuf, const int iBufSz)
{
	int iLookingLoop  = 0; 
	int iStringStart  = -1;

	if(sSearchForSz > iBufSz)
	{
		return -2;
	}

	for(int iControlLoop = 0; iControlLoop <= (iBufSz-sSearchForSz); iControlLoop++)
	{
		if(sInBuf[iControlLoop] == sSearchFor[iLookingLoop] || sSearchFor[iLookingLoop] == '?')
		{
			while(iLookingLoop <= sSearchForSz) 
			{
				if(sSearchFor[iLookingLoop] == sInBuf[iLookingLoop + iControlLoop]
					|| sSearchFor[iLookingLoop] == '?')
				{
					iLookingLoop++;
				}
				else iLookingLoop = sSearchForSz + 1; 

				if(iLookingLoop == sSearchForSz)
				{
					iStringStart = iControlLoop; 
					iControlLoop = (iBufSz - sSearchForSz) + 1;
				}
			}
		}
		iLookingLoop = 0;
	}

	return iStringStart; 
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

char sLookForName[1024];
char sLookForType[1024];
char sLookForPath[1024];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK SearchDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //--------------------------------------------------------------------------

	static HWND hName = NULL;
	static HWND hType = NULL;
	static HWND hPath = NULL;
	static HWND hFilesFound = NULL;

	static bool bFirstTime = true;
    static DWORD dwBackgroundColor = 0;
    static DWORD dwForegroundColor = 0;

    //--------------------------------------------------------------------------

    if(uMsg == WM_INITDIALOG) // Received an Initialize Dialog Message
    {
        hSearchDialog = hWnd;
 
        HMENU hSystemMenu = GetSystemMenu(hWnd, FALSE);
        AppendMenu(hSystemMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hSystemMenu, MF_STRING, WM_ABOUT_DIALOG, "About");

        SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Find in Archive");
        SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN)) );

        dwBackgroundColor = GetSysColor(COLOR_3DFACE);

		hName = GetDlgItem(hWnd, IDC_NAME);
		hType = GetDlgItem(hWnd, IDC_TYPE);
		hPath = GetDlgItem(hWnd, IDC_PATH);
		hFilesFound = GetDlgItem(hWnd, IDC_FILESFOUND);

		SendMessage(hName, EM_LIMITTEXT, sizeof(sLookForName), 0);
		SendMessage(hType, EM_LIMITTEXT, sizeof(sLookForType), 0);
		SendMessage(hPath, EM_LIMITTEXT, sizeof(sLookForPath), 0);

		if(bFirstTime)
		{
			strcpy_s(sLookForName, sizeof(sLookForName), "");
			strcpy_s(sLookForType, sizeof(sLookForType), "");
			strcpy_s(sLookForPath, sizeof(sLookForPath), "");
			bFirstTime = false;
		}

		Set_Text(hFilesFound, "");
		Set_Text(hName, sLookForName);
		Set_Text(hType, sLookForType);
		Set_Text(hPath, sLookForPath);

        CenterOverOwner(hWnd);

		ShowWindowInTaskbar(hWnd, true);

		return TRUE; // Return TRUE to set the keyboard focus, Otherwise return FALSE
    }

    //--------------------------------------------------------------------------

    if(uMsg == WM_COMMAND)
    {
		if(wParam == IDC_FIND || wParam == IDC_FINDNEXT || wParam == IDC_SELECTALLMATCHES || wParam == IDOK)
        {
			Set_Text(hFilesFound, "");

			bool bFindNext = (wParam == IDC_FINDNEXT);
			bool bSelectAll = (wParam == IDC_SELECTALLMATCHES);

			char sLookInName[2048];
			char sLookInType[2048];
			char sLookInPath[2048];
			char sFound[2048 + 512];

			int iLookForName = Get_Text(hName, sLookForName, sizeof(sLookForName));
			int iLookForType = Get_Text(hType, sLookForType, sizeof(sLookForType));
			int iLookForPath = Get_Text(hPath, sLookForPath, sizeof(sLookForPath));
			int iLookInName = 0;
			int iLookInType = 0;
			int iLookInPath = 0;

			int iFilesFound = 0;
			int iReqMatches = 0;
			int iFirstFound = -1;
			int iPreviouslySelectedItem = -1;
			int iItemPos = -1;
			int iMatches = 0;

			if(iLookForName && iLookForName < sizeof(sLookForName))
			{
				LCase(sLookForName, iLookForName);
				iReqMatches++;
			}

			if(iLookForType && iLookForType < sizeof(sLookForType))
			{
				LCase(sLookForType, iLookForType);
				iReqMatches++;
			}

			if(iLookForPath && iLookForPath < sizeof(sLookForPath))
			{
				LCase(sLookForPath, iLookForPath);
				iReqMatches++;
			}

			if( (iPreviouslySelectedItem = ListView_GetNextItem(ghFileList, -1, LVNI_SELECTED)) != -1)
			{
				ListView_SetItemState(ghFileList, iItemPos, 0, LVIS_SELECTED); //Deselect Item.
			}

			if(bFindNext)
			{
				iItemPos = iPreviouslySelectedItem;
			}
			else{
				iItemPos = -1;
			}

			while( (iItemPos = ListView_GetNextItem(ghFileList, iItemPos, LVNI_ALL)) != -1)
			{
				iMatches = 0;

				if(iLookForName > 0 && iLookForName < sizeof(sLookForName))
				{
					ListView_GetItemText(ghFileList, iItemPos, LIST_POS_NAME, sLookInName, sizeof(sLookInName));
					if((iLookInName = (int)strlen(sLookInName)) > 0)
					{
						LCase(sLookInName, iLookInName);
						if(IsMatch(sLookForName, iLookForName, sLookInName, iLookInName) >= 0)
						{
							iMatches++;
						}
					}
				}

				if(iLookForType > 0 && iLookForType < sizeof(sLookForType))
				{
					ListView_GetItemText(ghFileList, iItemPos, LIST_POS_TYPE, sLookInType, sizeof(sLookInType));

					if((iLookInType = (int)strlen(sLookInType)) > 0)
					{
						LCase(sLookInType, iLookInType);
						if(IsMatch(sLookForType, iLookForType, sLookInType, iLookInType) >= 0)
						{
							iMatches++;
						}
					}
				}

				if(iLookForPath > 0 && iLookForPath < sizeof(sLookForPath))
				{
					ListView_GetItemText(ghFileList, iItemPos, LIST_POS_PATH, sLookInPath, sizeof(sLookInPath));

					if((iLookInPath = (int)strlen(sLookInPath)) > 0)
					{
						LCase(sLookInPath, iLookInPath);
						if(IsMatch(sLookForPath, iLookForPath, sLookInPath, iLookInPath) >= 0)
						{
							iMatches++;
						}
					}
				}

				if(iMatches >= iReqMatches)
				{
					iFilesFound++;

					ListView_SetItemState(ghFileList, iItemPos, LVIS_SELECTED, LVIS_SELECTED); //Select Item.

					if(iFirstFound < 0)
					{
						iFirstFound = iItemPos;
					}

					if(!bSelectAll)
					{
						break;
					}
				}
				else{
					ListView_SetItemState(ghFileList, iItemPos, 0, LVIS_SELECTED); //Deselect Item.
				}
			}

			if(iFilesFound == 0)
			{
				dwForegroundColor = RGB(255, 0, 0);

				Set_Text(hFilesFound, "No matching files were found.");				
				SafeMsgBox(hWnd, "No matching files were found.", gsTitleCaption, MB_ICONINFORMATION);

				if(iPreviouslySelectedItem >= 0)
				{
					ListView_SetItemState(ghFileList, iPreviouslySelectedItem, LVIS_SELECTED, LVIS_SELECTED);
					ListView_EnsureVisible(ghFileList, iPreviouslySelectedItem, FALSE);
				}
			}
			else{
				dwForegroundColor = RGB(0, 0, 255);

				if(bSelectAll)
				{
					char sFiles[64];
					FormatInteger(sFiles, sizeof(sFiles), iFilesFound);

					sprintf_s(sFound, sizeof(sFound), "Found and Selected %s Files.", sFiles);
					Set_Text(hFilesFound, sFound);
				}
				else{
					ListView_GetItemText(ghFileList, iItemPos, LIST_POS_NAME, sLookInName, sizeof(sLookInName));

					if(bFindNext)
					{
						sprintf_s(sFound, sizeof(sFound), "Found next occurrence: %s", sLookInName);
					}
					else{
						sprintf_s(sFound, sizeof(sFound), "Found first occurrence: %s", sLookInName);
					}
					Set_Text(hFilesFound, sFound);
				}

				ListView_SetItemState(ghFileList, iFirstFound, LVIS_SELECTED, LVIS_SELECTED);
				ListView_EnsureVisible(ghFileList, iFirstFound, FALSE);
			}

			RecalcSelectedFiles();
		}

        if(wParam == IDC_CLOSE || wParam == IDCANCEL)
        {
			Get_Text(hName, sLookForName, sizeof(sLookForName));
			Get_Text(hType, sLookForType, sizeof(sLookForType));
			Get_Text(hPath, sLookForPath, sizeof(sLookForPath));

			hSearchDialog = NULL;
			EndDialog(hWnd, 0);
			DestroyWindow(hWnd);
            return TRUE;
        }

		return FALSE;
    }

    //--------------------------------------------------------------------------

    if(uMsg == WM_CTLCOLORSTATIC)
	{
		if((HANDLE)lParam == hFilesFound)
		{
			return Set_Color(dwForegroundColor, dwBackgroundColor, wParam);
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
		Get_Text(hName, sLookForName, sizeof(sLookForName));
		Get_Text(hType, sLookForType, sizeof(sLookForType));
		Get_Text(hPath, sLookForPath, sizeof(sLookForPath));

		hSearchDialog = NULL;
        EndDialog(hWnd, 0);
        DestroyWindow(hWnd);
        return TRUE;
    }

    //--------------------------------------------------------------------------

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

