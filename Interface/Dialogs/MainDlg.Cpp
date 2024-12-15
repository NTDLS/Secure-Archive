#ifndef _MAINDLG_CPP
#define _MAINDLG_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <CommCtrl.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "../Source/Package.H"
#include "../Source/Workers.H"
#include "../Source/Entry.H"
#include "../Source/Routines.H"
#include "../Source/DragDrop.H"
#include "../Source/ListView.H"
#include "../Source/Init.H"

#include "../Dialogs/MainDlg.H"
#include "../Dialogs/ReadingDlg.H"
#include "../Dialogs/ProgressDialog.H"
#include "../Dialogs/ExtractDlg.H"
#include "../Dialogs/OpenPassDlg.H"
#include "../Dialogs/NewPassDlg.H"
#include "../Dialogs/FileInfoDlg.H"
#include "../Dialogs/SearchDlg.H"
#include "../Dialogs/NetLogoDlg.H"
#include "../Dialogs/HardKeyDlg.H"
#include "../Dialogs/SecureDeleteDialog.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 
using namespace NSWFL::File; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ResetDialog(void)
{
	gbArchiveOpen = false;

	ResetDefaultAttributes();
	ResetEncryptionOptions();
	gdwCurrentJobType = 0;

	memset(gsArchiveName, 0, sizeof(gsArchiveName));
	memset(gsWorkingOutput, 0, sizeof(gsWorkingOutput));

	strcpy_s(gsWorkingOutput, sizeof(gsWorkingOutput), "");
	strcpy_s(gsArchiveName, sizeof(gsArchiveName), "");
	strcpy_s(gsExtractLocation, sizeof(gsExtractLocation), "");
	SetPassword(&gPassword, NULL, 0);

	SendMessage(hStatusBar, (UINT)SB_SETTEXT, (WPARAM)1, (LPARAM)"");
	SendMessage(hStatusBar, (UINT)SB_SETTEXT, (WPARAM)0, (LPARAM)"Selected: 0 Files, 0 B");

	EnableButton(hTB_New, true);
	EnableButton(hTB_Open, true);
	EnableButton(hTB_AddFiles, true);
	EnableButton(hTB_Delete, false);
	EnableButton(hTB_Extract, false);
	//EnableButton(hTB_View, true);
	EnableButton(hTB_Info, false);
	EnableButton(hTB_Config, true);
	EnableButton(hTB_Find, false);
	EnableButton(hTB_Exit, true);

	char sCaption[1024];
	GetAppTitle(sCaption, sizeof(sCaption));
	SendMessage(hMainDialog, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)sCaption);

	ListView_DeleteAllItems(ghFileList);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void NeedToOpenArchive(HWND hOwner)
{
	SafeMsgBox(hOwner,
		"You need to open or create an archive to perform this action.",
		gsTitleCaption, MB_ICONINFORMATION);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void StartDialogProcesses(HWND hWnd)
{
	if(gdwCurrentJobType == JOB_TYPE_EXTRACT_ALL)
	{
		if(strlen(gsWorkingOutput) == 0)
		{
			if(DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_EXTRACT), hWnd, ExtractDialog) == 1)
			{
				CreateWorker(ExtractEntireArchiveProc, (LPVOID) gsArchiveName);
				RunProgressDialog(hWnd);
			}
		}
		else{
			CreateWorker(ExtractEntireArchiveProc, (LPVOID) gsArchiveName);
			RunProgressDialog(hWnd);
		}
	}
	else if(gdwCurrentJobType == JOB_TYPE_READING)
	{
		if(RunReadingDialog(NULL) != 1)
		{
			ImageList_Destroy(hImageList);
			ImageList_Destroy(hImageListBig);
			EndDialog(hWnd, 0);
			if(hSearchDialog) EndDialog(hSearchDialog, 0);
			DestroyWindow(hWnd);
		}
	}
	else if(gdwCurrentJobType == JOB_TYPE_PACKING_FOLDER || gdwCurrentJobType == JOB_TYPE_PACKING_FILE)
	{
		if(RunProgressDialog(NULL) != 1)
		{
			ImageList_Destroy(hImageList);
			ImageList_Destroy(hImageListBig);
			EndDialog(hWnd, 0);
			if(hSearchDialog) EndDialog(hSearchDialog, 0);
			DestroyWindow(hWnd);
		}
		else if(gbAutoClose == false)
		{
			SendMessage(hMainDialog, WM_REFRESH_ARCHIVE, 0, (LPARAM)gsWorkingOutput);
			gbArchiveOpen = true;
		}
	}
	else{
		ResumeThread(hWorkerProc);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK MainDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//---------------------------------------------------------------------------------------------------------------------

	if(uMsg == WM_INITDIALOG) // Received an initialize message.
	{
		hMainDialog = hWnd;

		CreateThread(NULL, 0, MouseOverThread, (LPVOID) gsArchiveName, 0, NULL);

		HMENU hSystemMenu = GetSystemMenu(hWnd, FALSE);
		AppendMenu(hSystemMenu, MF_SEPARATOR, 0, 0);
		AppendMenu(hSystemMenu, MF_STRING, WM_ABOUT_DIALOG, "About");

		ghMenu = LoadMenu(ghAppInstance, MAKEINTRESOURCE(IDR_MAINMENU));
		SetMenu(hWnd, ghMenu);

		if(gbAlwaysOnTop)
		{
			CheckMenuItem(ghMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND|MF_CHECKED);
			SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}
		else{
			CheckMenuItem(ghMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND|MF_UNCHECKED);
			SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}

		char sCaption[1024];
		GetAppTitle(sCaption, sizeof(sCaption));
		SendMessage(hMainDialog, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)sCaption);
		SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN)) );

		ghFileList = InitListView(hWnd);
		hHorizBar = GetDlgItem(hWnd, IDC_HORIZBAR);
		hStatusBar = CreateStatusWindow(WS_CHILD|WS_VISIBLE, "Selected: 0 Files, 0 B", hWnd, 1);

		int Widths[] = {250, -1};

		SendMessage(hStatusBar, (UINT)SB_SETPARTS, (WPARAM)2, (LPARAM)&Widths);

		//---------------------------------------------------------------------------------

		CenterOverOwner(hWnd);

		//---------------------------------------------------------------------------------

		RECT rcHwnd;
		GetClientRect(hWnd, &rcHwnd);
		MoveWindow(ghFileList, 0, 78, rcHwnd.right, rcHwnd.bottom - 100, TRUE);
		MoveWindow(hStatusBar, rcHwnd.right-20, rcHwnd.bottom-20, rcHwnd.right, rcHwnd.bottom, TRUE);
		MoveWindow(hHorizBar, 0, 2, rcHwnd.right, 2, TRUE);

		//---------------------------------------------------------------------------------

		StartDialogProcesses(hWnd);

		if(gbAutoClose == true)
		{
			EndDialog(hWnd, 0);
			if(hSearchDialog) EndDialog(hSearchDialog, 0);
			DestroyWindow(hWnd);			
			return false;
		}

		//---------------------------------------------------------------------------------

		UpdateButtons();

		//---------------------------------------------------------------------------------

		ShowWindowInTaskbar(hWnd, true);

		return TRUE; // Return TRUE to set the keyboard focus, Otherwise return FALSE
	}

	//---------------------------------------------------------------------------------------------------------------------

	else if(uMsg == WM_REFRESH_ARCHIVE)
	{
		if(FileAccess((char *) lParam, FExist))
		{
			// View an existing archive
			gdwCurrentJobType = JOB_TYPE_READING;
			CreateWorker(ViewExistingProc, (LPVOID) lParam);
			if(RunReadingDialog(hWnd) != 1)
			{
				ImageList_Destroy(hImageList);
				ImageList_Destroy(hImageListBig);
				EndDialog(hWnd, 0);
				if(hSearchDialog) EndDialog(hSearchDialog, 0);
				DestroyWindow(hWnd);
			}
		}
		else{
			ResetDialog(); //The requested file doesn't exist.
		}
	}

	//---------------------------------------------------------------------------------------------------------------------

	else if(uMsg == WM_DROPFILES)
	{
		if(gbEnableDragDrop)
		{
			bool bFirstFile = true;
			bool bCreateNew = false;

			char sExt[MAX_PATH];
			//char sRoot[MAX_PATH];
			char sFullPath[MAX_PATH];

			FilePack fpPack;
			HDROP hFilesInfo = (HDROP)wParam;

			int wIndex = 0;
			int wTotalFiles = DragQueryFile(hFilesInfo, -1, NULL, 0);

			//Save the name of the first sFile in sFullPath, it will be overwritten.
			DragQueryFile(hFilesInfo, wIndex, (LPSTR)sFullPath, MAX_PATH);

			if(wTotalFiles == 1)
			{
				_splitpath_s(sFullPath, NULL, 0, NULL, 0, NULL, 0, sExt, sizeof(sExt));

				if(_strcmpi(sExt, DEFAULTEXT) == 0 || (_strcmpi(sExt, ".exe") == 0 && GetExeEnd(sFullPath) > 0))
				{
					strcpy_s(gsArchiveName, sizeof(gsArchiveName), sFullPath);

					bool bIsUsingBlankPassword = false;
					if(!IsUsingBlankPassword(hWnd, gsArchiveName, &bIsUsingBlankPassword))
					{
						DragFinish(hFilesInfo);
						return TRUE;
					}

					if(bIsUsingBlankPassword)
					{
						SetPassword(&gPassword, gsEmptyPassword);
					}
					else{
						if(DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_OPENPASS), hWnd, OpenPassDialog) != 1)
						{
							DragFinish(hFilesInfo);
							return TRUE;
						}
					}

					DragFinish(hFilesInfo);

					SendMessage(hMainDialog, WM_REFRESH_ARCHIVE, 0, (LPARAM)gsArchiveName);
					return TRUE;
				}
			}

			if(gbArchiveOpen)
			{
				strcpy_s(gsWorkingOutput, sizeof(gsWorkingOutput), gsArchiveName);
			}
			else {
				if(SafeMsgBox(hWnd, "Would you like to create a new archive?", gsTitleCaption, MB_YESNO|MB_ICONQUESTION) == IDNO)
				{
					DragFinish(hFilesInfo);
					return TRUE;
				}

				bCreateNew = true;

				ResetEncryptionOptions();

				// Create a new archive from the directory's contents or files.
				if(DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_NEWPASS), hWnd, NewPassDialog) != 1)
				{
					DragFinish(hFilesInfo);
					return TRUE;
				}
			}

			fpPack.Initialize(hWnd, &AltFilePackage);

			for(wIndex = 0; wIndex < wTotalFiles; wIndex++)
			{
				DragQueryFile(hFilesInfo, wIndex, (LPSTR)sFullPath, MAX_PATH);

				if(IsDirectory(sFullPath))
				{
					if(!fpPack.IndexDirectorySimple(sFullPath, NULL, gbIncludeHiddenFiles, &AltFilePackage))
					{
						char sStatusText[5120];
						sprintf_s(sStatusText, sizeof(sStatusText),
							"An error occurred while trying to index the directory:\r\n'%s'",
							sFullPath);

						SafeMsgBox(hWnd, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

						DragFinish(hFilesInfo);
						fpPack.FreeMemory(&AltFilePackage);
						return TRUE;
					}
				}
				else{
					if(!fpPack.AddSimpleFile(sFullPath, NULL, 0, &AltFilePackage))
					{
						char sStatusText[5120];
						sprintf_s(sStatusText, sizeof(sStatusText),
							"An error occurred while trying to index the file:\r\n'%s'",
							sFullPath);

						SafeMsgBox(hWnd, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

						DragFinish(hFilesInfo);
						fpPack.FreeMemory(&AltFilePackage);
						return TRUE;
					}
				}
			}

			DragFinish(hFilesInfo);

			if(AltFilePackage.dwFileCount > 0)
			{
				gdwCurrentJobType = JOB_TYPE_ADDING_FILES;
				if(FileAccess(gsArchiveName, FExist) && bCreateNew && !(giPkgOptions &OPTION_ADDFILES))
				{
					if(!DeleteFile(gsArchiveName))
					{
						SafeMsgBox(hWnd, "Failed to delete the existing archive.", gsTitleCaption, MB_ICONERROR);
						fpPack.FreeMemory(&AltFilePackage);
						return TRUE;
					}
				}

				CreateWorker(AddFilesProc, NULL);
				RunProgressDialog(hWnd);

				SendMessage(hMainDialog, WM_REFRESH_ARCHIVE, 0, (LPARAM)gsArchiveName);
			}

			fpPack.FreeMemory(&AltFilePackage);

			UpdateButtons();
		}
		return TRUE;
	}

	//---------------------------------------------------------------------------------------------------------------------

	else if(uMsg == WM_LBUTTONUP)
	{
		if(hMainDialog == GetForegroundWindow())
		{
			if(IsMouseOverHwnd(hTB_AddFiles[0]))
			{
				SendMessage(hWnd, WM_COMMAND, IDI_TB_ADDFILES, 0);
				Float(hTB_AddFiles[0]);
				return TRUE;
			}
			else if(IsMouseOverHwnd(hTB_Delete[0]))
			{
				SendMessage(hWnd, WM_COMMAND, IDI_TB_DELETE, 0);
				Float(hTB_Delete[0]);
				return TRUE;
			}
			else if(IsMouseOverHwnd(hTB_New[0]))
			{
				SendMessage(hWnd, WM_COMMAND, IDI_TB_NEW, 0);
				Float(hTB_New[0]);
				return TRUE;
			}
			else if(IsMouseOverHwnd(hTB_Find[0]))
			{
				SendMessage(hWnd, WM_COMMAND, IDI_TB_FIND, 0);
				Float(hTB_Find[0]);
				return TRUE;
			}
			else if(IsMouseOverHwnd(hTB_Open[0]))
			{
				SendMessage(hWnd, WM_COMMAND, IDI_TB_OPEN, 0);
				Float(hTB_Open[0]);
				return TRUE;
			}
			else if(IsMouseOverHwnd(hTB_Config[0]))
			{
				SendMessage(hWnd, WM_COMMAND, IDI_TB_CONFIG, 0);
				Float(hTB_Config[0]);
				return TRUE;
			}
			else if(IsMouseOverHwnd(hTB_Info[0]))
			{
				SendMessage(hWnd, WM_COMMAND, IDI_TB_INFO, 0);
				Float(hTB_Info[0]);
				return TRUE;
			}
			else if(IsMouseOverHwnd(hTB_Extract[0]))
			{
				SendMessage(hWnd, WM_COMMAND, IDI_TB_EXTRACT, 0);
				Float(hTB_Extract[0]);
				return TRUE;
			}
			else if(IsMouseOverHwnd(hTB_Exit[0]))
			{
				SendMessage(hWnd, WM_COMMAND, IDI_TB_EXIT, 0);
				Float(hTB_Exit[0]);
				return TRUE;
			}
		}
	}

	//---------------------------------------------------------------------------------------------------------------------

	else if(uMsg == WM_GETMINMAXINFO)
	{
		MINMAXINFO *pMMI = (MINMAXINFO *)lParam;
		pMMI->ptMinTrackSize.x = 600;
		pMMI->ptMinTrackSize.y = 353;
		return 0;
	}

	//---------------------------------------------------------------------------------------------------------------------

	else if(uMsg == WM_SIZE)
	{
		int nWidth = LOWORD(lParam);
		int nHeight = HIWORD(lParam);

		MoveWindow(ghFileList, 0, 78, nWidth, nHeight - 100, TRUE);
		MoveWindow(hStatusBar, nWidth-20, nHeight-20, nWidth, nHeight, TRUE);
		MoveWindow(hHorizBar, 0, 2, nWidth, 2, TRUE);

		return FALSE;
	}

	//---------------------------------------------------------------------------------------------------------------------

	else if(uMsg == WM_COMMAND)
	{
		if(wParam == ID_OPTIONS_SETTINGS || wParam == IDI_TB_CONFIG)
		{
			LaunchConfigDlg(hWnd);
			return TRUE;
		}
		else if(wParam == ID_OPTIONS_ALWAYSONTOP)
		{
			gbAlwaysOnTop = !gbAlwaysOnTop;

			if(gbAlwaysOnTop)
			{
				CheckMenuItem(ghMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND|MF_CHECKED);
				SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			}
			else{
				CheckMenuItem(ghMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND|MF_UNCHECKED);
				SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			}

			return TRUE;
		}
		else if(wParam == ID_SECURITY_HARDKEYGENERATOR)
		{
			DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_HARDKEY), NULL, HardKeyDlg);
			return TRUE;
		}
		else if(wParam == ID_SECURITY_SECUREDELETEWIZARD)
		{
			DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_SECUREDELETE), NULL, SecureDeleteDialog);
		}
		else if(wParam == ID_SECURITY_SECURELYDELETEFOLDER)
		{
			char sDeleteFolder[MAX_PATH];
			memset(sDeleteFolder, 0, sizeof(sDeleteFolder));

			if(BrowseDirectory(hWnd,
				"Select the folder that you would like to permanently delete.",
				BIF_RETURNONLYFSDIRS|BIF_USENEWUI, sDeleteFolder, sizeof(sDeleteFolder)))
			{
				if(strlen(sDeleteFolder) <= 3)
				{
					MessageBox(hWnd,
						"For your protection, you cannot delete the root directory.",
						gsTitleCaption, MB_ICONINFORMATION);

					return TRUE;
				}

				char sText[2048];
				sprintf_s(sText, sizeof(sText),
					"Are you sure you want to permanently delete\r\n\"%s\"",
					sDeleteFolder);

				if(MessageBox(hWnd, sText, gsTitleCaption, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2) != IDYES)
				{
					return TRUE;
				}
				else{
					DWORD dwSecureDeletePasses = gdwSecureDeletePasses;
					if(dwSecureDeletePasses <= 0)
					{
						dwSecureDeletePasses = 5;
					}

					if(SDeleteFile(sDeleteFolder, dwSecureDeletePasses))
					{
						MessageBox(hWnd, "Complete!", gsTitleCaption, MB_ICONINFORMATION);
					}
					else{
						MessageBox(hWnd, "Could not delete the folder!\r\n"
										 "Either the file path is too long,"
										 " or the folder contains files which are in use.",
										 gsTitleCaption, MB_ICONERROR);
					}
				}
			}

			return TRUE;
		}
		else if(wParam == ID_SECURITY_SECURELYDELETEFILE)
		{
			char sInitialDirectory[MAX_PATH + 1];
			char sDeleteFile[MAX_PATH];

			GetCurrentDirectory(MAX_PATH, sInitialDirectory);

			memset(sDeleteFile, 0, sizeof(sDeleteFile));

			if(OSFileDialog(hWnd, sDeleteFile, sizeof(sDeleteFile), NULL,
				"Select a file to delete.",
				"All-Files\0*.*\0\0", sInitialDirectory, OFD,
				OFN_EXPLORER + OFN_LONGNAMES + OFN_PATHMUSTEXIST))
			{
				if(strlen(sDeleteFile) <= 3) //Just-in-case the user finds some way to pull this off.
				{
					MessageBox(hWnd,
						"For your protection, you cannot delete the root directory.",
						gsTitleCaption, MB_ICONINFORMATION);

					return TRUE;
				}

				char sText[2048];
				sprintf_s(sText, sizeof(sText),
					"Are you sure you want to permanently delete\r\n\"%s\"",
					sDeleteFile);

				if(MessageBox(hWnd, sText, gsTitleCaption, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2) != IDYES)
				{
					return TRUE;
				}
				else{
					DWORD dwSecureDeletePasses = gdwSecureDeletePasses;
					if(dwSecureDeletePasses <= 0)
					{
						dwSecureDeletePasses = 5;
					}

					if(SDeleteFile(sDeleteFile, dwSecureDeletePasses))
					{
						MessageBox(hWnd, "Complete!", gsTitleCaption, MB_ICONINFORMATION);
					}
					else{
						MessageBox(hWnd, "Could not delete the file!", gsTitleCaption, MB_ICONERROR);
					}
				}
			}

			return TRUE;
		}
		else if(wParam == ID_ACTION_TESTARCHIVE)
		{
			if(gbArchiveOpen)
			{
				gdwCurrentJobType = JOB_TYPE_TEST_ARCHIVE;
				CreateWorker(TestArchiveProc, (LPVOID) gsArchiveName);
				RunProgressDialog(hWnd);
			}
			else{
				NeedToOpenArchive(hWnd);
			}

			return TRUE;
		}
		else if(wParam == ID_HELP_CONTENTS)
		{
			OpenHelpFile();
			return TRUE;
		}
		else if(wParam == ID_ACTION_PROPERTIES || wParam == IDI_TB_INFO)
		{
			if(gbArchiveOpen)
			{
				if(ListView_GetSelectedCount(ghFileList) == 1)
				{
					DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_FILEINFO), hWnd, FileInfoDialog);
				}
			}
			else{
				if(wParam == ID_ACTION_PROPERTIES)
				{
					NeedToOpenArchive(hWnd);
				}
			}
			return TRUE;
		}
		else if(wParam == ID_ACTION_DELETE || wParam == IDI_TB_DELETE)
		{
			if(gbArchiveOpen)
			{
				int iSelectedCount = ListView_GetSelectedCount(ghFileList);
				if(iSelectedCount > 0)
				{
					if(iSelectedCount == ListView_GetItemCount(ghFileList))
					{
						SafeMsgBox(hWnd, "You cannot delete all items from an archive.",
							gsTitleCaption, MB_ICONINFORMATION|MB_OK|MB_APPLMODAL);
					}
					else{
						if(SafeMsgBox(hWnd,
							"Delete all selected files from the archive?",
							gsTitleCaption, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2) == IDYES)
						{
							gdwCurrentJobType = JOB_TYPE_DELETE_SELECTED;
							CreateWorker(DeleteArchiveFilesProc, (LPVOID) gsArchiveName);
							RunProgressDialog(hWnd);
						}
					}
				}
			}
			else{
				if(wParam == ID_ACTION_DELETE)
				{
					NeedToOpenArchive(hWnd);
				}
			}
			return TRUE;
		}
		else if(wParam == ID_FILE_FNEW || wParam == IDI_TB_NEW)
		{
			ResetDialog();			
			return TRUE;
		}
		else if(wParam == ID_FILE_FOPEN || wParam == IDI_TB_OPEN)
		{
			char *Filters = "Secure-Archives\0*" DEFAULTEXT "\0All-Files\0*.*\0\0";
			char InitialDirectory[MAX_PATH + 1];

			GetCurrentDirectory(MAX_PATH, InitialDirectory);

			if(OSFileDialog(hWnd, gsArchiveName, sizeof(gsArchiveName), DEFAULTEXT,
				"Select an archive to open",
				Filters, InitialDirectory, OFD, OFN_EXPLORER + OFN_LONGNAMES + OFN_PATHMUSTEXIST))
			{
				// View an existing archive
				gdwCurrentJobType = JOB_TYPE_READING;

				bool bIsUsingBlankPassword = false;
				if(!IsUsingBlankPassword(hWnd, gsArchiveName, &bIsUsingBlankPassword))
				{
					return TRUE;
				}

				if(bIsUsingBlankPassword)
				{
					SetPassword(&gPassword, gsEmptyPassword);
				}
				else{
					if(DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_OPENPASS), hWnd, OpenPassDialog) != 1)
					{
						return TRUE;
					}
				}

				ShowWindow(hWnd, SW_HIDE);
				SendMessage(hMainDialog, WM_REFRESH_ARCHIVE, 0, (LPARAM)gsArchiveName);
				ShowWindow(hWnd, SW_SHOW);
			}

			return TRUE;
		}
		else if(wParam == ID_ACTION_FIND || wParam == IDI_TB_FIND)
		{
			if(gbArchiveOpen)
			{
				if(hSearchDialog)
				{
					SetFocus(hSearchDialog);
				}
				else{
					DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_FINDFILE), NULL, SearchDialog);
				}
			}
			else{
				if(wParam == ID_ACTION_FIND)
				{
					NeedToOpenArchive(hWnd);
				}
			}
			return TRUE;
		}
		else if(wParam == ID_ACTION_EXTRACT || wParam == IDI_TB_EXTRACT)
		{
			if(gbArchiveOpen)
			{
				if(DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_EXTRACT), hWnd, ExtractDialog) == 1)
				{
					CreateWorker(ExtractEntireArchiveProc, (LPVOID) gsArchiveName);
					RunProgressDialog(hWnd);
				}
			}
			else{
				if(wParam == ID_ACTION_EXTRACT)
				{
					NeedToOpenArchive(hWnd);
				}
			}
			return TRUE;
		}
		else if(wParam == ID_FILE_EXIT || wParam == IDI_TB_EXIT)
		{
			ImageList_Destroy(hImageList);
			ImageList_Destroy(hImageListBig);
			EndDialog(hWnd, 0);
			if(hSearchDialog) EndDialog(hSearchDialog, 0);
			DestroyWindow(hWnd);
			return TRUE;
		}
		else if(wParam == ID_HELP_ABOUT)
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
		else if(wParam == IDI_TB_ADDFILES || wParam == ID_ACTION_ADD)
		{
			int iFileNamesSz = 1024 * 1024;
			char *sFileNames = (char *) calloc(sizeof(char), iFileNamesSz + 1);
			char sCurrentDirectory[MAX_PATH];

			GetLastPath(sCurrentDirectory, sizeof(sCurrentDirectory));

			OPENFILENAME OFN;

			OFN.lpstrCustomFilter = NULL;
			OFN.nMaxCustFilter    = 0;
			OFN.lpstrFileTitle    = NULL;
			OFN.nMaxFileTitle     = 0;
			OFN.lpstrFilter  = "All-Files\0*.*\0Text-Files\0*.txt\0\0";
			OFN.lpstrDefExt = "txt";
			OFN.lpstrTitle = "Select file(s) to Archive";
			OFN.lpstrInitialDir = sCurrentDirectory;
			OFN.lStructSize       = sizeof(OFN);
			OFN.hwndOwner         = hWnd;
			OFN.nFilterIndex      = 1;
			OFN.lpstrFile         = sFileNames;
			OFN.nMaxFile          = iFileNamesSz;
			OFN.Flags = OFN_EXPLORER | OFN_LONGNAMES | OFN_PATHMUSTEXIST | OFN_ALLOWMULTISELECT;

			if(GetOpenFileName(&OFN))
			{
				if(gbArchiveOpen)
				{
					strcpy_s(gsWorkingOutput, sizeof(gsWorkingOutput), gsArchiveName);
				}
				else {
					if(SafeMsgBox(hWnd, "Would you like to create a new archive?", gsTitleCaption, MB_YESNO|MB_ICONQUESTION) == IDNO)
					{
						return TRUE;
					}

					ResetEncryptionOptions();

					// Create a new archive from the directory's contents
					if(DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_NEWPASS), NULL, NewPassDialog) != 1)
					{
						return TRUE;
					}
				}

				char sRoot[MAX_PATH];
				char *sPath = sFileNames;
				char *sFileName = sFileNames + OFN.nFileOffset;

				FilePack fpPack;
				fpPack.Initialize(hWnd, &AltFilePackage);

				SetLastPath(sPath);

				if(IsDirectory(sPath))
				{
					char sFullPath[MAX_PATH];
					while(*sFileName)
					{
						_makepath_s(sFullPath, sizeof(sFullPath), NULL, sPath, sFileName, NULL);
						if(!fpPack.AddSimpleFile(sFullPath, sRoot, sizeof(sRoot), &AltFilePackage))
						{
							char sStatusText[5120];
							sprintf_s(sStatusText, sizeof(sStatusText),
								"An error occurred while trying to index the file:\r\n'%s'",
								sFileName);

							SafeMsgBox(hWnd, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

							fpPack.FreeMemory(&AltFilePackage);
							free(sFileNames);
							return TRUE;
						}

						sFileName += strlen(sFileName) + 1;
					}
				}
				else{
					if(!fpPack.AddSimpleFile(sPath, sRoot, sizeof(sRoot), &AltFilePackage))
					{
						char sStatusText[5120];
						sprintf_s(sStatusText, sizeof(sStatusText),
							"An error occurred while trying to index the file:\r\n'%s'",
							sFileName);

						SafeMsgBox(hWnd, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

						fpPack.FreeMemory(&AltFilePackage);
						free(sFileNames);
						return TRUE;
					}
				}

				if(AltFilePackage.dwFileCount > 0)
				{
					gdwCurrentJobType = JOB_TYPE_ADDING_FILES;
					CreateWorker(AddFilesProc, (LPVOID) sRoot);
					RunProgressDialog(hWnd);

					SendMessage(hMainDialog, WM_REFRESH_ARCHIVE, 0, (LPARAM)gsWorkingOutput);
				}

				free(sFileNames);
				fpPack.FreeMemory(&AltFilePackage);
			}
			return TRUE; 
		}

		return FALSE;
	}

	//---------------------------------------------------------------------------------------------------------------------

	else if(uMsg == WM_NOTIFY)
	{
		LPNMHDR lpnmhdr = (LPNMHDR)lParam;

		if (lpnmhdr->hwndFrom == ghFileList)
		{
			int iSortSubItem = 0;
			NMLISTVIEW  *pNMLV;
			int iIndex = 0;

			if(lpnmhdr->code == LVN_BEGINDRAG)
			{
				if(gbEnableDragDrop)
				{
					DragDropProc(hWnd, ghFileList);
				}
			}
			else if(lpnmhdr->code == NM_DBLCLK)
			{
				if(ListView_GetItemCount(ghFileList) > 0 && ListView_GetSelectedCount(ghFileList) == 1)
				{
					DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_FILEINFO), hWnd, FileInfoDialog);
				}
			}
			else if(lpnmhdr->code == LVN_COLUMNCLICK)
			{
				pNMLV = (NMLISTVIEW *)lParam;
				iSortSubItem = pNMLV->iSubItem;

				if(giSortCol == iSortSubItem)
				{
					if(giSortDirection == SORT_DIR_ASCENDING)
					{
						giSortDirection = SORT_DIR_DESCENDING;
					}
					else{
						giSortDirection = SORT_DIR_ASCENDING;
					}
				}
				else {
					giSortCol = iSortSubItem;
					giSortDirection = SORT_DIR_ASCENDING;
				}

				SortFileList(ghFileList, giSortCol, giSortDirection);
			}
		}
		return TRUE;
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

	//---------------------------------------------------------------------------------------------------------------------

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

	//---------------------------------------------------------------------------------------------------------------------

	else if(uMsg == WM_CLOSE)  // Received Close Message
	{
		ImageList_Destroy(hImageList);
		ImageList_Destroy(hImageListBig);
		EndDialog(hWnd,0);
		if(hSearchDialog) EndDialog(hSearchDialog, 0);
		DestroyWindow(hWnd);
		return TRUE;
	}

	//---------------------------------------------------------------------------------------------------------------------

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

