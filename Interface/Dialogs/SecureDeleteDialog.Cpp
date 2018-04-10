#ifndef SecureDeleteDialog_Cpp
#define SecureDeleteDialog_Cpp
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
using namespace NSWFL::KeyGeneration; 
using namespace NSWFL::Conversion;
using namespace NSWFL::ListView;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _tag_SECUREDELETETHREADINFO{
	DWORD dwPasses;
	HWND hList;
	HWND hOwner;
	HANDLE hThread;
	HANDLE hEvent;
} SECUREDELETETHREADINFO, *LPSECUREDELETETHREADINFO;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool gbRunThread = true;
bool gbThreadStopped = true;

int giSDeleteSortCol = -1;
int giSDeleteSortDirection = -1;

bool IndexSecureDeleteDirectory(const char *sinFolder, HWND hOwner, HWND hList, int iRecursionCount);
DWORD WINAPI SecureDeleteThreadProc(LPVOID lpData);
void AddSDeleteItem(HWND hWnd, HWND hList, const char *sFilename);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI SecureDeleteThreadProc(LPVOID lpData)
{
	int iAttemptCount = 0;

	bool bAttemptDeletion = true;

	char sTemp[1024];
	char sPath[1024];
	char sGarbage[1024];
	char sRandomPath[1024];
	SECUREDELETETHREADINFO SDTI;
	memcpy_s(&SDTI, sizeof(SDTI), lpData, sizeof(SDTI));
	SetEvent(SDTI.hEvent);

	EnableWindow(SDTI.hList, FALSE);

	while (bAttemptDeletion)
	{
		int iLockedCount = 0;

		bAttemptDeletion = false;
		iAttemptCount++;

		for (int iWorkItem = ListView_GetItemCount(SDTI.hList) - 1; gbRunThread && iWorkItem != -1;)
		{
			ListView_SetItemState(SDTI.hList, iWorkItem, 0, LVIS_SELECTED);
			ListView_EnsureVisible(SDTI.hList, iWorkItem, FALSE);

			ListView_GetItemText(SDTI.hList, iWorkItem, 0, sTemp, sizeof(sTemp));
			if (ListView_GetCheckState(SDTI.hList, iWorkItem))
			{
				ListView_SetItemText(SDTI.hList, iWorkItem, 1, "Wiping");

				if (FileAccess(sTemp, FExist))
				{
					bool bResult = false;
					if (IsDirectory(sTemp))
					{
						GetFilePath(sTemp, sPath, sizeof(sPath));

						GenerateKey((int)(strlen(sTemp) - strlen(sPath)),
							GetTickCount(), GKUPPER_AZ | GKLOWER_AZ | GKNUMBERS, sGarbage);

						sprintf_s(sRandomPath, sizeof(sRandomPath), "%s\\%s", sPath, sGarbage);
						CorrectReversePath(sRandomPath, sizeof(sRandomPath));

						SetFileAttributes(sTemp, FILE_ATTRIBUTE_NORMAL);

						//Rename the file to something fairly random.
						Sleep(1);
						if ((bResult = (MoveFile(sTemp, sRandomPath) > 0)))
						{
							if (!(bResult = (RemoveDirectory(sRandomPath) > 0)))
							{
								MoveFile(sRandomPath, sTemp);
							}
						}
						else {
							MoveFile(sRandomPath, sTemp);
						}
					}
					else {
						bResult = SDeleteFile(sTemp, SDTI.dwPasses);
					}

					if (bResult)
					{
						ListView_DeleteItem(SDTI.hList, iWorkItem);
						iWorkItem--;
					}
					else {
						char sMsg[255];
						sprintf_s(sMsg, sizeof(sMsg), "Locked (Attmept %d)", iAttemptCount);
						ListView_SetItemText(SDTI.hList, iWorkItem, 1, sMsg);
						ListView_SetCheckState(SDTI.hList, iWorkItem, FALSE);
						iLockedCount++;
					}
				}
				else
				{
					ListView_DeleteItem(SDTI.hList, iWorkItem);
					iWorkItem--;
				}
			}
			else {
				//ListView_DeleteItem(SDTI.hList, iWorkItem);
				iWorkItem--;
			}
		}

		if (iLockedCount > 0 && iAttemptCount < 5 && gbRunThread)
		{
			bAttemptDeletion = true;

			for (int iWorkItem = 0; iWorkItem < ListView_GetItemCount(SDTI.hList) && gbRunThread; iWorkItem++)
			{
				char sTemp[255];
				ListView_GetItemText(SDTI.hList, iWorkItem, 1, sTemp, sizeof(sTemp));

				if (_strnicmp(sTemp, "Locked", 6) == 0)
				{
					ListView_SetCheckState(SDTI.hList, iWorkItem, TRUE);
				}
			}
		}
	}

	EnableWindow(SDTI.hList, TRUE);
	EnableWindow(GetDlgItem(SDTI.hOwner, IDC_PASSES), TRUE);
	EnableWindow(GetDlgItem(SDTI.hOwner, IDC_PASSESSPIN), TRUE);
	EnableWindow(GetDlgItem(SDTI.hOwner, IDC_DELETE), TRUE);
	EnableWindow(GetDlgItem(SDTI.hOwner, IDC_CLOSE), TRUE);
	EnableWindow(GetDlgItem(SDTI.hOwner, IDC_CLEARLIST), TRUE);
	EnableWindow(GetDlgItem(SDTI.hOwner, IDC_SELECTALL), TRUE);
	EnableWindow(GetDlgItem(SDTI.hOwner, IDC_SELECTNONE), TRUE);
	EnableWindow(GetDlgItem(SDTI.hOwner, IDC_REMOVE), TRUE);
	EnableWindow(GetDlgItem(SDTI.hOwner, IDC_ADDFILES), TRUE);
	EnableWindow(GetDlgItem(SDTI.hOwner, IDC_ADDFOLDER), TRUE);

	Set_Text(GetDlgItem(SDTI.hOwner, IDC_CLOSE), "Close");

	CloseHandle(SDTI.hThread);

	gbThreadStopped = true;

	if(gbRunThread)
	{
		MessageBox(SDTI.hOwner, "Complete!", gsTitleCaption, MB_ICONINFORMATION);
	}
	else{
		MessageBox(SDTI.hOwner, "Cancelled!", gsTitleCaption, MB_ICONINFORMATION);
	}

	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AddSDeleteItem(HWND hWnd, HWND hList, const char *sFilename)
{
	if(IsDirectory(sFilename))
	{
		LV_ITEM lvItem;
		memset(&lvItem, 0, sizeof(lvItem));

		lvItem.pszText = (char *)sFilename;
		lvItem.mask = LVIF_TEXT|LVIF_IMAGE;
		lvItem.iImage = 0;
		lvItem.iSubItem = 0;
		int iItem = ListView_InsertItem(hList, &lvItem);

		ListView_SetCheckState(hList, iItem, TRUE);
		lvItem.pszText = "";
		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = iItem;
		lvItem.iSubItem = 1;
		ListView_SetItem(hList, &lvItem);

		IndexSecureDeleteDirectory(sFilename, hWnd, hList, 0);
	}
	else {
		char sText[MAX_PATH];

		LV_ITEM lvItem;
		memset(&lvItem, 0, sizeof(lvItem));

		lvItem.pszText = (char *)sFilename;
		lvItem.mask = LVIF_TEXT|LVIF_IMAGE;
		lvItem.iImage = 1;
		lvItem.iSubItem = 0;
		int iItem = ListView_InsertItem(hList, &lvItem);

		__int64 i64FileSize = 0;

		if(FileSize(sFilename, &i64FileSize))
		{
			ListView_SetCheckState(hList, iItem, TRUE);
			I64FormatInteger(sText, sizeof(sText), i64FileSize);
		}
		else{
			strcpy_s(sText, sizeof(sText), "Locked");
		}

		lvItem.pszText  = sText;
		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = iItem;
		lvItem.iSubItem = 1;
		ListView_SetItem(hList, &lvItem);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool IndexSecureDeleteDirectory(const char *sinFolder,
									  HWND hOwner, HWND hList,
									  int iRecursionCount)
{
	WIN32_FIND_DATA FD;
	memset(&FD, 0, sizeof(WIN32_FIND_DATA));

	HANDLE hFileHandle = NULL;
	char sTempDir[MAX_PATH];
	int iDirLen = (int)strlen(sinFolder);

	if(iDirLen > 255)
	{
		SafeMsgBox(hOwner, "The root path name is too long.",
			gsTitleCaption, MB_ICONSTOP | MB_APPLMODAL);
		return false;
	}

	sprintf_s(sTempDir, sizeof(sTempDir), "%s\\%s", sinFolder, "*.*");
	if((hFileHandle = FindFirstFile(sTempDir, &FD)) == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	do{
		if( strcmp(FD.cFileName, ".") && strcmp(FD.cFileName, "..") )
		{
			if(FD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if(DoesPathContainUnicode(FD.cFileName))
				{
					SafeMsgBox(hOwner,
						"One or more of the folder names under the directory you are accessing"
						" contain UNICODE\r\ncharacters, which are not supported."
						" All files and sub-folders of these directories will be skipped.",
						gsTitleCaption, MB_ICONASTERISK);
				}
				else{
					if(iDirLen + strlen(FD.cFileName) >= 255)
					{
						char sError[2048];
						sprintf_s(sError, sizeof(sError),
							"One of folder names that you are accessing is too"
							" long to be safely accessed\r\n\r\n%d folder(s) deep.\r\n%s\r\n\r\n"
							" Would you like to skip this file and continue?",
							iRecursionCount, FD.cFileName);

						if(SafeMsgBox(hOwner, sError, gsTitleCaption, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1) == IDYES)
						{
							continue;
						}
						else{
							return false; //Fail out of recursion.
						}
					}

					sprintf_s(sTempDir, sizeof(sTempDir), "%s\\%s", sinFolder, FD.cFileName);
					CorrectReversePath(sTempDir, sizeof(sTempDir));

					LV_ITEM lvItem;
					memset(&lvItem, 0, sizeof(lvItem));

					lvItem.pszText  = sTempDir;
					lvItem.mask = LVIF_TEXT|LVIF_IMAGE;
					lvItem.iImage = 0;
					lvItem.iSubItem = 0;
					int iItem = ListView_InsertItem(hList, &lvItem);

					ListView_SetCheckState(hList, iItem, TRUE);
					lvItem.pszText  = "";
					lvItem.mask = LVIF_TEXT;
					lvItem.iItem = iItem;
					lvItem.iSubItem = 1;
					ListView_SetItem(hList, &lvItem);

					if(!IndexSecureDeleteDirectory(
						sTempDir, hOwner, hList, iRecursionCount + 1))
					{

						char sError[2048];
						sprintf_s(sError, sizeof(sError),
							"One of folders that you are accessing could not be accessed.\r\n"
							" Would you like to skip this folder and continue?\r\n\r\n\"%s\"",
							FD.cFileName);

						if(SafeMsgBox(hOwner, sError, gsTitleCaption, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1) == IDYES)
						{
							continue;
						}
						else{
							return false; //Fail out of recursion.
						}
					}
				}
			}
			else {
				if(iDirLen + strlen(FD.cFileName) >= 255)
				{
					char sError[2048];
					sprintf_s(sError, sizeof(sError),
						"One of the file names that you are trying to access is too"
						" long to be safely accessed\r\n\r\n%d folder(s) deep.\r\n%s\r\n\r\n"
						" Would you like to skip this file and continue?",
						iRecursionCount, FD.cFileName);

					if(SafeMsgBox(hOwner, sError, gsTitleCaption, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1) == IDYES)
					{
						continue;
					}
					else{
						return false; //Fail out of recursion.
					}
				}

				LV_ITEM lvItem;
				memset(&lvItem, 0, sizeof(lvItem));

				sprintf_s(sTempDir, sizeof(sTempDir), "%s\\%s",
				sinFolder, FD.cFileName);
				CorrectReversePath(sTempDir, sizeof(sTempDir));

				lvItem.pszText  = sTempDir;
				lvItem.mask = LVIF_TEXT|LVIF_IMAGE;
				lvItem.iImage = 1;
				lvItem.iSubItem = 0;
				int iItem = ListView_InsertItem(hList, &lvItem);

				ListView_SetCheckState(hList, iItem, TRUE);

				ULARGE_INTEGER FileSz;
				FileSz.HighPart = FD.nFileSizeHigh;
				FileSz.LowPart = FD.nFileSizeLow;

				I64FormatInteger(sTempDir, sizeof(sTempDir), FileSz.QuadPart);
				lvItem.pszText  = sTempDir;
				lvItem.mask = LVIF_TEXT;
				lvItem.iItem = iItem;
				lvItem.iSubItem = 1;
				ListView_SetItem(hList, &lvItem);
			}
		}
	}
	while(FindNextFile(hFileHandle, &FD));

	FindClose(hFileHandle);

	if(gdwCurrentJobType == JOB_TYPE_CANCEL)
	{
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK SecureDeleteDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//--------------------------------------------------------------------------

	static HWND hDelList = NULL;
	static HWND hPasses = NULL;
	static HIMAGELIST hImgLst = NULL;

	//--------------------------------------------------------------------------

	if(uMsg == WM_INITDIALOG)
	{
		HMENU hSystemMenu = GetSystemMenu(hWnd, FALSE);
		AppendMenu(hSystemMenu, MF_SEPARATOR, 0, 0);
		AppendMenu(hSystemMenu, MF_STRING, WM_ABOUT_DIALOG, "About");

		SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Secure Delete");
		SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN)) );

		hDelList = GetDlgItem(hWnd, IDC_DELETELIST);
		hPasses  = GetDlgItem(hWnd, IDC_PASSES);

		if(gdwSecureDeletePasses > 0)
		{
			Set_LongFormatted(hPasses, gdwSecureDeletePasses);
		}
		else{
			Set_Long(hPasses, 1);
		}

		SendMessage(GetDlgItem(hWnd, IDC_PASSESSPIN),(UINT)UDM_SETBUDDY, (WPARAM)hPasses, (LPARAM)NULL);
		SendMessage(GetDlgItem(hWnd, IDC_PASSESSPIN),(UINT)UDM_SETRANGE, (WPARAM)NULL,(LPARAM)MAKELONG(100, 0));

		DWORD dwFlags = LVS_EX_HEADERDRAGDROP|LVS_EX_CHECKBOXES;

		if(gbEnableFullRowSelect)
		{
			dwFlags |= LVS_EX_FULLROWSELECT;
		}

		if(gbShowGridLines)
		{
			dwFlags |= LVS_EX_GRIDLINES;
		}

		SendMessage(hDelList, (UINT)LVM_SETEXTENDEDLISTVIEWSTYLE, (WPARAM)0, (LPARAM)dwFlags);

		ListView_SetTextColor(hDelList, RGB(0,0,100));

		LV_COLUMN lvColumn;
		memset(&lvColumn, 0, sizeof(LV_COLUMN));

		lvColumn.cx        = 350;
		lvColumn.pszText   = "Name";
		lvColumn.iSubItem  = 0;
		lvColumn.mask      = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
		lvColumn.fmt       = LVCFMT_LEFT;
		ListView_InsertColumn(hDelList, 0, &lvColumn);

		lvColumn.cx        = 150;
		lvColumn.pszText   = "Status";
		lvColumn.iSubItem  = 1;
		lvColumn.mask      = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
		lvColumn.fmt       = LVCFMT_RIGHT;
		ListView_InsertColumn(hDelList, 1, &lvColumn);

		HICON hIcon = NULL;
		hImgLst = ImageList_Create(
			GetSystemMetrics(SM_CXSMICON),
			GetSystemMetrics(SM_CYSMICON),
			ILC_MASK|ILC_COLOR32, 1, 1);

		ListView_SetImageList(hDelList, hImgLst, LVSIL_SMALL);
		
		hIcon = LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_FOLDER16));
		ImageList_AddIcon(hImgLst, hIcon);
		DestroyIcon(hIcon);

		hIcon = LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_UNKNOWN16));
		ImageList_AddIcon(hImgLst, hIcon);
		DestroyIcon(hIcon);

		if(strlen(gsSecureDeletePopulate) > 0)
		{
			AddSDeleteItem(hWnd, hDelList, gsSecureDeletePopulate);
		}

		CenterOverOwner(hWnd);

		ShowWindowInTaskbar(hWnd, true);

		SORTDATA SortData;
		memset(&SortData, 0, sizeof(SortData));
		SortData.hwndList = hDelList;
		SortData.isortSubItem = 0;
		SortData.iUPDOWN = SORT_DIR_ASCENDING;
		SortData.iColType = COL_STRING;
		ListView_SortItemsEx(hDelList, CompListEx, (LPARAM)&SortData);

		return TRUE; // Return TRUE to set the keyboard focus, Otherwise return FALSE
	}

	//--------------------------------------------------------------------------

	else if(uMsg == WM_DROPFILES)
	{
		if(!gbThreadStopped)
		{
			MessageBox(hWnd, "Wait untill the process has completed before adding additional files.",
				gsTitleCaption, MB_ICONINFORMATION);
			return TRUE;
		}

		char sDroppedName[MAX_PATH];
		HDROP hFilesInfo = (HDROP)wParam;
		int iTotalFiles = DragQueryFile(hFilesInfo, -1, NULL, 0);

		LV_ITEM lvItem;
		memset(&lvItem, 0, sizeof(lvItem));

		int iItem = -1;
		__int64 i64FileSize = 0;

		for(int iIndex = 0; iIndex < iTotalFiles; iIndex++)
		{
			DragQueryFile(hFilesInfo, iIndex, (LPSTR)sDroppedName, sizeof(sDroppedName));
			if(strlen(sDroppedName) > 3)
			{
				AddSDeleteItem(hWnd, hDelList, sDroppedName);
			}
			else{
				MessageBox(hWnd,
					"For your protection, you cannot delete the root directory.",
					gsTitleCaption, MB_ICONINFORMATION);
			}
		}

		SORTDATA SortData;
		memset(&SortData, 0, sizeof(SortData));
		SortData.hwndList = hDelList;
		SortData.isortSubItem = 0;
		SortData.iUPDOWN = SORT_DIR_ASCENDING;
		SortData.iColType = COL_STRING;
		ListView_SortItemsEx(hDelList, CompListEx, (LPARAM)&SortData);
	}

	//--------------------------------------------------------------------------

	else if(uMsg == WM_COMMAND)
	{
		if(wParam == IDC_CLEARLIST)
		{
			if(MessageBox(hWnd, "Remove all items from the list?\r\n"
				"Note: This does *NOT* delete any files.",
				gsTitleCaption, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2) == IDYES)
			{
				ListView_DeleteAllItems(hDelList);
			}
			return TRUE;
		}
		else if(wParam == IDC_DELETE || wParam == IDOK)
		{
			int iListViewItems = ListView_GetItemCount(hDelList);
			if(iListViewItems > 0)
			{
				char sFormat[64];
				char sTemp[1024];

				SECUREDELETETHREADINFO SDTI;
				memset(&SDTI, 0, sizeof(SDTI));
				SDTI.hOwner = hWnd;
				SDTI.hList = hDelList;
				SDTI.dwPasses = Get_Long(hPasses);
				
				if(SDTI.dwPasses <= 0 || SDTI.dwPasses > 100)
				{
					MessageBox(hWnd,
						"You must specify a number of \"Secure Delete Passes\" between 1 and 100.",
						gsTitleCaption, MB_ICONINFORMATION);
					return TRUE;
				}

				sprintf_s(sTemp, sizeof(sTemp),
					"Are you sure you want to permanently delete these %s files / folders?",
					FormatInteger(sFormat, sizeof(sFormat), iListViewItems));

				if(MessageBox(hWnd, sTemp, gsTitleCaption, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2) != IDYES)
				{
					return TRUE;
				}

				gbRunThread = true;
				gbThreadStopped = false;

				SDTI.hEvent = CreateEvent(NULL, FALSE, FALSE, "SecureDeleteThread");
				SDTI.hThread = CreateThread(NULL, NULL, SecureDeleteThreadProc, &SDTI, CREATE_SUSPENDED, NULL); 

				EnableWindow(GetDlgItem(hWnd, IDC_PASSES), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_PASSESSPIN), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_DELETE), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_CLEARLIST), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_SELECTALL), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_SELECTNONE), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_REMOVE), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_ADDFILES), FALSE);
				EnableWindow(GetDlgItem(hWnd, IDC_ADDFOLDER), FALSE);

				Set_Text(GetDlgItem(hWnd, IDC_CLOSE), "Cancel");

				//We create suspended then resume so we can be sure that SDTI.hThread
				//	contains a valid handle upon thread initilization.
				ResumeThread(SDTI.hThread);

				WaitForSingleObject(SDTI.hEvent, INFINITE);
				CloseHandle(SDTI.hEvent);
			}
			else{
				MessageBox(hWnd, "There is nothing to delete.", gsTitleCaption, MB_ICONINFORMATION);
			}
		}
		else if(wParam == IDC_CLOSE || wParam == IDCANCEL)
		{
			char sTemp[64];

			Get_Text(GetDlgItem(hWnd, IDC_CLOSE), sTemp, sizeof(sTemp));
			if(_strcmpi(sTemp, "Close") == 0)
			{
				if(ListView_GetItemCount(hDelList) > 0)
				{
					if(MessageBox(hWnd, "Are you sure you want to close without deleting?",
						gsTitleCaption, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2) != IDYES)
					{
						return TRUE;
					}
				}

				ImageList_Destroy(hImgLst);
				EndDialog(hWnd, 0);
				DestroyWindow(hWnd);
			}
			else{
				if(MessageBox(hWnd, "Are you sure you want to cancel?",
					gsTitleCaption, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2) == IDYES)
				{
					if(!gbThreadStopped)
					{
						gbRunThread = false;
						EnableWindow(GetDlgItem(hWnd, IDC_CLOSE), FALSE);
					}
				}
			}

			return TRUE;
		}
		else if (wParam == IDC_SELECTALL)
		{
			LVCheckAllRows(hDelList);
		}
		else if (wParam == IDC_SELECTNONE)
		{
			LVUnCheckAllRows(hDelList);
		}
		else if (wParam == IDC_REMOVE)
		{
			LVDeleteSelectedItems(hDelList);
		}
		else if (wParam == IDC_ADDFILES)
		{
			int iBufferSize = 1024 * 1024;
			char *sFileNames = (char*)calloc(iBufferSize, 1);
			char sCurrDir[MAX_PATH];
			memset(sFileNames, 0, sizeof(sFileNames));
			memset(sCurrDir, 0, sizeof(sCurrDir));

			OPENFILENAME OFN;
			memset(&OFN, 0, sizeof(OFN));

			OFN.lpstrCustomFilter = NULL;
			OFN.nMaxCustFilter = 0;
			OFN.lpstrFileTitle = NULL;
			OFN.nMaxFileTitle = 0;

			if (NSWFL::File::Get_CurrentDirectory(sCurrDir, MAX_PATH))
			{
				OFN.lpstrInitialDir = sCurrDir;
			}

			OFN.lStructSize = sizeof(OFN);
			OFN.lpstrTitle = "Select file to securely delete";
			OFN.lpstrFilter = "All-Files\0*.*\0\0";
			OFN.hwndOwner = hWnd;
			OFN.nFilterIndex = 1;
			OFN.lpstrFile = sFileNames;
			OFN.nMaxFile = iBufferSize;
			OFN.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_LONGNAMES | OFN_PATHMUSTEXIST;

			if (GetOpenFileName(&OFN))
			{
				char *sFile = sFileNames;
				char *sDirectory = sFile;
				sFile += (strlen(sDirectory) + 1);
				while (*sFile)
				{
					char sFileName[MAX_PATH];
					sprintf_s(sFileName, sizeof(sFileName), "%s\\%s", sDirectory, sFile);
					AddSDeleteItem(hWnd, hDelList, sFileName);
					sFile += (strlen(sFile) + 1);
				}

				SORTDATA SortData;
				memset(&SortData, 0, sizeof(SortData));
				SortData.hwndList = hDelList;
				SortData.isortSubItem = 0;
				SortData.iUPDOWN = SORT_DIR_ASCENDING;
				SortData.iColType = COL_STRING;
				ListView_SortItemsEx(hDelList, CompListEx, (LPARAM)&SortData);
			}

			free(sFileNames);

			return TRUE;
		}
		else if (wParam == IDC_ADDFOLDER)
		{
			char sFolder[MAX_PATH];
			memset(sFolder, 0, sizeof(sFolder));

			if (BrowseDirectory(hWnd, "Select a folder to securely delete.", BIF_RETURNONLYFSDIRS | BIF_USENEWUI, sFolder, sizeof(sFolder)))
			{
				AddSDeleteItem(hWnd, hDelList, sFolder);

				SORTDATA SortData;
				memset(&SortData, 0, sizeof(SortData));
				SortData.hwndList = hDelList;
				SortData.isortSubItem = 0;
				SortData.iUPDOWN = SORT_DIR_ASCENDING;
				SortData.iColType = COL_STRING;
				ListView_SortItemsEx(hDelList, CompListEx, (LPARAM)&SortData);
			}
			return TRUE;
		}
		if (wParam == IDC_HELPBUTTON)
		{
			OpenHelpFile("/SecureDelete/HowToUse.html");
			return TRUE;
		}

		return FALSE;
	}

	//---------------------------------------------------------------------------------------------------------------------

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
		char sTemp[64];

		Get_Text(GetDlgItem(hWnd, IDC_CLOSE), sTemp, sizeof(sTemp));
		if(_strcmpi(sTemp, "Close") == 0)
		{
			if(ListView_GetItemCount(hDelList) > 0)
			{
				if(MessageBox(hWnd, "Are you sure you want to close?",
					gsTitleCaption, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2) != IDYES)
				{
					return TRUE;
				}
			}

			ImageList_Destroy(hImgLst);
			EndDialog(hWnd, 0);
			DestroyWindow(hWnd);
		}
		else{
			if(!gbRunThread)
			{
				MessageBox(hWnd, "Secure Archive is currently processing a file, the process\r\n"
								  "will be cancelled as soon as this file is finished.",
					gsTitleCaption, MB_ICONINFORMATION);
				return TRUE;
			}
			else{
				if(MessageBox(hWnd, "Are you sure you want to cancel?",
					gsTitleCaption, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2) == IDYES)
				{
					if(!gbThreadStopped)
					{
						gbRunThread = false;
						EnableWindow(GetDlgItem(hWnd, IDC_CLOSE), FALSE);
					}
				}
			}
		}

		return TRUE;
	}

	//--------------------------------------------------------------------------

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

