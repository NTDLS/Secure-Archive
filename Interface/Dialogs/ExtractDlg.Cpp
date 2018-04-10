#ifndef _ExtractDLG_CPP
#define _ExtractDLG_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <ShlObj.H>
#include <Stdio.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "../Source/Package.H"
#include "../Source/Entry.H"
#include "../Source/Routines.H"
#ifndef _SELFEXTRACTOR_APP
#include "../Source/FolderTree.H"
#include "../Source/ListView.H"
#endif

#include "../Dialogs/MainDlg.H"
#include "../Dialogs/ProgressDialog.H"
#ifndef _SELFEXTRACTOR_APP
#include "../Dialogs/NewFolderDlg.H"
#endif
#include "../Dialogs/NetLogoDlg.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

WNDPROC gpOldExtractToLocationProc;
HWND hExtractDialog = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK NewExtractToLocationProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if(uMsg == WM_GETDLGCODE)
	{
		return (DLGC_WANTALLKEYS | CallWindowProc(gpOldExtractToLocationProc, hWnd, uMsg, wParam, lParam));
	}
	else if(uMsg == WM_CHAR)
	{
		if(wParam == VK_RETURN)
		{
			return 0;
		}
		else if(wParam == VK_TAB)
		{
			return 0;
		}
	}
	else if(uMsg == WM_KEYDOWN || uMsg == WM_KILLFOCUS)
	{
		if((wParam == VK_RETURN || wParam == VK_TAB) || uMsg == WM_KILLFOCUS)
		{
		    int iLength = GetWindowTextLength(hWnd);
			if(iLength > 0)
			{
				char *sText = (char*) calloc(sizeof(char), iLength + 1);
				iLength = GetWindowText(hWnd, sText, iLength + 1);
				if(iLength > 2 && strncmp(sText, "\\\\", 2) == 0)
				{
					EnableWindow(GetDlgItem(hExtractDialog, IDC_DECRYPT), TRUE);
				}
				else {
					SendMessage(hExtractDialog, WM_COMMAND, IDC_GOTO, 0);
				}
				free(sText);
			}
			else {
				SendMessage(hExtractDialog, WM_COMMAND, IDC_GOTO, 0);
			}

			if(uMsg == WM_KEYDOWN)
			{
				if(wParam == VK_TAB)
				{
					PostMessage(hExtractDialog, WM_NEXTDLGCTL, 0, 0L);
				}
				else if(wParam == VK_RETURN) {
					SendMessage(hExtractDialog, WM_COMMAND, IDC_DECRYPT, 0);
				}
			}
		}
	}

	return CallWindowProc(gpOldExtractToLocationProc, hWnd, uMsg, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK ExtractDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //--------------------------------------------------------------------------

    static FolderTree FolderTree;

    static HWND hAllFilesRadio = NULL;
    static HWND hBrowseTree = NULL;
    static HWND hExtractToLocation = NULL;
    static HWND hSelectedRadio = NULL;
    static HWND hSkipOlderFilesCB = NULL;
    static HWND hUseFolderNamesCB = NULL;
    static HWND hOverwriteCB = NULL;

    //--------------------------------------------------------------------------

    if(uMsg == WM_INITDIALOG) // Received A Initilize Dialog Message
    {
        hExtractDialog = hWnd;

        HMENU hSystemMenu = GetSystemMenu(hWnd, FALSE);
        AppendMenu(hSystemMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hSystemMenu, MF_STRING, WM_ABOUT_DIALOG, "About");

		SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Extract");
        SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN)) );

        hSelectedRadio    = GetDlgItem(hWnd, IDC_SELECTEDRADIO);
        hAllFilesRadio     = GetDlgItem(hWnd, IDC_ALLRADIO);
        hExtractToLocation = GetDlgItem(hWnd, IDC_DECRYPTLOCATION);
        hBrowseTree        = GetDlgItem(hWnd, IDC_BROWSETREE);

		gpOldExtractToLocationProc = (WNDPROC)
			SetWindowLongPtr(hExtractToLocation, GWLP_WNDPROC, (LRESULT) &NewExtractToLocationProc);

		CheckDlgButton(hWnd, IDC_USEFOLDERS, BST_CHECKED);

		InvalidateRect(hBrowseTree, NULL, 0);

        if(ListView_GetSelectedCount(ghFileList) > 0)
        {
            CheckDlgButton(hWnd, IDC_SELECTEDRADIO, BST_CHECKED);
            CheckDlgButton(hWnd, IDC_ALLRADIO, BST_UNCHECKED);
            EnableWindow(hSelectedRadio, TRUE);
        }
        else {
            CheckDlgButton(hWnd, IDC_ALLRADIO, BST_CHECKED);
            CheckDlgButton(hWnd, IDC_SELECTEDRADIO, BST_UNCHECKED);
            EnableWindow(hSelectedRadio, FALSE);
        }

        FolderTree.InitFolderTree(hBrowseTree, GetDlgItem(hWnd, IDC_DECRYPT), hExtractToLocation, NULL);

        char FullPath[MAX_PATH + 1];
        char sDrive[MAX_PATH + 1];
        char sDir[MAX_PATH + 1];

        _splitpath_s(gsArchiveName,
			sDrive, sizeof(sDrive),
			sDir, sizeof(sDir),
			NULL, 0,
			NULL, 0);

        _makepath_s(FullPath, sizeof(FullPath), sDrive, sDir, NULL, NULL);

		Set_Text(hExtractToLocation, FullPath);

		if(strncmp(FullPath, "\\\\", 2) == 0)
		{
			EnableWindow(GetDlgItem(hWnd, IDC_DECRYPT), TRUE);
		}
		else{
	        FolderTree.SelectPath(FullPath);
		}

        CenterOverOwner(hWnd);

		if(!IsWindowVisible(GetParent(hWnd)))
		{
			ShowWindowInTaskbar(hWnd, true);
		}

		return TRUE; // Return TRUE to set the keyboard focus, Otherwise return FALSE
    }

    //--------------------------------------------------------------------------

    else if(FolderTree.FolderTreeHandler(hWnd, uMsg, wParam, lParam))
    {
        return TRUE;
    }

    //--------------------------------------------------------------------------

    else if(uMsg == WM_COMMAND)
    {
		if(wParam == IDC_NEWFOLDER)
        {
            int iLength = Get_Text(hExtractToLocation, gsExtractLocation, 255);
			if(iLength > 0 || iLength <= 255)
			{
				if(DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_NEWFOLDER), hWnd, NewFolderDialog) == 1)
				{
					char sNewPath[MAX_PATH + 1];
					_makepath_s(sNewPath, sizeof(sNewPath), NULL, gsExtractLocation, sNewFolderName, NULL);
					strcat_s(sNewPath, sizeof(sNewPath), "\\");
					CustomCreateFolderStructure(sNewPath);

					//This is a bad way to refresh the treview.
					FolderTree.UnInitFolderTree();
					FolderTree.InitFolderTree(hBrowseTree, GetDlgItem(hWnd, IDC_DECRYPT), hExtractToLocation, NULL);
					FolderTree.SelectPath(sNewPath);
				}
			}
			return TRUE;
		}

		if(wParam == IDC_GOTO)
        {
            int iLength = Get_Text(hExtractToLocation, gsExtractLocation, 255);
			if(iLength > 0 || iLength <= 255)
			{
				FolderTree.SelectPath(gsExtractLocation);
			}
			return TRUE;
		}

		if(wParam == IDC_DECRYPT || wParam == IDOK)
        {
            int iLength = Get_Text(hExtractToLocation, gsExtractLocation, 255);
            if(iLength <= 0 || iLength > 255)
            {
                SafeMsgBox(hWnd, "You must specifiy a location to extract the files to.", gsTitleCaption, 0);
                return TRUE;
            }

            if(gsExtractLocation[strlen(gsExtractLocation) - 1] != '\\')
            {
                strcat_s(gsExtractLocation, sizeof(gsExtractLocation), "\\");
            }

			gbOverwriteDlft = (IsDlgButtonChecked(hWnd, IDC_OVERWRITE) == BST_CHECKED);
			gbUseFolderNames = (IsDlgButtonChecked(hWnd, IDC_USEFOLDERS) == BST_CHECKED);

			if(IsDlgButtonChecked(hWnd, IDC_SELECTEDRADIO))
            {
                gdwCurrentJobType = JOB_TYPE_EXTRACT_SELECTED;
            }
            else {
                gdwCurrentJobType = JOB_TYPE_EXTRACT_ALL;
            }

            FolderTree.UnInitFolderTree();
            EndDialog(hWnd, 1);

            return TRUE;
        }

        if(wParam == IDC_CANCEL || wParam == IDCANCEL)
        {
            FolderTree.UnInitFolderTree();
            EndDialog(hWnd, 0);
            return TRUE;
        }

        if(wParam == IDC_HELPBUTTON)
        {
			OpenHelpFile("Decrypting/WalkThrough.html");
            return TRUE;
        }
        return FALSE;
    }

    //--------------------------------------------------------------------------

    else if(uMsg == WM_PAINT)
    {
        HDC ThisHDC;
        PAINTSTRUCT ThisPS;

        ThisHDC = BeginPaint(hWnd, &ThisPS);

        // Any painting should be done here

        EndPaint(hWnd, &ThisPS);
        return TRUE;
    }

    //--------------------------------------------------------------------------

    else if(uMsg == WM_SYSCOMMAND) // Received A System Menu Message
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

    else if(uMsg == WM_CLOSE)  // Received Close Message
    {
        FolderTree.UnInitFolderTree();
        EndDialog(hWnd, 0);
        return TRUE;
    }

    //--------------------------------------------------------------------------
	/*
    else if(uMsg == WM_SIZE)
	{
		if(wParam == SIZE_MINIMIZED || wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
		{
			HWND hParent = GetParent(hWnd);
			if(hParent)
			{
				if(wParam == SIZE_MINIMIZED)
				{
					//ShowWindow(hParent, SW_MINIMIZE);
				}
				else if(wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
				{
					//ShowWindow(hWnd, SW_MAXIMIZE);
				}
			}
		}
	}
	*/
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

