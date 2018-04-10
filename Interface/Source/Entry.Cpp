#ifndef _ENTRY_CPP
#define _ENTRY_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <ShlObj.H>
#include <Stdio.H>
#include <Stdlib.H>

#include "Entry.H"
#include "Routines.H"
#include "Package.H"
#include "Workers.H"
#include "Init.H"

#ifndef _SELFEXTRACTOR_APP
#include "../Dialogs/ExtractDlg.H"
#include "../Dialogs/MainDlg.H"
#include "../Dialogs/ReadingDlg.H"
#include "../Dialogs/NewPassDlg.H"
#include "../Dialogs/HardKeyDlg.H"
#include "../Dialogs/SecureDeleteDialog.H"
#endif

#include "../Dialogs/OpenPassDlg.H"
#include "../Dialogs/ProgressDialog.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::String;
using namespace NSWFL::File; 
using namespace NSWFL::Windows;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HINSTANCE ghAppInstance = NULL;

char gsAppCopyRight[64];
char gsFileVersion[32];
char gsTitleCaption[64];
char gsRegistryKey[1024];
char gsPathKey[1024];
char gsCompatibility[100];
char gsEmptyPassword[100];
char gsWorkingOutput[MAX_PATH + 1];
char gsArchiveName[MAX_PATH + 1];
char gsExtractLocation[MAX_PATH + 1];
char gsLastPath[MAX_PATH];
char gsSecureDeletePopulate[MAX_PATH];

char gsDfltCompressLevel[255];
char gsPath[MAX_PATH];
char gsTempFilePath[MAX_PATH]; //Default temp path, leave empty to use default.
char gsDfltEncryptAlgo[255];
bool gbDfltChkTimeStamps = true;
bool gbDfltChkAttributes = true;
bool gbDfltChkPathAttributes = true;
bool gbDfltChkCRCs = true;
bool gbIncludeHiddenFiles = true;
bool gbDeleteSourceFiles = false;
bool gbDfltChkSavePaths = true;
bool gbShowGridLines = true;
bool gbShowGridIcons = true;
bool gbEnableFullRowSelect = true;
bool gbEnableDragDrop = true;
bool gbAlwaysOnTop = false;
bool gbKeepSelectionVisible = true;
bool gbUsePreAllocation = false;
bool gbIgnoreFreeSpaceWarning = false;
bool gbDragDropCopyInsteadOfMove = false;
bool gbAlwaysRunInBackground = false;
bool gbWarnAboutNoEncryption = true;
bool gbAutoClose = false; //Close when operation is complete?
bool gbArchiveOpen = false;
bool gbSilent = false; //Work in silent mode?
bool gbOverwrite = false; //Overwrite by default?

PASSWORD gPassword;
CNASCCL gCode;
CONIO gConIO;
CTrayIcon gTrayIcon;

DWORD gdwCurrentJobType = 0;
DWORD gdwSecureDeletePasses = 1;
DWORD gdwWeakEncryptionWarnLevel = 0;

bool gbUseProcessBoost = true;
bool gbUseThreadBoost = true;
DWORD_PTR gdwCPUAffinityMask = 0;

int giPkgOptions = 0;
int giPkgAttribs = 0;

int giCompressLevel = 0;
int giCryptAlgo = 0;
DWORD gdwDefaultSortColumn = -1;
DWORD gdwDefaultSortDirection = -1;

CCRC32 *gCRC32;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int ParseCmdLine(CCmdLine *lpCmd);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevious, LPSTR sCmdLine, int iCmdShow)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	ghAppInstance = hInstance;

	if(!InitializeApp())
    {
        return 1;
    }

#ifndef _SELFEXTRACTOR_APP

	CCmdLine *pCmdLine = new CCmdLine;

	ParseCmdLine(pCmdLine);

	//Wait on worker thread to exit.
	DWORD ExitCode = 0;
	while(GetExitCodeThread(hWorkerProc, &ExitCode))
	{
		if(ExitCode != STILL_ACTIVE)
		{
			break;
		}
		else{
			Sleep(100);
		}
	}

	delete pCmdLine;

#else
	/*
	if((int)strlen(sCmdLine) > 0)
	{
		strcpy_s(gsArchiveName, sizeof(gsArchiveName), sCmdLine);
	}
	else{
	*/
		GetModuleFileName(NULL, gsArchiveName, sizeof(gsArchiveName));
	/*
	}
	*/

	__int64 i64FileSize = 0;

	if(!FileSize(gsArchiveName, &i64FileSize))
	{
		SafeMsgBox(NULL, "Failed to read the size of the self extractor.",
			gsTitleCaption, MB_ICONEXCLAMATION);
		return 0;
	}

	if(i64FileSize == SELFEXTRACTOREXEEND)
	{
		SafeMsgBox(NULL, "The self extractor is empty.",
			gsTitleCaption, MB_ICONEXCLAMATION);
		return 0;
	}

	bool bIsUsingBlankPassword = false;
	if(!IsUsingBlankPassword(NULL, gsArchiveName, &bIsUsingBlankPassword))
	{
        return 0;
	}

	// Decrypt the entire archive.
	gdwCurrentJobType = JOB_TYPE_EXTRACT_ALL;
	if(bIsUsingBlankPassword)
	{
		SetPassword(&gPassword, gsEmptyPassword);
	}
	else{
		if(DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_OPENPASS), NULL, OpenPassDialog) != 1)
		{
			return 0;
		}
	}

	if(BrowseDirectory(NULL, "Select the directory where you would like to save the decrypted files.",
		BIF_RETURNONLYFSDIRS|BIF_USENEWUI, gsExtractLocation, sizeof(gsExtractLocation)))
	{
		CreateWorker(ExtractEntireArchiveProc, (LPVOID) gsArchiveName);

		RunProgressDialog(NULL);
		ResumeThread(hWorkerProc);
	}
#endif

	if(!UninitializeApp())
    {
        return 1;
    }

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
int IsCommand(const char *sCmdLine, const char *sArg)
{
	int iArgSz = (int)strlen(sArg);
	if(_strnicmp(sCmdLine, sArg, iArgSz) == 0)
	{
		return iArgSz;
	}
	else{
		return 0;
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
int ParseCmdLine(CCmdLine *lpCmd)
{
	int iArg = 0;
	int iArgSz = 0;
	char *sArg = NULL;
	char *sMsg = NULL;

	char sSource[MAX_PATH];
	char sTemp[255];

	bool bEncrypt = false;
	bool bDecrypt = false;
	bool bCreateSelfExtractor = false;
	bool bAuto = false;
	bool bView = false;
	bool bSourceSpecified = false;
	bool bTargetSpecified = false;

	memset(sSource, 0, sizeof(sSource));

	if(lpCmd->Count() == 1)
	{
	    // Create new archive
		DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDialog);
		return 0;
	}
	else if(lpCmd->Count() == 2)
	{
		if((iArgSz = IsCommand(lpCmd->Item(1), "/Repair:")) > 0)
		{
			char sBrokenFile[MAX_PATH];

			gdwCurrentJobType = JOB_TYPE_REPAIR_ARCHIVE;

			GetFlagedValue(lpCmd->Item(1), iArgSz, lpCmd->Length(1), sBrokenFile, sizeof(sBrokenFile));
			CreateWorker(RepairCorruptArchiveProc, (LPVOID) sBrokenFile);
			RunProgressDialog(NULL);

			return 0;
		}
		else if((iArgSz = IsCommand(lpCmd->Item(1), "/Test:")) > 0)
		{
			GetFlagedValue(lpCmd->Item(1), iArgSz, lpCmd->Length(1), gsArchiveName, sizeof(gsArchiveName));

			bool bIsUsingBlankPassword = false;
			if(!IsUsingBlankPassword(hProgressDialog, gsArchiveName, &bIsUsingBlankPassword))
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

			gdwCurrentJobType = JOB_TYPE_TEST_ARCHIVE;
			CreateWorker(TestArchiveProc, (LPVOID) gsArchiveName);
			RunProgressDialog(NULL);
			return 0;
		}
		else if(_strcmpi(lpCmd->Item(1), "/SecureDelete") == 0)
		{
			DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_SECUREDELETE), NULL, SecureDeleteDialog);
			return 0;
		}
		else if(_strnicmp(lpCmd->Item(1), "/SecureDelete:", 14) == 0)
		{
			strcpy_s(gsSecureDeletePopulate, sizeof(gsSecureDeletePopulate), lpCmd->Item(1) + 14);
			DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_SECUREDELETE), NULL, SecureDeleteDialog);
			return 0;
		}
		else if(_strcmpi(lpCmd->Item(1), "/Configure") == 0)
		{
			LaunchConfigDlg(NULL);
			return 0;
		}
		else if(_strcmpi(lpCmd->Item(1), "/HardKeyGen") == 0)
		{
			DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_HARDKEY), NULL, HardKeyDlg);
			return 0;
		}
		else if(_strcmpi(lpCmd->Item(1), "/KeyGenFile") == 0)
		{
			int iResult = SafeMsgBox(NULL, "This is a \"Secure Archive Hard Key File\", "
				"it is used to encrypt/decrypt data.\r\n"
				"Would you like to view documentation explaining how to use it?",
				gsTitleCaption, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON1);

			if(iResult == IDYES)
			{
				OpenHelpFile("/HardKey/Overview.Html");
			}
			return 0;
		}
	}

	for(int iArg = 1; iArg < lpCmd->Count(); iArg++)
	{
		if((iArgSz = IsCommand(lpCmd->Item(iArg), "/Delete:")) > 0)
		{
			char sDeleteMe[MAX_PATH];

			GetFlagedValue(lpCmd->Item(iArg), iArgSz, lpCmd->Length(iArg), sDeleteMe, sizeof(sDeleteMe));

			if(SDeleteFile(sDeleteMe))
			{
				return 0;
			}
			else{
				return 1;
			}
		}
		/*
		else if((iArgSz = IsCommand(lpCmd->Item(iArg), "/Console")) > 0)
		{
			//AttachConsole does not work on windows 2000.
			if(AttachConsole(ATTACH_PARENT_PROCESS))
			{
				//SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);

				//char sConTitle[255];
				//sprintf_s(sConTitle, sizeof(sConTitle), "%s [Version: %s]", gsTitleCaption, gsFileVersion);
				//SetConsoleTitle(sConTitle);

				gConIO.StdIn  = GetStdHandle(STD_INPUT_HANDLE);
				gConIO.StdOut = GetStdHandle(STD_OUTPUT_HANDLE);
				gConIO.StdErr = GetStdHandle(STD_ERROR_HANDLE);
			}
		}
		*/
		else if((iArgSz = IsCommand(lpCmd->Item(iArg), "/CompressLevel:")) > 0)
		{
			GetFlagedValue(lpCmd->Item(iArg), iArgSz, lpCmd->Length(iArg), sTemp, sizeof(sTemp));
			giCompressLevel = atol(sTemp); //(0 - 9 (Off - Max))
		}
		else if((iArgSz = IsCommand(lpCmd->Item(iArg), "/Passes:")) > 0)
		{
			GetFlagedValue(lpCmd->Item(iArg), iArgSz, lpCmd->Length(iArg), sTemp, sizeof(sTemp));
			gdwSecureDeletePasses = atol(sTemp);
		}
		else if((iArgSz = IsCommand(lpCmd->Item(iArg), "/Overwrite")) > 0)
		{
			gbOverwrite = true;
		}
		else if((iArgSz = IsCommand(lpCmd->Item(iArg), "/NoFolders")) > 0)
		{
			gbUseFolderNames = false;
		}
		else if((iArgSz = IsCommand(lpCmd->Item(iArg), "/GO")) > 0)
		{
			bAuto = true;
			gbAutoClose = true;
		}
		else if((iArgSz = IsCommand(lpCmd->Item(iArg), "/New")) > 0)
		{
	        // Create new archive
			DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDialog);
			return 0;
		}
		else if((iArgSz = IsCommand(lpCmd->Item(iArg), "/Source:")) > 0)
		{
			if(DoesPathContainUnicode(lpCmd->Item(iArg), lpCmd->Length(iArg)))
			{
				SafeMsgBox(NULL,
					"The name of the file you are trying to open contains\r\n"
					"UNICODE characters, which are not supported.",
					gsTitleCaption, MB_ICONASTERISK);
				return 1000;
			}

			GetFlagedValue(lpCmd->Item(iArg), iArgSz, lpCmd->Length(iArg), sSource, sizeof(sSource));
			strcpy_s(gsArchiveName, sizeof(gsArchiveName), sSource);
			bSourceSpecified = true;
		}
		else if((iArgSz = IsCommand(lpCmd->Item(iArg), "/Target:")) > 0)
		{
			if(DoesPathContainUnicode(lpCmd->Item(iArg), lpCmd->Length(iArg)))
			{
				SafeMsgBox(NULL,
					"The name of the file you are trying to write contains\r\n"
					"UNICODE characters, which are not supported.",
					gsTitleCaption, MB_ICONASTERISK);
				return 1000;
			}

			GetFlagedValue(lpCmd->Item(iArg), iArgSz, lpCmd->Length(iArg), gsWorkingOutput, sizeof(gsWorkingOutput));
			strcpy_s(gsExtractLocation, sizeof(gsExtractLocation), gsWorkingOutput);
			bTargetSpecified = true;
		}
		else if((iArgSz = IsCommand(lpCmd->Item(iArg), "/CreateSelfExtractor")) > 0)
		{
			bCreateSelfExtractor = true;
		}
		else if((iArgSz = IsCommand(lpCmd->Item(iArg), "/Encrypt")) > 0)
		{
			if(bDecrypt)
			{
				SafeMsgBox(NULL, "You cannot specify both /Encrypt and /Decrypt.", 
					gsTitleCaption, MB_ICONINFORMATION);
				return 0;
			}
			else bEncrypt = true;
		}
		else if((iArgSz = IsCommand(lpCmd->Item(iArg), "/Decrypt")) > 0)
		{
			if(bEncrypt)
			{
				SafeMsgBox(NULL, "You cannot specify both /Encrypt and /Decrypt.", 
					gsTitleCaption, MB_ICONINFORMATION);
				return 0;
			}
			else bDecrypt = true;
		}
		else if((iArgSz = IsCommand(lpCmd->Item(iArg), "/View")) > 0)
		{
			bView = true;
			bAuto = false;
			gbAutoClose = false;
		}
		else if((iArgSz = IsCommand(lpCmd->Item(iArg), "/Silent")) > 0)
		{
			gbSilent = true;
			bAuto = true;
			gbAutoClose = true;
		}
		else if((iArgSz = IsCommand(lpCmd->Item(iArg), "/Password:")) > 0)
		{
			char *sPwd = NULL;

			int iPwdLength = GetFlagedValue(lpCmd->Item(iArg), iArgSz, lpCmd->Length(iArg), NULL, 0);

			iPwdLength *= -1;

			if((sPwd = (char *) calloc(sizeof(char), iPwdLength + 1)) == NULL)
			{
				SafeMsgBox(GetActiveWindow(),
					"Failed to allocate memory for your password. Out of memory?",
					gsTitleCaption, MB_ICONERROR);
				exit(0);
			}

			iPwdLength = GetFlagedValue(lpCmd->Item(iArg), iArgSz, lpCmd->Length(iArg), sPwd, iPwdLength + 1);
			SetPassword(&gPassword, sPwd, iPwdLength);
			free(sPwd);
		}
		else if((iArgSz = IsCommand(lpCmd->Item(iArg), "/AutoClose")) > 0)
		{
			gbAutoClose = true;
		}
		else{
			char sStatus[5120];
			sprintf_s(sStatus, sizeof(sStatus), "Unknown command specified: [%s]", lpCmd->Item(iArg));
			SafeMsgBox(NULL, sStatus, gsTitleCaption, MB_ICONWARNING);
			return 0;
		}
	}

	//--------------------------------------------------------------------------------------------------

	if(bAuto)
	{
		if(gPassword.iLength <= 0)
		{
			SetPassword(&gPassword, gsEmptyPassword);
			giPkgAttribs |= ATTRIB_NOPASSWORD;
		}

		if(!bSourceSpecified || !bTargetSpecified)
		{
			sMsg = "If the \"/Go\" parameter is specified, you must also specifiy:\r\n"
				"\tSource: (by using the \"/Source:\" parameter)\r\n"
				"\tTarget: (by using the \"/Target:\" parameter)\r\n"
				"Example:\r\n\tPE.exe /Source:\"C:\\My Folder\" /Target:\"C:\\NewFile" DEFAULTEXT "\" /GO";
			SafeMsgBox(NULL, sMsg, gsTitleCaption, MB_ICONINFORMATION);
			return 1;
		}
		else if(bEncrypt)
		{
			CreateFileFolderStructure(gsWorkingOutput);
		}
		else{
			CreateFolderStructure(gsWorkingOutput);
		}
	}

	//check that the source file was specified and that it exists.
	if(!bSourceSpecified)
	{
		SafeMsgBox(NULL, "The source file was not specified.", gsTitleCaption, MB_ICONEXCLAMATION);
		return 1;
	}
	else if(!FileAccess(sSource, FExist))
	{
		char sStatus[5120];
		sprintf_s(sStatus, sizeof(sStatus),
			"The specified source file does not exist.\r\n"
			"[%s]", sSource);
		SafeMsgBox(NULL, sStatus, gsTitleCaption, MB_ICONEXCLAMATION);
		return 1;
	}
	else if(bCreateSelfExtractor && (bDecrypt || bEncrypt || bView || bAuto))
	{
		SafeMsgBox(NULL,
			"/CreateSelfExtractor cannot be specified with any other flags except /Silent.",
			gsTitleCaption, MB_ICONEXCLAMATION);
		return 1;
	}

	if(bCreateSelfExtractor)
	{
		gdwCurrentJobType = JOB_TYPE_CREATE_SELF_EXTRACTOR;

		CreateWorker(CreateSelfExtractorProc, (LPVOID) gsArchiveName);

		if(!gbSilent)
		{
			RunProgressDialog(NULL);
		}
		else{
			ResumeThread(hWorkerProc);
		}
		return 0;
	}
	else if((!bDecrypt && !bEncrypt) || bView == true)
	{
        // View an existing archive
        gdwCurrentJobType = JOB_TYPE_READING;

		bool bIsUsingBlankPassword = false;
		if(!IsUsingBlankPassword(NULL, sSource, &bIsUsingBlankPassword))
		{
			return 0;
		}

		if(bIsUsingBlankPassword)
		{
			SetPassword(&gPassword, gsEmptyPassword);
		}
		else{
	        if(DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_OPENPASS), NULL, OpenPassDialog) != 1)
			{
	            return 0;
			}
		}

        CreateWorker(ViewExistingProc, (LPVOID) sSource);
        DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDialog);

		return 0;
	}
	else if(bDecrypt)
    {
		if(bAuto)
		{
			CreateWorker(ExtractEntireArchiveProc, (LPVOID) gsArchiveName);

			if(!gbSilent)
			{
				RunProgressDialog(NULL);
			}
			else{
				ResumeThread(hWorkerProc);
			}
		}
		else{
			// Decrypt the entire archive.
			gdwCurrentJobType = JOB_TYPE_EXTRACT_ALL;

			bool bIsUsingBlankPassword = false;
			if(!IsUsingBlankPassword(NULL, gsArchiveName, &bIsUsingBlankPassword))
			{
				return 0;
			}

			if(bIsUsingBlankPassword)
			{
				SetPassword(&gPassword, gsEmptyPassword);
			}
			else{
				if(DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_OPENPASS), NULL, OpenPassDialog) != 1)
				{
		            return 0;
				}
			}
			//CreateWorker(ViewExistingProc, (LPVOID) sSource);
			DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDialog);
		}
		return 0;
    }
    else if(bEncrypt)
	{
		ResetEncryptionOptions();

		if(FileAccess(gsWorkingOutput, FExist))
		{
			if(gbOverwrite)
			{
				giPkgOptions = OPTION_OVERWRITE;
			}
			else{
				giPkgOptions = OPTION_ADDFILES;
			}
		}

		if(IsDirectory(sSource))
		{
			// Create a new archive from the directorys contents.

			gdwCurrentJobType = JOB_TYPE_PACKING_FOLDER;
			if(bAuto == false)
			{
				//Don't reset options here.
				//ResetEncryptionOptions(); //Reset the global encryption flags.

				if(DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_NEWPASS), NULL, NewPassDialog) != 1)
				{
					return 0;
				}
			}

			CreateWorker(CreateFolderArchiveProc, (LPVOID) sSource);

			if(!gbSilent)
			{
				DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDialog);
			}
			else{
				ResumeThread(hWorkerProc);
			}
		}
		else{
			giPkgOptions |= OPTION_USEALTFILEPACK;

			// Create a new archive containing a single file.
			FilePack fpPack;

			fpPack.Initialize(NULL, &AltFilePackage);
			
			//This is alot of memory, but we need it all.
			char sFileName[MAX_PATH]; 
			char sDirTemp[MAX_PATH]; //Just some memory for AddFile() to use.
			char sRoot[MAX_PATH + 1];
			char sDrive[MAX_PATH + 1];
			char sDir[MAX_PATH + 1];
			char sFile[MAX_PATH + 1];
			char sExt[MAX_PATH + 1];

			int iRootLen = 0;

			_splitpath_s(sSource,
				sDrive, sizeof(sDrive),
				sDir, sizeof(sDir),
				sFile, sizeof(sFile),
				sExt, sizeof(sExt));

			_makepath_s(sRoot, sizeof(sRoot), sDrive, sDir, NULL, NULL);
			_makepath_s(sFileName, sizeof(sFileName), NULL, NULL, sFile, sExt);

			iRootLen = (int)strlen(sRoot);

			fpPack.AddFile(sRoot, iRootLen, sDirTemp, iRootLen, sFileName, &AltFilePackage);

			gdwCurrentJobType = JOB_TYPE_PACKING_FILE;
			if(bAuto == false)
			{
				//Don't reset options here.
				//ResetEncryptionOptions(); //Reset the global encryption flags.

				if(DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_NEWPASS), NULL, NewPassDialog) != 1)
				{
					return 0;
				}
			}

			CreateWorker(CreateFileArchiveProc, (LPVOID) sSource);

			if(!gbSilent)
			{
				DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDialog);
			}
			else{
				ResumeThread(hWorkerProc);
			}
		}

		return 0;
	}

	return 0;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
