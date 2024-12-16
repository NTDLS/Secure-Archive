#ifndef _Routines_H
#define _Routines_H
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Entry.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define WM_ABOUT_DIALOG	WM_USER + 1007

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _DBGICONLIST {
	char* sExt;
	char* sDescr;
	int iIndex;
} DBGICONLIST, * LPDBGICONLIST;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define ThrowFatalError(sError) ThrowFatalErrorEx(sError, __FILE__, __FUNCTION__, __LINE__)
extern bool gbSilent;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
bool LaunchConfigDlg(HWND hOwner);
#endif

#ifndef _SELFEXTRACTOR_APP
void GetAppTitle(__out char* sText, __in int iMaxSz);
void GetAppTitle(__out char* sText, __in int iMaxSz, __in const char* sFileName);
#endif

#ifdef _DEBUG
bool SaveDebugHex(const char* sFolder, const char* sIdentifier, const char* sData, unsigned long ulDataSize);
#endif

bool WriteCon(const char* sFormat, ...);
bool WriteConEx(const char* sBuffer);
bool WriteConEx(const char* sBuffer, int iLength);
int SafeMsgBox(HWND hWnd, const char* sText, const char* sTitle, const DWORD dwStyle);
bool ComparePasswordTextBoxes(HWND hText1, int iLen1, HWND hText2, int iLen2, int* iKeyWeight);
void ProgressTrayIconProc(CTrayIcon* lpTI, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool SDeleteFile(const char* sFileName);
bool SDeleteFile(const char* sFileName, DWORD dwSecureDeletePasses);

void ResetDefaultAttributes(void);
void ResetEncryptionOptions(void);
void CustomCreateFolderStructure(char* sFullFileName);
void SetStatusFileName(HWND hWnd, const char* StatusText);
bool GetLastDirectoryInPath(char* inFullPath, char* outLastDir);
long MakeChecksum(const char* sRawKey, int iKeySz);
long Reflect_Checksum(long lReflect, char cChar);
void AllocIconList(LPDBGICONLIST lpList, int iArraySize, int iDescriptionLength);
void FreeIconList(LPDBGICONLIST lpList);
bool GetIconForExt(char* inExt, char* outIconFile, int iMaxIconFile,
	int* outIconIndex, char* outDescr, int iMaxDescr);

void FixDirTarget(char* sPath, int iMaxSz);
int iFormattedValue(const char* sFormatted);
char* FileTimeToString(FILETIME* ftInput, bool bReturnNA, char* sOut, int iOutSz);
void ThrowFatalErrorEx(char* sError, char* sFile, char* sProc, int iLine);
void CreateFileFolderStructure(const char* sFullPath);
__int64* SortIntegerArrayASC(__int64* i64Array, unsigned int iItems);
__int64* SortIntegerArrayDESC(__int64* i64Array, unsigned int iItems);
void SetPassword(LPPASSWORD lpPwd, const char* sPwd, int iPwdLen);
void SetPassword(LPPASSWORD lpPwd, HWND hTextBox);
void SetPassword(LPPASSWORD lpPwd, const char* sPwd);
bool OpenHelpFile(void);
bool OpenHelpFile(const char* sTopic);
bool DoesPathContainUnicode(const char* sPath, int iPathLen);
bool DoesPathContainUnicode(const char* sPath);
void GetLastPath(char* sOutPath, int iMaxSize);
void SetLastPath(const char* sPath);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
