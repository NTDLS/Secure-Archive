#ifndef _LISTVIEW_CPP_
#define _LISTVIEW_CPP_
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <CommCtrl.H>
#include <ShlOBJ.H>
#include <Stdlib.H>
#include <Mmsystem.h>

#include "../Source/Package.H"
#include "../Source/Entry.H"
#include "../Source/ListView.H"

#include "../Dialogs/MainDlg.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 
using namespace NSWFL::String; 
using namespace NSWFL::File; 
using namespace NSWFL::Conversion; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

WNDPROC gpOldListProc;
HWND hMainDialog = NULL;
HWND ghFileList = NULL;
HMENU ghMenu = NULL;
HWND hHorizBar = NULL;
HWND hStatusBar = NULL;

HWND hTB_New[2];
HWND hTB_Open[2];
HWND hTB_AddFiles[2];
HWND hTB_Delete[2];
HWND hTB_Extract[2];
//HWND hTB_View[2];
HWND hTB_Info[2];
HWND hTB_Config[2];
HWND hTB_Find[2];
HWND hTB_Exit[2];

HIMAGELIST hImageList = NULL;
HIMAGELIST hImageListBig = NULL;

int giSortCol = -1;
int giSortDirection = -1;

const char *gsColumnNames[] = {
	"Name",
	"Type",
	"Modified",
	"Size",
	"Ratio",
	"Packed",
	"CRC",
	"Attributes",
	"Path",
	"Position",
	NULL
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int ColumnNameToPosition(const char *sColumnName)
{
	for(int iIndex = 0; gsColumnNames[iIndex]; iIndex++)
	{
		if(_strcmpi(sColumnName, gsColumnNames[iIndex]) == 0)
		{
			return iIndex;
		}
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UpdateButtons(void)
{
	//EnableButton(hTB_New, true);
	//EnableButton(hTB_Open, true);
	//EnableButton(hTB_AddFiles, true);
	EnableButton(hTB_Delete, gbArchiveOpen);
	EnableButton(hTB_Extract, gbArchiveOpen);
	////EnableButton(hTB_View, true);
	EnableButton(hTB_Info, gbArchiveOpen && (ListView_GetSelectedCount(ghFileList) == 1));
	//EnableButton(hTB_Config, true);
	EnableButton(hTB_Find, gbArchiveOpen);
	//EnableButton(hTB_Exit, true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void EnableButton(HWND *hButton, bool bEnable)
{
	if(bEnable)
	{
		if(!IsWindowVisible(hButton[0]))
		{
			ShowWindow(hButton[0], SW_SHOW);
			ShowWindow(hButton[1], SW_HIDE);
			RedrawWindow(hButton[0], NULL, NULL, RDW_INVALIDATE);
		}
	}
	else{
		if(!IsWindowVisible(hButton[1]))
		{
			ShowWindow(hButton[0], SW_HIDE);
			ShowWindow(hButton[1], SW_SHOW);
			RedrawWindow(hButton[1], NULL, NULL, RDW_INVALIDATE);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SetColumnWidth(int iPos, int iType, int iMax)
{
	LVCOLUMN vColumn;
	int iMin = 0;

	ListView_SetColumnWidth(ghFileList, iPos, LVSCW_AUTOSIZE_USEHEADER);
	memset(&vColumn, 0, sizeof(LVCOLUMN));
	vColumn.mask = LVCF_WIDTH;
	if(ListView_GetColumn(ghFileList, iPos, &vColumn))
	{
		iMin = vColumn.cx;
	}
	
	ListView_SetColumnWidth(ghFileList, iPos, iType);
	if(iMin > 0 || iMax > 0) //If iMax less than 0 means no Max size.
	{
		memset(&vColumn, 0, sizeof(LVCOLUMN));
		vColumn.mask = LVCF_WIDTH;
		if(ListView_GetColumn(ghFileList, iPos, &vColumn))
		{
			if(iMin > 0 && vColumn.cx < iMin)
			{
				ListView_SetColumnWidth(ghFileList, iPos, iMin);
			}

			if(iMax > 0 && vColumn.cx > iMax)
			{
				ListView_SetColumnWidth(ghFileList, iPos, iMax);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AutoSizeGridCols(void)
{
	SetColumnWidth(LIST_POS_NAME, LVSCW_AUTOSIZE, 250);
	SetColumnWidth(LIST_POS_TYPE, LVSCW_AUTOSIZE, 250);
	SetColumnWidth(LIST_POS_MODIFIED, LVSCW_AUTOSIZE, -1);
	SetColumnWidth(LIST_POS_SIZE, LVSCW_AUTOSIZE, -1);
	SetColumnWidth(LIST_POS_RATIO, LVSCW_AUTOSIZE, -1);
	SetColumnWidth(LIST_POS_PACKED, LVSCW_AUTOSIZE, -1);
	SetColumnWidth(LIST_POS_CRC, LVSCW_AUTOSIZE, -1);
	SetColumnWidth(LIST_POS_ATTRIBUTES, LVSCW_AUTOSIZE_USEHEADER, -1);
	SetColumnWidth(LIST_POS_PATH, LVSCW_AUTOSIZE, -1);
	ListView_SetColumnWidth(ghFileList, LIST_POS_POSITION, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AlignText(HWND hButColor, HWND hButGraye, HWND hButLabel, int *iInOutLeft)
{
	const int iIncrement = 64;
	int iLeft = *iInOutLeft;
	int iWidth = 50;
	int iHeight = 50;

	SetWindowPos(hButColor, NULL, iLeft, 6, iWidth, iHeight, SWP_FRAMECHANGED);
	SetWindowPos(hButGraye, NULL, iLeft, 6, iWidth, iHeight, SWP_FRAMECHANGED);
    SetWindowPos(hButLabel, NULL,
		iLeft - ((iIncrement - iWidth) / 2),
		57,
		iWidth + (iIncrement - iWidth),
		18,
		SWP_NOZORDER);

	*iInOutLeft = (iLeft + iIncrement);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HWND InitListView(HWND hDlg)
{
	HWND hList = GetDlgItem(hDlg, IDC_FILELIST);

	hTB_New[0]      = GetDlgItem(hDlg, IDI_TB_NEW);
	hTB_Open[0]     = GetDlgItem(hDlg, IDI_TB_OPEN);
	hTB_AddFiles[0] = GetDlgItem(hDlg, IDI_TB_ADDFILES);
	hTB_Delete[0]   = GetDlgItem(hDlg, IDI_TB_DELETE);
	hTB_Extract[0]  = GetDlgItem(hDlg, IDI_TB_EXTRACT);
	//hTB_View[0]	= GetDlgItem(hDlg, IDI_TB_VIEW);
	hTB_Info[0]     = GetDlgItem(hDlg, IDI_TB_INFO);
	hTB_Config[0]   = GetDlgItem(hDlg, IDI_TB_CONFIG);
	hTB_Find[0]     = GetDlgItem(hDlg, IDI_TB_FIND);
	hTB_Exit[0]     = GetDlgItem(hDlg, IDI_TB_EXIT);

	hTB_New[1]      = GetDlgItem(hDlg, IDI_TB_NEW_GS);
	hTB_Open[1]     = GetDlgItem(hDlg, IDI_TB_OPEN_GS);
	hTB_AddFiles[1] = GetDlgItem(hDlg, IDI_TB_ADDFILES_GS);
	hTB_Delete[1]   = GetDlgItem(hDlg, IDI_TB_DELETE_GS);
	hTB_Extract[1]  = GetDlgItem(hDlg, IDI_TB_EXTRACT_GS);
	//hTB_View[1]	= GetDlgItem(hDlg, IDI_TB_VIEW_GS);
	hTB_Info[1]     = GetDlgItem(hDlg, IDI_TB_INFO_GS);
	hTB_Config[1]   = GetDlgItem(hDlg, IDI_TB_CONFIG_GS);
	hTB_Find[1]     = GetDlgItem(hDlg, IDI_TB_FIND_GS);
	hTB_Exit[1]     = GetDlgItem(hDlg, IDI_TB_EXIT_GS);

	int iLeft = 16;

	AlignText(hTB_New[0],hTB_New[1], GetDlgItem(hDlg, IDI_TB_NEW_TXT), &iLeft);
	AlignText(hTB_Open[0], hTB_Open[1], GetDlgItem(hDlg, IDI_TB_OPEN_TXT), &iLeft);
	AlignText(hTB_AddFiles[0], hTB_AddFiles[1], GetDlgItem(hDlg, IDI_TB_ADDFILES_TXT), &iLeft);
	AlignText(hTB_Delete[0], hTB_Delete[1], GetDlgItem(hDlg, IDI_TB_DELETE_TXT), &iLeft);
	AlignText(hTB_Extract[0], hTB_Extract[1], GetDlgItem(hDlg, IDI_TB_EXTRACT_TXT), &iLeft);
	//AlignText(hTB_View[0], hTB_View[1], GetDlgItem(hDlg, IDI_TB_VIEW_TXT), &iLeft);
	AlignText(hTB_Info[0], hTB_Info[1], GetDlgItem(hDlg, IDI_TB_INFO_TXT), &iLeft);
	AlignText(hTB_Config[0], hTB_Config[1], GetDlgItem(hDlg, IDI_TB_CONFIG_TXT), &iLeft);
	AlignText(hTB_Find[0], hTB_Find[1], GetDlgItem(hDlg, IDI_TB_FIND_TXT), &iLeft);
	AlignText(hTB_Exit[0], hTB_Exit[1], GetDlgItem(hDlg, IDI_TB_EXIT_TXT), &iLeft);

	//---------------------------------------------------------------------------------

	DWORD dwFlags = LVS_EX_HEADERDRAGDROP;

	if(gbEnableFullRowSelect)
	{
		dwFlags |= LVS_EX_FULLROWSELECT;
	}

	if(gbShowGridLines)
	{
		dwFlags |= LVS_EX_GRIDLINES;
	}

    SendMessage(hList, (UINT)LVM_SETEXTENDEDLISTVIEWSTYLE, (WPARAM)0, (LPARAM)dwFlags);

    ListView_SetTextColor(hList, RGB(0,0,100));

    LV_COLUMN vColumn;

    memset(&vColumn, 0, sizeof(LV_COLUMN));

    vColumn.cx        = 200;
    vColumn.pszText   = "Name";
    vColumn.iSubItem  = LIST_POS_NAME;
    vColumn.mask      = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    vColumn.fmt       = LVCFMT_LEFT;
    ListView_InsertColumn(hList, LIST_POS_NAME, &vColumn);

    vColumn.cx        = 150;
    vColumn.pszText   = "Type";
    vColumn.iSubItem  = LIST_POS_TYPE;
    vColumn.mask      = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    vColumn.fmt       = LVCFMT_LEFT;
    ListView_InsertColumn(hList, LIST_POS_TYPE, &vColumn);

    vColumn.cx        = 125;
    vColumn.pszText   = "Modified";
    vColumn.iSubItem  = LIST_POS_MODIFIED;
    vColumn.mask      = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    vColumn.fmt       = LVCFMT_LEFT;
    ListView_InsertColumn(hList, LIST_POS_MODIFIED, &vColumn);

    vColumn.cx        = 80;
    vColumn.pszText   = "Size";
    vColumn.iSubItem  = LIST_POS_SIZE;
    vColumn.mask      = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    vColumn.fmt       = LVCFMT_RIGHT;
    ListView_InsertColumn(hList, LIST_POS_SIZE, &vColumn);

    vColumn.cx        = 65;
    vColumn.pszText   = "Ratio";
    vColumn.iSubItem  = LIST_POS_RATIO;
    vColumn.mask      = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    vColumn.fmt       = LVCFMT_LEFT;
    ListView_InsertColumn(hList, LIST_POS_RATIO, &vColumn);

    vColumn.cx        = 80;
    vColumn.pszText   = "Packed";
    vColumn.iSubItem  = LIST_POS_PACKED;
    vColumn.mask      = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    vColumn.fmt       = LVCFMT_RIGHT;
    ListView_InsertColumn(hList, LIST_POS_PACKED, &vColumn);

    vColumn.cx        = 80;
    vColumn.pszText   = "CRC";
    vColumn.iSubItem  = LIST_POS_CRC;
    vColumn.mask      = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    vColumn.fmt       = LVCFMT_LEFT;
    ListView_InsertColumn(hList, LIST_POS_CRC, &vColumn);

    vColumn.cx        = 75;
    vColumn.pszText   = "Attributes";
    vColumn.iSubItem  = LIST_POS_ATTRIBUTES;
    vColumn.mask      = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    vColumn.fmt       = LVCFMT_LEFT;
    ListView_InsertColumn(hList, LIST_POS_ATTRIBUTES, &vColumn);

    vColumn.cx        = 300;
    vColumn.pszText   = "Path";
    vColumn.iSubItem  = LIST_POS_PATH;
    vColumn.mask      = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    vColumn.fmt       = LVCFMT_LEFT;
    ListView_InsertColumn(hList, LIST_POS_PATH, &vColumn);

    vColumn.cx        = 0;
    vColumn.pszText   = "Position";
    vColumn.iSubItem  = LIST_POS_POSITION;
    vColumn.mask      = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    vColumn.fmt       = LVCFMT_LEFT;
    ListView_InsertColumn(hList, LIST_POS_POSITION, &vColumn);

	//---------------------------------------------------------------------------------

	if(gbShowGridIcons)
	{
		hImageList = ImageList_Create( GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_MASK|ILC_COLOR32, 1, 1);
		hImageListBig = ImageList_Create( GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), ILC_MASK|ILC_COLOR32, 1, 1);
	    ListView_SetImageList(hList, hImageList, LVSIL_SMALL);
	    ListView_SetImageList(hList, hImageListBig, LVSIL_NORMAL);
	}

	//---------------------------------------------------------------------------------

    gpOldListProc = (WNDPROC) SetWindowLongPtr(hList, GWLP_WNDPROC, (LRESULT) &NewListProc);

	return hList;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool Sink(HWND hWnd)
{
	DWORD dwStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

	if(!(dwStyle & WS_EX_CLIENTEDGE))
	{
		SetWindowLong(hWnd, GWL_EXSTYLE, dwStyle | WS_EX_CLIENTEDGE);
		SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}

	UpdateButtons();

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool Float(HWND hWnd)
{
	DWORD dwStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

	if((dwStyle & WS_EX_CLIENTEDGE))
	{
		SetWindowLong(hWnd, GWL_EXSTYLE, dwStyle & ~WS_EX_CLIENTEDGE);
		SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}

	UpdateButtons();

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline void OnMouseOver(HWND hColor, HWND hGrey)
{
	HWND hChild = GetWindowUnderPoint();
	if((hChild == hColor || hChild == hGrey) && (hMainDialog == GetForegroundWindow()))
	{
		Sink(hColor);
	}
	else{
		Float(hColor);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI MouseOverThread(LPVOID lpData)
{
	while(true)
	{
		if(hMainDialog == GetForegroundWindow())
		{
			OnMouseOver(hTB_New[0], hTB_New[1]);
			OnMouseOver(hTB_Open[0], hTB_Open[1]);
			OnMouseOver(hTB_AddFiles[0], hTB_AddFiles[1]);
			OnMouseOver(hTB_Delete[0], hTB_Delete[1]);
			OnMouseOver(hTB_Extract[0], hTB_Extract[1]);
			//OnMouseOver(hTB_View[0], hTB_View[1]);
			OnMouseOver(hTB_Info[0], hTB_Info[1]);
			OnMouseOver(hTB_Config[0], hTB_Config[1]);
			OnMouseOver(hTB_Find[0], hTB_Find[1]);
			OnMouseOver(hTB_Exit[0], hTB_Exit[1]);
		}
		Sleep(100);
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SelectedAllOfSelectedPartialPath(HWND hGrid, const char *sPathPart)
{
	int iItemPos = -1;
	int iArrayItems = 0;
	int iLength = 0;
	int iItem = 0;
	int iSelectedCount = ListView_GetItemCount(hGrid);

	char sPath[1024];

	iLength = (int)strlen(sPathPart);

	while( (iItemPos = ListView_GetNextItem(hGrid, iItemPos, LVNI_ALL)) != -1)
	{
		ListView_GetItemText(hGrid, iItemPos, LIST_POS_PATH, sPath, sizeof(sPath));
		if(_strnicmp(sPath, sPathPart, iLength) == 0)
		{
			ListView_SetItemState(hGrid, iItemPos, LVIS_SELECTED, LVIS_SELECTED);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SelectedAllOfSelectedPaths(HWND hGrid)
{
	char sPath[2048];
	int iItemPos = -1;
	int iArrayItems = 0;
	int iLength = 0;
	int iItem = 0;
	int iSelectedCount = ListView_GetSelectedCount(hGrid);

	if(iSelectedCount > 0)
	{
		char **sPaths = (char **) calloc(sizeof(char *), iSelectedCount);

		iItemPos = -1;
		while( (iItemPos = ListView_GetNextItem(hGrid, iItemPos, LVNI_SELECTED)) != -1)
		{
			ListView_GetItemText(hGrid, iItemPos, LIST_POS_PATH, sPath, sizeof(sPath));
			iLength = (int)strlen(sPath);
			if(iLength > 0 && _strcmpi(sPath, "N/A") != 0)
			{
				sPaths[iArrayItems] = (char *) calloc(sizeof(char), iLength + 1);
				strcpy_s(sPaths[iArrayItems], iLength + 1, sPath);
				iArrayItems++;
			}
		}

		iItemPos = -1;
		while( (iItemPos = ListView_GetNextItem(hGrid, iItemPos, LVNI_ALL)) != -1)
		{
			ListView_GetItemText(hGrid, iItemPos, LIST_POS_PATH, sPath, sizeof(sPath));

			iItem = 0;
			while(iItem < iArrayItems)
			{
				if(_strcmpi(sPaths[iItem], sPath) == 0)
				{
					ListView_SetItemState(hGrid, iItemPos, LVIS_SELECTED, LVIS_SELECTED);
					break;
				}
				iItem++;
			}

		}

		iItem = 0;
		while(iItem < iArrayItems)
		{
			free(sPaths[iItem]);
			sPaths[iItem] = NULL;
			iItem++;
		}

		free(sPaths);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CheckGridRows(HWND hGrid, bool bCheck)
{
	int iRow = 0;
	int iItems = ListView_GetItemCount(hGrid);

	while(iRow < iItems)
    {
		ListView_SetItemState(hGrid, iRow, LVIS_SELECTED, LVIS_SELECTED);
		iRow++;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK NewListProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = CallWindowProc(gpOldListProc, hWnd, uMsg, wParam, lParam);

	if(uMsg == WM_COMMAND)
	{
		if(wParam == PCE_SELECTALL)
        {
			CheckGridRows(ghFileList, true);

			RecalcSelectedFiles();
		}
		else if(LOWORD(wParam) == PCE_SELECTALLINPATHS)
        {
			SelectedAllOfSelectedPaths(ghFileList);

			RecalcSelectedFiles();
		}
		else if(LOWORD(wParam) == PCE_SELECTALLINPATH)
        {
			int iSelectedIndex = ListView_GetNextItem(ghFileList, -1, LVNI_SELECTED);
			int iLastPos = HIWORD(wParam);

			if(iSelectedIndex >= 0 && iLastPos >= 0)
			{
				char sSelectedPath[2048];
				ListView_GetItemText(ghFileList, iSelectedIndex, LIST_POS_PATH, sSelectedPath, sizeof(sSelectedPath));
				int iSelectedPathLen = (int)strlen(sSelectedPath);
				if(iSelectedPathLen > 0)
				{
					sSelectedPath[iLastPos] = '\0';
					SelectedAllOfSelectedPartialPath(ghFileList, sSelectedPath);
				}
			}

			RecalcSelectedFiles();
		}
		else if(wParam == PCE_DELETESELECTED)
        {
			if(ListView_GetSelectedCount(hWnd) > 0)
			{
				SendMessage(hMainDialog, WM_COMMAND, IDI_TB_DELETE, 0);
			}
		}
		else if(wParam == PCE_EXTRACTSELECTED)
        {
			if(ListView_GetSelectedCount(hWnd) > 0)
			{
				SendMessage(hMainDialog, WM_COMMAND, IDI_TB_EXTRACT, 0);
			}
		}
	}
	else if(uMsg == WM_RBUTTONDOWN)
    {
		UpdateButtons();

		if(ListView_GetItemCount(hWnd) > 0)
		{
			int iSelected = ListView_GetSelectedCount(hWnd);

			HMENU hParent = CreateMenu();
			HMENU hChild = CreateMenu();
			HMENU hSelect = CreateMenu();

			AppendMenu(hChild, MF_STRING, PCE_SELECTALL, "Select all");

			if(iSelected > 1)
			{
				AppendMenu(hChild, MF_STRING, PCE_SELECTALLINPATHS, "Select all files in these paths");
			}
			else{
				int iSelectedIndex = ListView_GetNextItem(ghFileList, -1, LVNI_SELECTED);

				char sSelectedPath[2048];
				ListView_GetItemText(ghFileList, iSelectedIndex, LIST_POS_PATH, sSelectedPath, sizeof(sSelectedPath));
				int iSelectedPathLen = (int)strlen(sSelectedPath);
				if(iSelectedPathLen > 0 && _strcmpi(sSelectedPath, "\\") != 0)
				{
					char sPathPart[2048];
					int iLastPos = 0;
					int iPathNumber = 0;
					int iPaths = 0;

					strcpy_s(sPathPart, sizeof(sPathPart), "");

					while((iLastPos = InStr("\\", sSelectedPath, iSelectedPathLen, iLastPos)) >= 0
						&& iPaths < 20 && strlen(sPathPart) < 200)
					{
						if(iLastPos > 0)
						{
							strncpy_s(sPathPart, sizeof(sPathPart), sSelectedPath, iLastPos);
							sPathPart[iLastPos] = '\0';

							AppendMenu(hSelect, MF_STRING, MAKELONG(PCE_SELECTALLINPATH, iLastPos), sPathPart);
							iPathNumber++;
						}
						iLastPos++;
					}

					InsertMenu(hChild, 1, MF_POPUP|MF_BYPOSITION, (UINT_PTR)hSelect, "Select all files in...");
				}
			}

			AppendMenu(hChild, MF_SEPARATOR, PCE_SEPERATOR, "");
			AppendMenu(hChild, MF_STRING, PCE_EXTRACTSELECTED, "Extract selected files ...");
			AppendMenu(hChild, MF_STRING, PCE_DELETESELECTED, "Delete selected files ...");

			InsertMenu(hParent, 0, MF_POPUP|MF_BYPOSITION, (UINT_PTR)hChild, "Secure Archive");

			POINT xy;
			GetCursorPos(&xy);
			TrackPopupMenuEx(hChild, TPM_LEFTALIGN|TPM_RIGHTBUTTON, (xy.x) + 1, xy.y, hWnd, NULL);
			DestroyMenu(hChild);
			DestroyMenu(hParent);

			RecalcSelectedFiles();
		}
    }
	else if(uMsg == WM_KEYUP || uMsg == WM_RBUTTONDOWN || uMsg == WM_LBUTTONDOWN)
	{
		if(wParam == PCE_KEY_DELETE)
		{
			SendMessage(hMainDialog, WM_COMMAND, IDI_TB_DELETE, 0);
		}
		else{
			RecalcSelectedFiles();
		}
		UpdateButtons();
    }

	//--------------------------------------------------------------------------

	return lResult;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RecalcSelectedFiles(void)
{
	HWND hParent = GetParent(ghFileList);
	if(!hParent || !IsWindow(hParent))
	{
		return;
	}

	int iIndex = -1;
	int iSelected = ListView_GetSelectedCount(ghFileList);
	char sSize[64];
	char Selected[64];
	char sText[255];
	__int64 i64Size = 0;
	char *sName = NULL;

	while((iIndex = ListView_GetNextItem(ghFileList, iIndex, LVNI_SELECTED)) != -1)
	{
		ListView_GetItemText(ghFileList, iIndex, LIST_POS_SIZE, sSize, sizeof(sSize));
		i64Size = i64Size + i64FormattedValue(sSize);
	}

	FileSizeFriendly(i64Size, sSize, sizeof(sSize));
	FormatInteger(Selected, sizeof(Selected), iSelected);
	if(iSelected == 1)
	{
		sName = "File";
	}
	else{
		sName = "Files";
	}
	sprintf_s(sText, sizeof(sText), "Selected: %s %s, %s", Selected, sName, sSize);

	SendMessage(hStatusBar, (UINT)SB_SETTEXT, (WPARAM)0, (LPARAM)sText);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int CALLBACK CompListEx(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3)
{
    char sBuf1[1024];
    char sBuf2[1024];

    SORTDATA *lpsd;

    lpsd = (SORTDATA *)lParam3;

    ListView_GetItemText(lpsd->hwndList, lParam1, (int)lpsd->isortSubItem, sBuf1, sizeof(sBuf1));
    ListView_GetItemText(lpsd->hwndList, lParam2, (int)lpsd->isortSubItem, sBuf2, sizeof(sBuf2));

    if(lpsd->iColType == COL_STRING)
    {
        short iVal = _strcmpi(sBuf1, sBuf2);
		if(iVal == 0)
		{
			return 0;
		}
		else{
			if(lpsd->iUPDOWN == SORT_DIR_DESCENDING)
			{
				return iVal * -1;
			}
			else{
				return iVal;
			}
		}
    }
    else if(lpsd->iColType == COL_INTEGER)
    {
        __int64 i64Val1 = i64FormattedValue(sBuf1);
        __int64 i64Val2 = i64FormattedValue(sBuf2);

        if(i64Val1 > i64Val2)
		{
			if(lpsd->iUPDOWN == SORT_DIR_DESCENDING)
			{
				return -1;
			}
			else{
				return 1;
			}
		}
        else if(i64Val1 < i64Val2)
		{
			if(lpsd->iUPDOWN == SORT_DIR_DESCENDING)
			{
				return 1;
			}
			else{
				return -1;
			}
		}
		else{
			return 0;
		}
    }
    else if(lpsd->iColType == COL_DOUBLE)
    {
        double Val1 = fFormattedValue(sBuf1);
        double Val2 = fFormattedValue(sBuf2);

        if(Val1 > Val2)
		{
			if(lpsd->iUPDOWN == SORT_DIR_DESCENDING)
			{
				return -1;
			}
			else{
				return 1;
			}
		}
        else if(Val1 < Val2)
		{
			if(lpsd->iUPDOWN == SORT_DIR_DESCENDING)
			{
				return 1;
			}
			else{
				return -1;
			}
		}
		else{
			return 0;
		}
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int CALLBACK CompList(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3)
{
    SORTDATA *lpsd;

    lpsd = (SORTDATA *)lParam3;

    if(lParam1 > lParam2)
	{
		if(lpsd->iUPDOWN == SORT_DIR_DESCENDING)
		{
			return -1;
		}
		else{
			return 1;
		}
	}
    else if(lParam1 < lParam2)
	{
		if(lpsd->iUPDOWN == SORT_DIR_DESCENDING)
		{
			return 1;
		}
		else{
			return -1;
		}
	}
	else{
		return 0;
	}

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SortFileList(HWND hList, const char *sColumnName, int iSortDirection)
{
	int iSortColumn = ColumnNameToPosition(sColumnName);
	if(iSortColumn >= 0)
	{
		return SortFileList(hList, iSortColumn, iSortDirection);
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SortFileList(HWND hList, int iSortColumn, int iSortDirection)
{
	if(iSortColumn >= 0)
	{
		SORTDATA SortData;
		memset(&SortData, 0, sizeof(SortData));

		SortData.hwndList = hList;
		SortData.isortSubItem = iSortColumn;
		SortData.iUPDOWN = iSortDirection;

		if(SortData.isortSubItem == LIST_POS_NAME){
			SortData.iColType = COL_STRING;
		}
		else if(SortData.isortSubItem == LIST_POS_TYPE){
			SortData.iColType = COL_STRING;
		}
		else if(SortData.isortSubItem == LIST_POS_MODIFIED){
			SortData.iColType = COL_DATE;
		}
		else if(SortData.isortSubItem == LIST_POS_SIZE){
			SortData.iColType = COL_INTEGER;
		}
		else if(SortData.isortSubItem == LIST_POS_RATIO){
			SortData.iColType = COL_DOUBLE;
		}
		else if(SortData.isortSubItem == LIST_POS_PACKED){
			SortData.iColType = COL_INTEGER;
		}
		else if(SortData.isortSubItem == LIST_POS_CRC){
			SortData.iColType = COL_STRING;
		}
		else if(SortData.isortSubItem == LIST_POS_ATTRIBUTES){
			SortData.iColType = COL_STRING;
		}
		else if(SortData.isortSubItem == LIST_POS_PATH){
			SortData.iColType = COL_STRING;
		}
		else if(SortData.isortSubItem == LIST_POS_POSITION){
			SortData.iColType = COL_INTEGER;
		}
		else{
			SortData.iColType = 0;
		}

		if(SortData.iColType == COL_DATE)
		{
			ListView_SortItems(hList, CompList, (LPARAM)&SortData);
		}
		else if(SortData.iColType != 0)
		{
			ListView_SortItemsEx(hList, CompListEx, (LPARAM)&SortData);
		}

		if(gbKeepSelectionVisible)
		{
			int SelectedIndex = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
			if(SelectedIndex >= 0)
			{
				ListView_EnsureVisible(hList, SelectedIndex, FALSE);
			}
		}

		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
