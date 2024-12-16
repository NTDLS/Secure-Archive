#ifndef _CommonDialog_CPP
#define _CommonDialog_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <ShlObj.H>
#include <Stdio.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "../../Library/SecureDeleteLibrary/SecureDelete.H"

#include "Package.H"
#include "Entry.H"
#include "Routines.H"
#include "Init.H"

#include "../Dialogs/TabLookAndFeel.H"
#include "../Dialogs/TabGeneral.H"
#include "../Dialogs/TabProcessors.H"
#include "../Dialogs/TabAdvanced.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows;
using namespace NSWFL::System;
using namespace NSWFL::File;
using namespace NSWFL::DateTime;
using namespace NSWFL::Registry;
using namespace NSWFL::String;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool WriteCon(const char* sFormat, ...)
{
	char sBuffer[10240]; //What else can we do???
	va_list ArgList;
	va_start(ArgList, sFormat);

	int iLength = _vsprintf_s_l(sBuffer, sizeof(sBuffer), sFormat, NULL, ArgList);

	return WriteConEx(sBuffer, iLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool WriteConEx(const char* sBuffer, int iLength)
{
	DWORD dwNumberOfBytesWritten = 0;
	return(WriteFile(gConIO.StdOut, sBuffer, iLength, &dwNumberOfBytesWritten, 0) > 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool WriteConEx(const char* sBuffer)
{
	return WriteConEx(sBuffer, (int)strlen(sBuffer));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SafeMsgBox(HWND hWnd, const char* sText, const char* sTitle, const DWORD dwStyle)
{
	if (gbSilent)
	{
		WriteCon("%s\n", sText);
		return -1000;
	}
	else {
		return MessageBox(hWnd, sText, sTitle, dwStyle);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool DoesPathContainUnicode(const char* sPath)
{
	return DoesPathContainUnicode(sPath, (int)strlen(sPath));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool DoesPathContainUnicode(const char* sPath, int iPathLen)
{
	for (int iPos = 0; iPos < iPathLen; iPos++)
	{
		if (sPath[iPos] == '?')
		{
			return true;
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
bool OpenHelpFile(void)
{
	return OpenHelpFile(NULL);
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
bool OpenHelpFile(const char* sTopic)
{
	char sPath[MAX_PATH];
	char sRegistryKey[1024];
	char sHHExe[1024];

	if (Get_WindowsDirectory(sPath, sizeof(sPath)))
	{
		sprintf_s(sHHExe, sizeof(sHHExe), "%s\\hh.exe", sPath);
	}

	sprintf_s(sRegistryKey, sizeof(sRegistryKey), "%s\\Path", gsRegistryKey);
	DWORD dwSize = sizeof(sPath);
	memset(sPath, 0, sizeof(sPath));
	if (Get_StringRegistryValue(HKEY_LOCAL_MACHINE, sRegistryKey, "Path", sPath, dwSize))
	{
		if (dwSize > 0)
		{
			strcat_s(sPath, sizeof(sPath), "\\Help.chm");
			if (sTopic)
			{
				strcat_s(sPath, sizeof(sPath), "::");
				strcat_s(sPath, sizeof(sPath), sTopic);
			}
			return(((UINT_PTR)ShellExecute(NULL, "OPEN", sHHExe, sPath, NULL, SW_MAXIMIZE)) > 32);
		}
	}
	return false;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool DeleteTreeEx(const char* sDir, DWORD dwSecureDeletePasses)
{
	WIN32_FIND_DATA FD;
	memset(&FD, 0, sizeof(WIN32_FIND_DATA));

	HANDLE hFind = NULL;

	char sPath[2048];

	sprintf_s(sPath, sizeof(sPath), "%s\\*.*", sDir);

	CorrectReversePath(sPath, sizeof(sPath));

	if ((hFind = FindFirstFile(sPath, &FD)) == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	do
	{
		if (strcmp(FD.cFileName, ".") != 0 && strcmp(FD.cFileName, "..") != 0)
		{
			sprintf_s(sPath, sizeof(sPath), "%s\\%s", sDir, FD.cFileName);

			SetFileAttributes(sPath, FILE_ATTRIBUTE_NORMAL);

			if (FD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (!DeleteTreeEx(sPath, dwSecureDeletePasses))
				{
					return false;
				}
				RemoveDirectory(sPath);
			}
			else {
				if (!SecureDeleteFile(sPath, dwSecureDeletePasses))
				{
					//Failed to delete file.
				}
			}
		}
	} while (FindNextFile(hFind, &FD));

	FindClose(hFind);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	Deletes a folder with the default security passes.
*/
bool DeleteTree(const char* sDir)
{
	bool bResult = false;

	if ((bResult = DeleteTreeEx(sDir, gdwSecureDeletePasses)) == true)
	{
		return(RemoveDirectory(sDir) == TRUE);
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	Deletes a folder with a custom number of security passes.
*/
bool DeleteTree(const char* sDir, DWORD dwSecureDeletePasses)
{
	bool bResult = false;

	if ((bResult = DeleteTreeEx(sDir, dwSecureDeletePasses)) == true)
	{
		return(RemoveDirectory(sDir) == TRUE);
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	Deletes a file/folder with the default security passes.
*/
bool SDeleteFile(const char* sFileName)
{
	if (IsDirectory(sFileName))
	{
		return DeleteTree(sFileName, gdwSecureDeletePasses);
	}
	else return SecureDeleteFile(sFileName, gdwSecureDeletePasses);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	Deletes a file/folder with a custom number of security passes.
*/
bool SDeleteFile(const char* sFileName, DWORD dwSecureDeletePasses)
{
	if (IsDirectory(sFileName))
	{
		return DeleteTree(sFileName, dwSecureDeletePasses);
	}
	else return SecureDeleteFile(sFileName, dwSecureDeletePasses);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ThrowFatalErrorEx(char* sError, char* sFile, char* sProc, int iLine)
{
	char sErrorMsg[10240];

	sprintf_s(sErrorMsg, sizeof(sErrorMsg),
		"A fatal error has occurred and the application must be closed. Please take note of the\r\n"
		"following error information and report it to NetworkDLS as soon as possible.\r\n\r\n"
		"%s\r\n\r\n"
		"File: %s\r\n"
		"Procedure: %s\r\n"
		"Line of Code: %d"
#ifndef _SELFEXTRACTOR_APP
		"\r\n\r\nWould you like to view the \"Reporting Bugs\" documentation?"
#endif
		, sError, sFile, sProc, iLine);

#ifndef _SELFEXTRACTOR_APP
	if (SafeMsgBox(NULL, sErrorMsg, DISPLAYACRONYM"Fatal Error!",
		MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
	{
		OpenHelpFile("/Reporting Bugs.Html");
	}
#else
	SafeMsgBox(NULL, sErrorMsg, DISPLAYACRONYM"Fatal Error!", MB_ICONERROR);
#endif

	//DebugBreak();

	exit(1000); //Fatal error.
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CustomCreateFolderStructure(char* sFullFileName)
{
	char sDir[1024];

	int iNameSz = (int)strlen(sFullFileName);
	int iPos = 0;

	if (iNameSz - 1 > sizeof(sDir))
	{
		ThrowFatalError("File path is too long for the allocated buffer.");
	}

	while (iPos < iNameSz)
	{
		if (sFullFileName[iPos] == '\\' || sFullFileName[iPos] == '/')
		{
			sDir[iPos] = '\0';
			CreateDirectory(sDir, NULL);
		}

		sDir[iPos] = sFullFileName[iPos];

		iPos++;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void FixDirTarget(char* sPath, int iMaxSz)
{
	int iLen = (int)strlen(sPath);

	if (sPath[iLen - 1] != '\0')
	{
		strcat_s(sPath, iMaxSz, "\\");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	Trims all but the last 60 characters in a string before displaying them. "...Long file name.txt"
*/
void SetStatusFileName(HWND hWnd, const char* sStatusText)
{
	char sLocalBuffer[60 + 1];

	int iLength = (int)strlen(sStatusText);
	int iRPos = 0;
	int iWPos = 0;
	int iMinPos = (iLength - 60);
	bool bFoundSlash = false;

	if (iLength <= 60)
	{
		Set_Text(hWnd, sStatusText);
		return;
	}

	for (iRPos = iLength; iRPos != iMinPos; iRPos--)
	{
		if (sStatusText[iRPos] == '\\')
		{
			bFoundSlash = true;
			break;
		}
	}

	if (!bFoundSlash)
	{
		iRPos += 3;
		iWPos = 3;
		strcpy_s(sLocalBuffer, sizeof(sLocalBuffer), "...");
	}

	while (iRPos < iLength)
	{
		sLocalBuffer[iWPos++] = sStatusText[iRPos++];
	}

	sLocalBuffer[iWPos] = '\0';

	Set_Text(hWnd, sLocalBuffer);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

long Reflect_Checksum(long lReflect, char cChar)
{
	long Value = 0;

	// Swap Bit 0 For Bit 7 Bit 1 For Bit 6, Etc....
	for (int Loop = 1; Loop < (cChar + 1); Loop++)
	{
		if (lReflect & 1)
		{
			Value |= 1 << (cChar - Loop);
		}

		lReflect >>= 1;
	}

	return Value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

long MakeChecksum(const char* sRawKey, int iKeySz)
{
	char sKeyHash[CHECKSUM_LEN + 1];
	int iRPos = 0;
	long lChecksum = iKeySz;

	NASCCLStream Crypt;
	memset(&Crypt, 0, sizeof(NASCCLStream));

	HashKey(sRawKey, iKeySz, sKeyHash, CHECKSUM_LEN);

	while (iRPos < CHECKSUM_LEN)
	{
		lChecksum = (lChecksum + sKeyHash[iRPos]) + Reflect_Checksum(lChecksum, sKeyHash[iRPos]);
		iRPos++;
	}

	return lChecksum;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool GetLastDirectoryInPath(char* inFullPath, char* outLastDir)
{
	int iRPos = ((int)strlen(inFullPath) - 1);
	int iWPos = 0;

	if (iRPos <= 0)
	{
		outLastDir[0] = '\0';
		return false;
	}

	if (inFullPath[iRPos] == '\\' || inFullPath[iRPos] == '//')
	{
		iRPos--;
	}

	if (inFullPath[iRPos] != ':' && inFullPath[iRPos - 1] != ':')
	{
		while (iRPos != 0 && inFullPath[iRPos] != '\\' && inFullPath[iRPos] != '//')
		{
			outLastDir[iWPos] = inFullPath[iRPos];
			iRPos--;
			iWPos++;
		}

		outLastDir[iWPos++] = '\\';
		outLastDir[iWPos] = '\0';
		ReverseString(outLastDir, iWPos);
	}
	else {
		outLastDir[0] = '\0';
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
void AllocIconList(LPDBGICONLIST lpList, int iArraySize, int iDescriptionLength)
{
	lpList->sExt = (char*)calloc(sizeof(char), iArraySize + 1);
	lpList->sDescr = (char*)calloc(sizeof(char), iDescriptionLength + 1);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
void FreeIconList(LPDBGICONLIST lpList)
{
	free(lpList->sExt);
	free(lpList->sDescr);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool GetIconForExt(char* inExt, char* outIconFile, int iMaxIconFile,
	int* outIconIndex, char* outDescr, int iMaxDescr)
{
	char lsKey[256];
	char lsDefaultIcon[MAX_PATH];
	char lsTempKey[255];
	char sIcoPos[10];
	DWORD dwBufferLen = 0;

	dwBufferLen = sizeof(lsKey);
	memset(lsKey, 0, dwBufferLen);
	if (!Get_StringRegistryValue(HKEY_CLASSES_ROOT, inExt, NULL, lsKey, dwBufferLen))
	{
		return false;
	}

	dwBufferLen = iMaxDescr;
	memset(outDescr, 0, dwBufferLen);
	if (!Get_StringRegistryValue(HKEY_CLASSES_ROOT, lsKey, NULL, outDescr, dwBufferLen))
	{
		return false;
	}

	sprintf_s(lsTempKey, sizeof(lsTempKey), "%s\\DefaultIcon", lsKey);

	dwBufferLen = sizeof(lsDefaultIcon);
	memset(lsDefaultIcon, 0, dwBufferLen);
	if (!Get_StringRegistryValue(HKEY_CLASSES_ROOT, lsTempKey, NULL, lsDefaultIcon, dwBufferLen))
	{
		return false;
	}

	int iRPos = (int)strlen(lsDefaultIcon) - 1;
	int iWPos = 0;

	memset(sIcoPos, 0, sizeof(sIcoPos));

	while (iRPos > 1)
	{
		if (lsDefaultIcon[iRPos] != ' ' && lsDefaultIcon[iRPos] != '\t')
		{
			if (lsDefaultIcon[iRPos] == ',')
			{
				lsDefaultIcon[iRPos] = '\0';
				break;
			}

			if (!(lsDefaultIcon[iRPos] >= '0' && lsDefaultIcon[iRPos] <= '9') && lsDefaultIcon[iRPos] != '-')
			{
				break;
			}

			sIcoPos[iWPos] = lsDefaultIcon[iRPos];
			iWPos++;
		}

		iRPos--;
	}

	if (iWPos > 0)
	{
		ReverseString(sIcoPos, iWPos);
		*outIconIndex = atoi(sIcoPos);
	}

	strcpy_s(outIconFile, iMaxIconFile, lsDefaultIcon);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

char* FileTimeToString(FILETIME* ftInput, bool bReturnNA, char* sOut, int iOutSz)
{
	if (!bReturnNA)
	{
		char sTimeTemp[64];
		char sDateTemp[64];

		SYSTEMTIME ST;
		FILETIME lFT;
		FileTimeToLocalFileTime(ftInput, &lFT);
		if (FileTimeToSystemTime(&lFT, &ST))
		{
			GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &ST, NULL, sTimeTemp, sizeof(sTimeTemp));
			GetDateFormat(NULL, NULL, &ST, "MM-dd-yyyy", sDateTemp, sizeof(sDateTemp));

			sprintf_s(sOut, iOutSz, "%s %s", sDateTemp, sTimeTemp);
		}
		else strcpy_s(sOut, iOutSz, "(n/a)");
	}
	else strcpy_s(sOut, iOutSz, "(n/a)");

	return sOut;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CreateFileFolderStructure(const char* sFullPath)
{
	char sCreatePath[MAX_PATH];
	int iRPos = ((int)strlen(sFullPath) - 1);
	int iWPos = 0;

	while (sFullPath[iRPos] != '\\' && sFullPath[iRPos] != '/')
	{
		iRPos--;
	}

	while (iRPos != -1)
	{
		sCreatePath[iWPos++] = sFullPath[iRPos--];
	}

	sCreatePath[iWPos] = '\0';

	ReverseString(sCreatePath, iWPos);

	CustomCreateFolderStructure(sCreatePath);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool EncodePassword(LPPASSWORD lpPwd)
{
	SHA1 SHA;
	unsigned long ulDigest[5];
	char sHash[41];

	SHA.Input(lpPwd->sPassword, lpPwd->iLength);
	if (!SHA.Result(ulDigest))
	{
		ThrowFatalError("Failed to generate the key hash.");
		return false;
	}
	SHA.Reset();

	if (!SHA.ToString(ulDigest, sHash, sizeof(sHash)))
	{
		ThrowFatalError("Failed to convert the key hash.");
		return false;
	}

	NASCCLStream Crypt;
	memset(&Crypt, 0, sizeof(Crypt));

	Crypt.Initialize(sHash);
	Crypt.Cipher(lpPwd->sPassword, lpPwd->iLength);
	Crypt.Destroy();

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SetPassword(LPPASSWORD lpPwd, HWND hTextBox)
{
	int iPwdLen = Get_TextLength(hTextBox);

	SetPassword(lpPwd, NULL, 0); //Free RAM.

	if (iPwdLen > 0)
	{
		lpPwd->iLength = iPwdLen;
		lpPwd->sPassword = (char*)calloc(sizeof(char), lpPwd->iLength + 1);
		Get_Text(hTextBox, lpPwd->sPassword, lpPwd->iLength + 1);
		//EncodePassword(lpPwd);
	}

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SetPassword(LPPASSWORD lpPwd, const char* sPwd)
{
	SetPassword(lpPwd, sPwd, (int)strlen(sPwd));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SetPassword(LPPASSWORD lpPwd, const char* sPwd, int iPwdLen)
{
	if (lpPwd->iLength > 0 && lpPwd->sPassword)
	{
		free(lpPwd->sPassword);
		lpPwd->iLength = 0;
	}

	if (iPwdLen > 0)
	{
		lpPwd->iLength = iPwdLen;
		lpPwd->sPassword = (char*)calloc(sizeof(char), lpPwd->iLength + 1);
		memcpy_s(lpPwd->sPassword, lpPwd->iLength + 1, sPwd, iPwdLen);
		//EncodePassword(lpPwd);
	}
	else {
		lpPwd->iLength = 0;
		lpPwd->sPassword = "";
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
bool ComparePasswordTextBoxes(HWND hText1, int iLen1, HWND hText2, int iLen2, int* iKeyWeight)
{
	if (iLen1 != iLen2)
	{
		return false;
	}

	char* sBuf1 = (char*)calloc(sizeof(char), iLen1 + 1);
	char* sBuf2 = (char*)calloc(sizeof(char), iLen2 + 1);

	if (sBuf1 == NULL || sBuf2 == NULL)
	{
		SafeMsgBox(GetActiveWindow(), "Failed to allocate memory for your password. Out of memory?", gsTitleCaption, MB_ICONERROR);
		return false;
	}

	Get_Text(hText1, sBuf1, iLen1 + 1);
	Get_Text(hText2, sBuf2, iLen1 + 1);

	*iKeyWeight = KeyWeight(sBuf1, iLen1);

	for (int iRPos = 0; iRPos < iLen1; iRPos++)
	{
		if (sBuf1[iRPos] != sBuf2[iRPos])
		{
			free(sBuf1);
			free(sBuf2);
			return false;
		}
	}

	free(sBuf1);
	free(sBuf2);
	return true;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ProgressTrayIconProc(CTrayIcon* lpTI, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static int iLastIcon = 0;

	if (uMsg == WM_CTRAYICON_INITIALIZE)
	{
		iLastIcon = 0;

		iLastIcon = -1; //Force refresh.

		SetTimer(hWnd, 0, 1000, NULL);
		SendMessage(hWnd, WM_TIMER, 0, 0);
	}
	else if (uMsg == WM_TIMER)
	{
		PROGRESSTRAYINFO* pPTI = ((PROGRESSTRAYINFO*)lpTI->UserData);

		char sText[128];
		char sFile[MAX_PATH];
		PBRANGE PBRange;
		memset(&PBRange, 0, sizeof(PBRange));

		GetFileName(gsArchiveName, sFile, sizeof(sFile));

		SendMessage(pPTI->hProgressBar, PBM_GETRANGE, 0, (LPARAM)&PBRange);
		float fProgress = (fPackageProgress / (float)MAKELONG(PBRange.iHigh, PBRange.iLow)) * 100.0f;

		sprintf_s(sText, sizeof(sText), "%0.2f%% - %s", fProgress, sFile);

		int iNewIcon = (int)(16.0f * (fProgress / 100.0));
		if (iNewIcon <= 0) {
			iNewIcon = 0;
		}
		else if (iNewIcon >= 100.0) {
			iNewIcon = 15;
		}

		if (iLastIcon != iNewIcon)
		{
			int iIcon = 0;
			if (iNewIcon <= 0)
				iIcon = IDI_TRAYSTAT_00;
			else if (iNewIcon == 1)
				iIcon = IDI_TRAYSTAT_01;
			else if (iNewIcon == 2)
				iIcon = IDI_TRAYSTAT_02;
			else if (iNewIcon == 3)
				iIcon = IDI_TRAYSTAT_03;
			else if (iNewIcon == 4)
				iIcon = IDI_TRAYSTAT_04;
			else if (iNewIcon == 5)
				iIcon = IDI_TRAYSTAT_05;
			else if (iNewIcon == 6)
				iIcon = IDI_TRAYSTAT_06;
			else if (iNewIcon == 7)
				iIcon = IDI_TRAYSTAT_07;
			else if (iNewIcon == 8)
				iIcon = IDI_TRAYSTAT_08;
			else if (iNewIcon == 9)
				iIcon = IDI_TRAYSTAT_09;
			else if (iNewIcon == 10)
				iIcon = IDI_TRAYSTAT_10;
			else if (iNewIcon == 11)
				iIcon = IDI_TRAYSTAT_11;
			else if (iNewIcon == 12)
				iIcon = IDI_TRAYSTAT_12;
			else if (iNewIcon == 13)
				iIcon = IDI_TRAYSTAT_13;
			else if (iNewIcon == 14)
				iIcon = IDI_TRAYSTAT_14;
			else if (iNewIcon >= 15)
				iIcon = IDI_TRAYSTAT_15;

			lpTI->Update(iIcon, sText);

			iLastIcon = iNewIcon;
		}
		else {
			lpTI->Update(sText);
		}
	}
	else if (uMsg == WM_COMMAND)
	{
		if (LOWORD(wParam) == ID_TRAY_RESTORE)
		{
			PROGRESSTRAYINFO* pPTI = ((PROGRESSTRAYINFO*)lpTI->UserData);

			gTrayIcon.Destroy();

			ShowWindow(pPTI->hDialog, SW_SHOW);
			if (pPTI->bParentInitialVisibility)
			{
				ShowWindow(GetParent(pPTI->hDialog), SW_SHOW);
			}
			ShowWindow(pPTI->hDialog, SW_NORMAL);
		}
	}
	else if (uMsg == WM_SHELLNOTIFY)
	{
		if (lParam == WM_LBUTTONDBLCLK)
		{
			SendMessage(hWnd, WM_COMMAND, MAKELONG(ID_TRAY_RESTORE, 0), 0);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
bool SaveDebugHex(const char* sFolder, const char* sIdentifier, const char* sData, unsigned long ulDataSize)
{
	char sFileName[MAX_PATH];

	sprintf_s(sFileName, sizeof(sFileName),
		"%s\\%s (%d).txt", sFolder, sIdentifier, GetTickCount());

	return BufferDataToFile(sFileName, sData, ulDataSize);
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
bool LaunchConfigDlg(HWND hOwner)
{
	PROPSHEETPAGE psp[7];
	PROPSHEETHEADER psh;
	int iPage = 0;

	memset(&psp[iPage], '\0', sizeof(PROPSHEETPAGE));
	psp[iPage].dwSize = sizeof(PROPSHEETPAGE);
	psp[iPage].dwFlags = PSP_DEFAULT | PSP_USETITLE;
	psp[iPage].hInstance = ghAppInstance;
	psp[iPage].pszTemplate = MAKEINTRESOURCE(IDD_TABGENERAL);
	psp[iPage].pszIcon = NULL;
	psp[iPage].pszTitle = "General";
	psp[iPage].pfnDlgProc = (DLGPROC)TabGeneralDialog;
	psp[iPage].lParam = 0;
	psp[iPage].pfnCallback = NULL;
	psp[iPage].pcRefParent = NULL;
	iPage++;

	memset(&psp[iPage], '\0', sizeof(PROPSHEETPAGE));
	psp[iPage].dwSize = sizeof(PROPSHEETPAGE);
	psp[iPage].dwFlags = PSP_DEFAULT | PSP_USETITLE;
	psp[iPage].hInstance = ghAppInstance;
	psp[iPage].pszTemplate = MAKEINTRESOURCE(IDD_TABPROCESSORS);
	psp[iPage].pszIcon = NULL;
	psp[iPage].pszTitle = "Processor Settings";
	psp[iPage].pfnDlgProc = (DLGPROC)TabProcessorsDialog;
	psp[iPage].lParam = 0;
	psp[iPage].pfnCallback = NULL;
	psp[iPage].pcRefParent = NULL;
	iPage++;

	memset(&psp[iPage], '\0', sizeof(PROPSHEETPAGE));
	psp[iPage].dwSize = sizeof(PROPSHEETPAGE);
	psp[iPage].dwFlags = PSP_DEFAULT | PSP_USETITLE;
	psp[iPage].hInstance = ghAppInstance;
	psp[iPage].pszTemplate = MAKEINTRESOURCE(IDD_TABLOOKANDFEEL);
	psp[iPage].pszIcon = NULL;
	psp[iPage].pszTitle = "Look && Feel";
	psp[iPage].pfnDlgProc = (DLGPROC)TabLookAndFeelDialog;
	psp[iPage].lParam = 0;
	psp[iPage].pfnCallback = NULL;
	psp[iPage].pcRefParent = NULL;
	iPage++;

	memset(&psp[iPage], '\0', sizeof(PROPSHEETPAGE));
	psp[iPage].dwSize = sizeof(PROPSHEETPAGE);
	psp[iPage].dwFlags = PSP_DEFAULT | PSP_USETITLE;
	psp[iPage].hInstance = ghAppInstance;
	psp[iPage].pszTemplate = MAKEINTRESOURCE(IDD_TABADVANCED);
	psp[iPage].pszIcon = NULL;
	psp[iPage].pszTitle = "Advanced";
	psp[iPage].pfnDlgProc = (DLGPROC)TabAdvancedDialog;
	psp[iPage].lParam = 0;
	psp[iPage].pfnCallback = NULL;
	psp[iPage].pcRefParent = NULL;
	iPage++;

	memset(&psh, '\0', sizeof(PROPSHEETHEADER));
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_USEHICON | PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
	psh.hwndParent = hOwner;
	psh.hInstance = ghAppInstance;
	psh.hIcon = LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN));
	psh.pszCaption = DISPLAYACRONYM"Settings";
	psh.nPages = iPage;
	psh.nStartPage = 0;
	psh.ppsp = (LPCPROPSHEETPAGE)&psp;
	psh.pfnCallback = NULL;

	PropertySheet(&psh);

	SetRegistryValues();

	return true;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ResetDefaultAttributes(void)
{
	if (giPkgAttribs == 0)
	{
		if (gbDfltChkSavePaths)
		{
			giPkgAttribs |= ATTRIB_SAVEPATHS;
		}
		else {
			giPkgAttribs &= ~ATTRIB_SAVEPATHS;
		}

		if (gbDfltChkTimeStamps)
		{
			giPkgAttribs |= ATTRIB_DATETIME;
		}
		else {
			giPkgAttribs &= ~ATTRIB_DATETIME;
		}

		if (gbDfltChkAttributes)
		{
			giPkgAttribs |= ATTRIB_ATTRIBUTES;
		}
		else {
			giPkgAttribs &= ~ATTRIB_ATTRIBUTES;
		}

		if (gbDfltChkCRCs)
		{
			giPkgAttribs |= ATTRIB_CRCCHECK;
		}
		else {
			giPkgAttribs &= ~ATTRIB_CRCCHECK;
		}

		if (gbIncludeHiddenFiles)
		{
			giPkgAttribs |= ATTRIB_INCLUDEHIDDEN;
		}
		else {
			giPkgAttribs &= ~ATTRIB_INCLUDEHIDDEN;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
void ResetEncryptionOptions(void)
{
	giPkgOptions = 0;

	if (gbDeleteSourceFiles)
	{
		giPkgOptions |= OPTION_DELETESOURCE;
	}
	else {
		giPkgOptions &= ~OPTION_DELETESOURCE;
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GetLastPath(char* sOutPath, int iMaxSize)
{
	if (strlen(gsLastPath) > 0)
	{
		if (FileAccess(gsLastPath, FExist))
		{
			if (IsDirectory(gsLastPath))
			{
				strcpy_s(sOutPath, iMaxSize, gsLastPath);
				SetLastPath(sOutPath);
				return;
			}
			else {
				GetFilePath(gsLastPath, sOutPath, iMaxSize);
				SetLastPath(sOutPath);
				return;
			}
		}
	}
	if (strlen(gsWorkingOutput) > 0)
	{
		if (FileAccess(gsWorkingOutput, FExist))
		{
			if (IsDirectory(gsWorkingOutput))
			{
				strcpy_s(sOutPath, iMaxSize, gsWorkingOutput);
				SetLastPath(sOutPath);
				return;
			}
			else {
				GetFilePath(gsWorkingOutput, sOutPath, iMaxSize);
				SetLastPath(sOutPath);
				return;
			}
		}
	}

	if (!Get_DesktopDirectory(sOutPath, iMaxSize))
	{
		strcpy_s(sOutPath, iMaxSize, "C:\\");
	}
	SetLastPath(sOutPath);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SetLastPath(const char* sPath)
{
	if (strlen(sPath) > 0)
	{
		if (FileAccess(sPath, FExist))
		{
			if (IsDirectory(sPath))
			{
				strcpy_s(gsLastPath, sizeof(gsLastPath), gsLastPath);
			}
			else {
				GetFilePath(sPath, gsLastPath, sizeof(gsLastPath));
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
void GetAppTitle(__out char* sText, __in int iMaxSz, __in const char* sFileName)
{
	strcpy_s(sText, iMaxSz, "");
	strcat_s(sText, iMaxSz, gsTitleCaption);

	if (sFileName)
	{
		strcat_s(sText, iMaxSz, " [");
		strcat_s(sText, iMaxSz, sFileName);
		strcat_s(sText, iMaxSz, "]");
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
void GetAppTitle(__out char* sText, __in int iMaxSz)
{
	GetAppTitle(sText, iMaxSz, NULL);
}
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
