#ifndef _WORKERS_CPP
#define _WORKERS_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <ShlObj.H>
#include <Stdio.H>
#include <Stdlib.H>

#include "IndexDir.H"
#include "Package.H"
#include "Entry.H"
#include "Routines.H"
#include "Workers.H"
#include "PackHeader.H"
#include "ListView.H"

#include "../Dialogs/MainDlg.H"
#include "../Dialogs/OpenPassDlg.H"
#include "../Dialogs/ReadingDlg.H"
#include "../Dialogs/ProgressDialog.H"
#include "../Dialogs/ExtractDlg.H"
#include "../Dialogs/ReplaceFileDlg.H"

#include "../../Compression/zLib/ZLibEncapsulation.H"

#include "../../Rijndael/AES.H"
#include "../../Rijndael/AESOpt.H"
#include "../../Rijndael/Rijndael.H"

#include "../../BlowFish/BlowFish.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 
using namespace NSWFL::File; 

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HANDLE hWorkerProc = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CompleteStatusBars(void)
{
	Set_Text(hPackingActionText, "");
	Set_Text(hPackingStatusText, "");

	SendMessage(hProgressStatusBar, PBM_SETRANGE32, 0, (LPARAM) 100);
	SendMessage(hProgressFileStatusBar, PBM_SETRANGE32, 0, (LPARAM) 100);

	fPackageProgress = 100;
	fFileProgress = 100;

	SendMessage(hProgressStatusBar, PBM_SETPOS, 0, (LPARAM) 100);
	SendMessage(hProgressFileStatusBar, PBM_SETPOS, 0, (LPARAM) 100);

	Sleep(100);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CreateWorker(LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter)
{
	if(hWorkerProc)
	{
		CloseHandle(hWorkerProc);
	}

	hWorkerProc = CreateThread(NULL, 0,
		lpStartAddress, (LPVOID) lpParameter, CREATE_SUSPENDED, NULL);

	if(hWorkerProc)
	{
		SetThreadPriorityBoost(hWorkerProc, !gbUseThreadBoost);
		SetThreadPriority(hWorkerProc, THREAD_PRIORITY_NORMAL); 
	}

	return(hWorkerProc != NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
DWORD WINAPI CreateSelfExtractorProc(LPVOID lpVoid)
{
	char *sSourceFile = (char *) lpVoid;

	if(!gbSilent)
	{
		WaitOnWindow(hProgressDialog);
		Set_Text(hPackingActionText, "Starting process...");
	    Sleep(100);
	}

	if(CreateSelfExtractor(sSourceFile, gsWorkingOutput))
    {
        if(gdwCurrentJobType == JOB_TYPE_CANCEL)
        {
			if(!gbSilent)
			{
	            Set_Text(hPackingActionText, "Cancelled.");
				Set_Text(hPackingStatusText, "");
				Sleep(1000);
			}
            SDeleteFile(gsWorkingOutput);
        }
        else{
			gdwCurrentJobType = JOB_TYPE_SUCCESS;

			if(!gbSilent)
			{
	            Set_Text(hPackingActionText, "Done.");
				Set_Text(hPackingStatusText, "");
				Sleep(100);
			}
        }

        fPackageProgress = 0;
        EndProgressDialog(1);
    }
    else{
        fPackageProgress = 0;

		if(!gbSilent)
		{
			if(gdwCurrentJobType == JOB_TYPE_CANCEL)
			{
				Set_Text(hPackingActionText, "Cancelled.");
				Set_Text(hPackingStatusText, "");
			}

			Sleep(1000);
			EndProgressDialog(1);
		}
		DeleteFile(gsWorkingOutput);
    }

	return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI ExtractEntireArchiveProc(LPVOID lpVoid)
{
	char *sSourceFile = (char *) lpVoid;

	if(!gbSilent)
	{
		WaitOnWindow(hProgressDialog);
		Set_Text(hPackingActionText, "Starting process...");
	    Sleep(100);
	}

	FixDirTarget(gsExtractLocation, sizeof(gsExtractLocation));

	int iResult = BreakPackage(sSourceFile, gsExtractLocation, &gPassword, false);

	CompleteStatusBars();

	if(iResult == PACK_RESULT_OK)
	{
		gdwCurrentJobType = JOB_TYPE_SUCCESS;
        Set_Text(hPackingStatusText, "Done.");
	}
	else if(iResult == PACK_RESULT_CANCEL)
	{
		gdwCurrentJobType = JOB_TYPE_CANCEL;
        Set_Text(hPackingStatusText, "Cancelled.");
	}
	else if(iResult == PACK_RESULT_ERROR)
	{
		gdwCurrentJobType = JOB_TYPE_CANCEL;
        Set_Text(hPackingStatusText, "Cancelled.");
	}
	else if(iResult == PACK_RESULT_NOACTION)
	{
		gdwCurrentJobType = JOB_TYPE_CANCEL;
        Set_Text(hPackingStatusText, "Cancelled.");
	}

	if(!gbSilent)
	{
        Sleep(1000);
		EndProgressDialog(iResult == PACK_RESULT_OK);
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
DWORD WINAPI TestArchiveProc(LPVOID lpVoid)
{
	char *sSourceFile = (char *) lpVoid;

	if(!gbSilent)
	{
		WaitOnWindow(hProgressDialog);
		Set_Text(hPackingActionText, "Starting process...");
	    Sleep(100);
	}

	int iResult = BreakPackage(sSourceFile, NULL, &gPassword, true);

	CompleteStatusBars();

	if(iResult == PACK_RESULT_OK)
	{
		gdwCurrentJobType = JOB_TYPE_SUCCESS;
        Set_Text(hPackingStatusText, "Done.");
	}
	else if(iResult == PACK_RESULT_CANCEL)
	{
		gdwCurrentJobType = JOB_TYPE_CANCEL;
        Set_Text(hPackingStatusText, "Cancelled.");
	}
	else if(iResult == PACK_RESULT_ERROR)
	{
		gdwCurrentJobType = JOB_TYPE_CANCEL;
        Set_Text(hPackingStatusText, "Cancelled.");
	}
	else if(iResult == PACK_RESULT_NOACTION)
	{
		gdwCurrentJobType = JOB_TYPE_CANCEL;
        Set_Text(hPackingStatusText, "Cancelled.");
	}

	if(!gbSilent)
	{
        Sleep(1000);
		EndProgressDialog(iResult == PACK_RESULT_OK);
	}

	return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
DWORD WINAPI ViewExistingProc(LPVOID lpVoid)
{
	char *sSourceFile = (char *) lpVoid;

	WaitOnWindow(hReadingDialog);
    Set_Text(hReadingStatusText, "Please wait... Reading encrypted files.");
	Sleep(100);

	gbArchiveOpen = false;
    SendMessage(hMainDialog, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)gsTitleCaption);

	SendMessage(ghFileList, (UINT)WM_SETREDRAW , (WPARAM)FALSE, (LPARAM)0);
	int iResult = PopGrid(sSourceFile, &gPassword);
	SendMessage(ghFileList, (UINT)WM_SETREDRAW , (WPARAM)TRUE, (LPARAM)0);

	fPackageProgress = 0;

    if(iResult == PACK_RESULT_OK)
    {
		gdwCurrentJobType = JOB_TYPE_SUCCESS;
		Set_Text(hReadingStatusText, "Done.");
		
		char sFileName[MAX_PATH + 100];

		_splitpath_s(sSourceFile, NULL, 0, NULL, 0, sFileName, sizeof(sFileName), NULL, 0);

		char sCaption[1024];
		GetAppTitle(sCaption, sizeof(sCaption), sFileName);
		SendMessage(hMainDialog, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)sCaption);
		
		gbArchiveOpen = true;
    }
    else{
		gdwCurrentJobType = JOB_TYPE_CANCEL;
        Set_Text(hReadingStatusText, "Cancelled.");

		ResetDialog();
    }

	Sleep(250);
	EndReadingDialog(iResult == PACK_RESULT_OK);

	return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
DWORD WINAPI CreateFolderArchiveProc(LPVOID lpVoid)
{
	char *sSourceItem = (char *) lpVoid;

	if(!gbSilent)
	{
		WaitOnWindow(hProgressDialog);
	    Set_Text(hPackingStatusText, "Please wait...");
		Sleep(100);
	}

    MKPKGPARAM MyMkPkgParam;
	memset(&MyMkPkgParam, 0, sizeof(MyMkPkgParam));
	MyMkPkgParam.sSrcFolder  = sSourceItem;
	MyMkPkgParam.sTarName    = gsWorkingOutput;
	//MyMkPkgParam.sRawKey     = gsPassword;
	//MyMkPkgParam.iRawKeyLen  = (int)strlen(gsPassword);
	MyMkPkgParam.iPkgAttribs = giPkgAttribs;
	MyMkPkgParam.iOptions    = giPkgOptions;

	int iResult = MakePackage(&MyMkPkgParam, &gPassword);

	CompleteStatusBars();

	if(iResult == PACK_RESULT_OK)
	{
		gdwCurrentJobType = JOB_TYPE_SUCCESS;
        Set_Text(hPackingStatusText, "Done.");

		strcpy_s(gsArchiveName, sizeof(gsArchiveName), gsWorkingOutput);
	}
	else if(iResult == PACK_RESULT_CANCEL)
	{
		gdwCurrentJobType = JOB_TYPE_CANCEL;
        Set_Text(hPackingStatusText, "Cancelled.");

		if(!(MyMkPkgParam.iOptions & OPTION_ADDFILES))
		{
			SDeleteFile(gsWorkingOutput);
		}
	}
	else if(iResult == PACK_RESULT_ERROR)
	{
		gdwCurrentJobType = JOB_TYPE_CANCEL;
        Set_Text(hPackingStatusText, "Cancelled.");

		if(!(MyMkPkgParam.iOptions & OPTION_ADDFILES))
		{
			SDeleteFile(gsWorkingOutput);
		}
	}
	else if(iResult == PACK_RESULT_NOACTION)
	{
		gdwCurrentJobType = JOB_TYPE_CANCEL;
        Set_Text(hPackingStatusText, "Cancelled.");
	}

	if(!gbSilent)
	{
        Sleep(1000);
		EndProgressDialog(iResult == PACK_RESULT_OK);
	}

	return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
DWORD WINAPI CreateFileArchiveProc(LPVOID lpVoid)
{
	char *sSourceFile = (char *) lpVoid;

	if(!gbSilent)
	{
		WaitOnWindow(hProgressDialog);
	    Set_Text(hPackingStatusText, "Please wait...");
		Sleep(100);
	}

	MKPKGPARAM MyMkPkgParam;
	memset(&MyMkPkgParam, 0, sizeof(MyMkPkgParam));
	MyMkPkgParam.sSrcFolder  = sSourceFile;
	MyMkPkgParam.sTarName    = gsWorkingOutput;
	//MyMkPkgParam.sRawKey     = gsPassword;
	//MyMkPkgParam.iRawKeyLen  = (int)strlen(gsPassword);
	MyMkPkgParam.iPkgAttribs = giPkgAttribs;
	MyMkPkgParam.iOptions    = giPkgOptions|OPTION_USEALTFILEPACK;

	int iResult = MakePackage(&MyMkPkgParam, &gPassword);

	CompleteStatusBars();

	if(iResult == PACK_RESULT_OK)
    {
		gdwCurrentJobType = JOB_TYPE_SUCCESS;
		Set_Text(hPackingStatusText, "Done.");

        strcpy_s(gsArchiveName, sizeof(gsArchiveName), gsWorkingOutput);
	}
	else if(iResult == PACK_RESULT_CANCEL)
    {
		gdwCurrentJobType = JOB_TYPE_CANCEL;
		Set_Text(hPackingStatusText, "Cancelled.");

		if(!(MyMkPkgParam.iOptions & OPTION_ADDFILES))
		{
			SDeleteFile(gsWorkingOutput);
		}
	}
	else if(iResult == PACK_RESULT_ERROR)
    {
		gdwCurrentJobType = JOB_TYPE_CANCEL;
        Set_Text(hPackingStatusText, "An error has occured.");

		if(!(MyMkPkgParam.iOptions & OPTION_ADDFILES))
		{
			SDeleteFile(gsWorkingOutput);
		}
    }
	else if(iResult == PACK_RESULT_NOACTION)
	{
		gdwCurrentJobType = JOB_TYPE_CANCEL;
        Set_Text(hPackingStatusText, "Cancelled.");
	}

    Sleep(1000);
    EndProgressDialog(iResult == PACK_RESULT_OK);

	return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
DWORD WINAPI AddFilesProc(LPVOID lpVoid)
{
	char *sRoot = (char *) lpVoid;

	WaitOnWindow(hProgressDialog);
    Set_Text(hPackingStatusText, "Please wait...");
    Sleep(100);

    MKPKGPARAM MyMkPkgParam;
	memset(&MyMkPkgParam, 0, sizeof(MyMkPkgParam));
	MyMkPkgParam.sSrcFolder  = sRoot;
	MyMkPkgParam.sTarName    = gsWorkingOutput;
	//MyMkPkgParam.sRawKey     = gsPassword;
	//MyMkPkgParam.iRawKeyLen  = (int)strlen(gsPassword);
	MyMkPkgParam.iPkgAttribs = giPkgAttribs;
	MyMkPkgParam.iOptions    = giPkgOptions | OPTION_USEALTFILEPACK | OPTION_ADDFILES;

	int iResult = MakePackage(&MyMkPkgParam, &gPassword);

	CompleteStatusBars();

	if(iResult == PACK_RESULT_OK)
    {
		gdwCurrentJobType = JOB_TYPE_SUCCESS;
		Set_Text(hPackingStatusText, "Done.");

		strcpy_s(gsArchiveName, sizeof(gsArchiveName), gsWorkingOutput);
	}
	else if(iResult == PACK_RESULT_CANCEL)
    {
		gdwCurrentJobType = JOB_TYPE_CANCEL;
		Set_Text(hPackingStatusText, "Cancelled.");

		if(!(MyMkPkgParam.iOptions & OPTION_ADDFILES))
		{
			SDeleteFile(gsWorkingOutput);
		}
	}
	else if(iResult == PACK_RESULT_ERROR)
    {
		gdwCurrentJobType = JOB_TYPE_CANCEL;
        Set_Text(hPackingStatusText, "An error has occured.");

		if(!(MyMkPkgParam.iOptions & OPTION_ADDFILES))
		{
			SDeleteFile(gsWorkingOutput);
		}
    }
	else if(iResult == PACK_RESULT_NOACTION)
	{
		gdwCurrentJobType = JOB_TYPE_CANCEL;
        Set_Text(hPackingStatusText, "Cancelled.");
	}

    Sleep(1000);
    EndProgressDialog(iResult == PACK_RESULT_OK);

	return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
DWORD WINAPI DeleteArchiveFilesProc(LPVOID lpVoid)
{
	char *sSourceFile = (char *) lpVoid;

	if(!gbSilent)
	{
		WaitOnWindow(hProgressDialog);
		Set_Text(hPackingActionText, "Starting process...");
	    Sleep(100);
	}

	FixDirTarget(gsExtractLocation, sizeof(gsExtractLocation));

	int iSelectedCount = ListView_GetSelectedCount(ghFileList);
	int iItem = 0;
	char sTempBuf[64];

	GRIDDELETEITEMS GDI;
	memset(&GDI, 0, sizeof(GDI));

	GDI.Items = (GRIDDELETEITEM *) calloc(sizeof(GRIDDELETEITEM), iSelectedCount);
	GDI.hGrid = ghFileList;

	for(int iIndex = -1; (iIndex = ListView_GetNextItem(ghFileList, iIndex, LVNI_SELECTED)) != -1;)
    {
        ListView_GetItemText(ghFileList, iIndex, LIST_POS_POSITION, sTempBuf, sizeof(sTempBuf));

		GDI.Items[GDI.ulCount].i64FilePosition = _atoi64(sTempBuf);
		GDI.Items[GDI.ulCount].ulGridPosition = iIndex;

		GDI.ulCount++;
    }

	char sTempPath[MAX_PATH];
	char sDrive[MAX_PATH];
	char sDir[MAX_PATH];
	char sTempFile[MAX_PATH];
	char sFile[MAX_PATH];
	char sExt[MAX_PATH];

	_splitpath_s(sSourceFile,
		sDrive, sizeof(sDrive),
		sDir, sizeof(sDir),
		sFile, sizeof(sFile),
		sExt, sizeof(sExt));

	sprintf_s(sTempFile, sizeof(sTempFile), "DP-%d-%d-%s", rand(), GetTickCount(), sFile);
	_makepath_s(sTempPath, sizeof(sTempPath), sDrive, sDir, sTempFile, sExt);

	int iResult = DeleteFilesFromPackage(sSourceFile, sTempPath, &gPassword, &GDI, iSelectedCount);

	free(GDI.Items);

	CompleteStatusBars();

	if(iResult == PACK_RESULT_OK)
    {
		gdwCurrentJobType = JOB_TYPE_SUCCESS;
		Set_Text(hPackingActionText, "Done.");

		if(gdwCurrentJobType = JOB_TYPE_SUCCESS && !gbSilent)
		{
			EndProgressDialog(1);
			SendMessage(hMainDialog, WM_REFRESH_ARCHIVE, 0, (LPARAM)gsArchiveName);
			return 0;
		}
	}
	else if(iResult == PACK_RESULT_CANCEL)
    {
		gdwCurrentJobType = JOB_TYPE_CANCEL;
		Set_Text(hPackingStatusText, "Cancelled.");

		SetFileAttributes(sTempPath, FILE_ATTRIBUTE_NORMAL);
		if(!DeleteFile(sTempPath))
		{
			SafeMsgBox(hProgressDialog, "Failed to delete the temp archive.",
				gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);		
		}
	}
	else if(iResult == PACK_RESULT_ERROR)
    {
		gdwCurrentJobType = JOB_TYPE_CANCEL;
        Set_Text(hPackingStatusText, "An error has occured.");

		SetFileAttributes(sTempPath, FILE_ATTRIBUTE_NORMAL);
		if(!DeleteFile(sTempPath))
		{
			SafeMsgBox(hProgressDialog, "Failed to delete the temp archive.",
				gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);		
		}
    }
	else if(iResult == PACK_RESULT_NOACTION)
	{
		gdwCurrentJobType = JOB_TYPE_CANCEL;
        Set_Text(hPackingStatusText, "Cancelled.");
	}

	EndProgressDialog(iResult == PACK_RESULT_OK);

	return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
DWORD WINAPI RepairCorruptArchiveProc(LPVOID lpVoid)
{
	char *sSourceFile = (char *) lpVoid;
	char sTargetFile[MAX_PATH];

	WaitOnWindow(hProgressDialog);
	Set_Text(hPackingActionText, "Starting process...");
    Sleep(100);

	bool bIsUsingBlankPassword = false;
	if(!IsUsingBlankPassword(hProgressDialog, sSourceFile, &bIsUsingBlankPassword))
	{
		Sleep(1000);
		return 0;
	}

	if(bIsUsingBlankPassword)
	{
		SetPassword(&gPassword, gsEmptyPassword);
	}
	else{
		if(DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_OPENPASS), hProgressDialog, OpenPassDialog) != 1)
		{
			Sleep(1000);
			EndProgressDialog(1);
			return 0;
		}
	}

	char sCurrentDirectory[MAX_PATH];
	if(!Get_DesktopDirectory(sCurrentDirectory, sizeof(sCurrentDirectory)))
	{
		strcpy_s(sCurrentDirectory, sizeof(sCurrentDirectory), "C:\\");
	}

	OPENFILENAME OFN;
	memset(sTargetFile, 0, sizeof(sTargetFile));

	OFN.lpstrCustomFilter = NULL;
	OFN.nMaxCustFilter    = 0;
	OFN.lpstrFileTitle    = NULL;
	OFN.nMaxFileTitle     = 0;
	OFN.lpstrFilter		  = "Archives\0*.saef\0All-Files\0*.*\0\0";
	OFN.lpstrDefExt		  = "saef";
	OFN.lpstrTitle		  = "Save repaired archive to...";
	OFN.lpstrInitialDir	  = sCurrentDirectory;
	OFN.lStructSize       = sizeof(OFN);
	OFN.hwndOwner         = hProgressDialog;
	OFN.nFilterIndex      = 1;
	OFN.lpstrFile         = sTargetFile;
	OFN.nMaxFile          = sizeof(sTargetFile);
	OFN.Flags = OFN_EXPLORER | OFN_LONGNAMES | OFN_PATHMUSTEXIST;

	if(!GetSaveFileName(&OFN))
	{
		Sleep(1000);
		EndProgressDialog(1);
		return 0;
	}

	if(RepairCorruptArchive(sSourceFile, sTargetFile, &gPassword))
	{
		if(gdwCurrentJobType == JOB_TYPE_CANCEL)
		{
			if(!gbSilent)
			{
				Set_Text(hPackingActionText, "Cancelled.");
				Set_Text(hPackingStatusText, "");
				Sleep(1000);
			}
			SDeleteFile(gsWorkingOutput);
		}
		else{
			gdwCurrentJobType = JOB_TYPE_SUCCESS;

			if(!gbSilent)
			{
				Set_Text(hPackingActionText, "Done.");
				Set_Text(hPackingStatusText, "");
				Sleep(100);
			}
		}

		fPackageProgress = 0;
		EndProgressDialog(1);
	}
	else{
		fPackageProgress = 0;

		if(!gbSilent)
		{
			if(gdwCurrentJobType == JOB_TYPE_CANCEL)
			{
				Set_Text(hPackingActionText, "Cancelled.");
				Set_Text(hPackingStatusText, "");
			}

			Sleep(1000);
			EndProgressDialog(1);
		}
		SDeleteFile(gsWorkingOutput);
	}

	return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
