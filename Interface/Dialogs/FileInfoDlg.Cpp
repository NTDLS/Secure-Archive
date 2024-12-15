#ifndef _FileInfoDlg_CPP
#define _FileInfoDlg_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "../Source/Package.H"
#include "../Source/Entry.H"
#include "../Source/Routines.H"
#include "../Source/ListView.H"

#include "../Dialogs/MainDlg.H"
#include "../Dialogs/FileInfoDlg.H"
#include "../Dialogs/NetLogoDlg.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 
using namespace NSWFL::File; 
using namespace NSWFL::String; 
using namespace NSWFL::Conversion; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------(Variable Declarations)
HWND hFileInfoDialog = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK FileInfoDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //--------------------------------------------------------------------------

	static HWND hName = NULL;
	static HWND hType = NULL;
	static HWND hPath = NULL;
	static HWND hFSize = NULL;
	static HWND hPSize = NULL;
	static HWND hRatio = NULL;
	static HWND hLastMod = NULL;
	static HWND hCRC32 = NULL;
	static HWND hAttrib = NULL;
	static HWND hInfoIcon = NULL;
	static HICON hLarge = NULL;

    //--------------------------------------------------------------------------

    if(uMsg == WM_INITDIALOG) // Received an Initialize Dialog Message
    {
        hFileInfoDialog = hWnd;

        HMENU hSystemMenu = GetSystemMenu(hWnd, FALSE);
        AppendMenu(hSystemMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hSystemMenu, MF_STRING, WM_ABOUT_DIALOG, "About");

        SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"File Information");
        SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN)) );

		hName		= GetDlgItem(hWnd, IDC_NAME);
		hType		= GetDlgItem(hWnd, IDC_TYPE);
		hPath		= GetDlgItem(hWnd, IDC_PATH);
		hFSize		= GetDlgItem(hWnd, IDC_FILESIZE);
		hPSize		= GetDlgItem(hWnd, IDC_PACKSIZE);
		hRatio		= GetDlgItem(hWnd, IDC_RATIO);
		hLastMod	= GetDlgItem(hWnd, IDC_LASTMOD);
		hCRC32		= GetDlgItem(hWnd, IDC_CRC32);
		hAttrib		= GetDlgItem(hWnd, IDC_ATTRIB);
		hInfoIcon	= GetDlgItem(hWnd, IDC_INFOICON);

		SendMessage(hName, EM_SETREADONLY, 1, 0);
		SendMessage(hType, EM_SETREADONLY, 1, 0);
		SendMessage(hPath, EM_SETREADONLY, 1, 0);
		SendMessage(hFSize, EM_SETREADONLY, 1, 0);
		SendMessage(hPSize, EM_SETREADONLY, 1, 0);
		SendMessage(hRatio, EM_SETREADONLY, 1, 0);
		SendMessage(hLastMod, EM_SETREADONLY, 1, 0);
		SendMessage(hCRC32, EM_SETREADONLY, 1, 0);
		SendMessage(hAttrib, EM_SETREADONLY, 1, 0);

		char sBuf[2048];
		char sSize[64];

		int iItem = ListView_GetNextItem(ghFileList, -1, LVNI_SELECTED);	

		if(iItem >= 0)
		{
			LV_ITEM lvItem;
			memset(&lvItem, 0, sizeof(lvItem));

			lvItem.iItem = iItem;
			lvItem.iSubItem = 0;
			lvItem.mask = LVIF_IMAGE;

			ListView_GetItem(ghFileList, &lvItem);

			hLarge = ImageList_ExtractIcon(ghAppInstance, hImageListBig, lvItem.iImage);
			if(hLarge)
			{
				SendMessage(hInfoIcon,(UINT)STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)(HICON)hLarge);
			}

			ListView_GetItemText(ghFileList, iItem, LIST_POS_NAME, sBuf, sizeof(sBuf));
			Set_Text(hName, sBuf);

			ListView_GetItemText(ghFileList, iItem, LIST_POS_TYPE, sBuf, sizeof(sBuf));
			Set_Text(hType, sBuf);

			ListView_GetItemText(ghFileList, iItem, LIST_POS_MODIFIED, sBuf, sizeof(sBuf));
			Set_Text(hLastMod, sBuf);

			ListView_GetItemText(ghFileList, iItem, LIST_POS_SIZE, sBuf, sizeof(sBuf));
			FileSizeFriendly(i64FormattedValue(sBuf), sSize, sizeof(sSize));
			strcat_s(sBuf, sizeof(sBuf), " (");
			strcat_s(sBuf, sizeof(sBuf), sSize);
			strcat_s(sBuf, sizeof(sBuf), ")");
			Set_Text(hFSize, sBuf);

			ListView_GetItemText(ghFileList, iItem, LIST_POS_RATIO, sBuf, sizeof(sBuf));
			Set_Text(hRatio, sBuf);

			ListView_GetItemText(ghFileList, iItem, LIST_POS_PACKED, sBuf, sizeof(sBuf));
			FileSizeFriendly(i64FormattedValue(sBuf), sSize, sizeof(sSize));
			strcat_s(sBuf, sizeof(sBuf), " (");
			strcat_s(sBuf, sizeof(sBuf), sSize);
			strcat_s(sBuf, sizeof(sBuf), ")");
			Set_Text(hPSize, sBuf);

			ListView_GetItemText(ghFileList, iItem, LIST_POS_CRC, sBuf, sizeof(sBuf));
			Set_Text(hCRC32, sBuf);

			ListView_GetItemText(ghFileList, iItem, LIST_POS_ATTRIBUTES, sBuf, sizeof(sBuf));
			Set_Text(hAttrib, sBuf);

			ListView_GetItemText(ghFileList, iItem, LIST_POS_PATH, sBuf, sizeof(sBuf));
			Set_Text(hPath, sBuf);
		}

        CenterOverOwner(hWnd);

		ShowWindowInTaskbar(hWnd, false);

		return TRUE; // Return TRUE to set the keyboard focus, Otherwise return FALSE
    }

    //--------------------------------------------------------------------------

    if(uMsg == WM_COMMAND)
    {
        if(wParam == IDC_CMDOK || wParam == IDOK || wParam == IDCANCEL)
        {
			if(hLarge)
			{
			    DestroyIcon(hLarge);
				DeleteObject(hLarge);
			}

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
		if(hLarge)
		{
		    DestroyIcon(hLarge);
			DeleteObject(hLarge);
		}

        EndDialog(hWnd, 0);
        DestroyWindow(hWnd);
        return TRUE;
    }

    //--------------------------------------------------------------------------

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

