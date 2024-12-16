#ifndef _ENTRY_H
#define _ENTRY_H
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "../../NSWFL/NSWFL.H"
#include "../../NASCCL/NASCCL.H"
#include "../../CTrayIcon/CTrayIcon.H"

#include "../Resources/Resource.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Hashing;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define DISPLAYACRONYM		"SA-"
#define DEFAULTEXT          ".SAef"
#define DEFAULTHARDKEYEXT   ".SAkf"
#define DEFAULTPRODUCTURL	"http://networkdls.com/Software.Asp?Review=27"
#define UNREGISTEREDDAYS	30
#define UNREGISTEREDDATE	3510588L
#define UNREGISTEREDHASH	1691007L

#define JOB_TYPE_READING				1
#define JOB_TYPE_PACKING_FOLDER			2
#define JOB_TYPE_PACKING_FILE			3
#define JOB_TYPE_ADDING_FILES			4
#define JOB_TYPE_CANCEL					5
#define JOB_TYPE_SUCCESS				6
#define JOB_TYPE_EXTRACT_ALL			7
#define JOB_TYPE_EXTRACT_SELECTED		8
#define JOB_TYPE_DELETE_SELECTED		9
#define JOB_TYPE_CREATE_SELF_EXTRACTOR	10
#define JOB_TYPE_TEST_ARCHIVE			11
#define JOB_TYPE_REPAIR_ARCHIVE			12

#define ID_TRAY_RESTORE	WM_USER + 1

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _tag_ProgressTrayInfo {
	HWND hDialog;
	HWND hProgressBar;
	bool bParentInitialVisibility;
} PROGRESSTRAYINFO, * LPPROGRESSTRAYINFO;

typedef struct tag_CONIO {
	HANDLE StdIn;
	HANDLE StdOut;
	HANDLE StdErr;
} CONIO, * LPCONIO;

typedef struct _tag_Password {
	char* sPassword;
	int iLength;
} PASSWORD, * LPPASSWORD;

extern PASSWORD gPassword;
extern NASCCLStream gCode;
extern CONIO gConIO;
extern CTrayIcon gTrayIcon;

extern HINSTANCE ghAppInstance;

extern char gsAppCopyRight[64];
extern char gsFileVersion[32];
extern char gsTitleCaption[64];
extern char gsRegistryKey[1024];
extern char gsPathKey[1024];
extern char gsCompatibility[100];
extern char gsEmptyPassword[100];
extern char gsWorkingOutput[MAX_PATH + 1];
extern char gsExtractLocation[MAX_PATH + 1];
extern char gsPath[MAX_PATH];
extern char gsLastPath[MAX_PATH];
extern char gsSecureDeletePopulate[MAX_PATH];

extern char gsDfltCompressLevel[255];
extern char gsDfltEncryptAlgo[255];
extern char gsTempFilePath[MAX_PATH]; //Default temp path, leave empty to use default.
extern bool gbDfltChkTimeStamps;
extern bool gbDfltChkAttributes;
extern bool gbDfltChkPathAttributes;
extern bool gbDfltChkCRCs;
extern bool gbIncludeHiddenFiles;
extern bool gbDeleteSourceFiles;
extern bool gbDfltChkSavePaths;
extern bool gbShowGridLines;
extern bool gbShowGridIcons;
extern bool gbEnableFullRowSelect;
extern bool gbEnableDragDrop;
extern bool gbAlwaysOnTop;
extern bool gbKeepSelectionVisible;
extern bool gbUsePreAllocation;
extern bool gbIgnoreFreeSpaceWarning;
extern bool gbDragDropCopyInsteadOfMove;
extern bool gbAlwaysRunInBackground;
extern bool gbWarnAboutNoEncryption;
extern bool gbAutoClose;
extern bool gbArchiveOpen;
extern bool gbOverwrite;

extern DWORD gdwCurrentJobType;
extern DWORD gdwSecureDeletePasses;
extern DWORD gdwWeakEncryptionWarnLevel;

extern bool gbUseProcessBoost;
extern bool gbUseThreadBoost;
extern DWORD_PTR gdwCPUAffinityMask;

extern char gsArchiveName[MAX_PATH + 1];

extern int giPkgAttribs;
extern int giPkgOptions;

extern int giCompressLevel;
extern int giCryptAlgo;
extern DWORD gdwDefaultSortColumn;
extern DWORD gdwDefaultSortDirection;

extern DWORD gdwWeakEncryptionWarnLevel;

extern CRC32* gCRC32;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

