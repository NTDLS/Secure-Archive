#ifndef _PACKAGE_H
#define _PACKAGE_H
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "IndexDir.H"
#include "PackHeader.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define DEFAULT_BUF_SZ			524288	// 1/2 MB Buffer.
#define EXTRA_SIZE				524288
#define PREALLOCATION_SLOP		1048576

#define PACK_RESULT_OK			0
#define PACK_RESULT_NOACTION	1
#define PACK_RESULT_CANCEL		2
#define PACK_RESULT_ERROR		3
#define PACK_RESULT_RESUME		4

#ifndef _SELFEXTRACTOR_APP
#define OPTION_ADDFILES			0x0001	//Add files to the archive, otherwise replace the existing archive.
#define OPTION_USEALTFILEPACK	0x0002	//Use the contents of the passed lpFilePack structure.
#define OPTION_OVERWRITE		0x0004	//Force overwrite of existing package.
#define OPTION_DELETESOURCE		0x0010
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _tag_GridDeleteItem {
	__int64 i64FilePosition;
	unsigned int ulGridPosition;
} GRIDDELETEITEM, *LPGRIDDELETEITEM;

typedef struct _tag_GridDeleteItems {
	HWND hGrid;
	unsigned long ulCount;
	GRIDDELETEITEM *Items;
} GRIDDELETEITEMS, *LPGRIDDELETEITEMS;

typedef struct _MakePackage_Params{
	char *sSrcFolder;
	char *sTarName;
	//char *sRawKey;
	//int iRawKeyLen;
	int iPkgAttribs;
	int iOptions;
} MKPKGPARAM, *LPMKPKGPARAM;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern float fPackageProgress;
extern float fFileProgress;
extern bool gbOverwriteDlft;
extern bool gbUseFolderNames;

extern FILEPACKAGE AltFilePackage;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
__inline bool UpdateFileCount(__int64 i64ExeEnd, HANDLE hSource, CNASCCL *lpCode, DWORD dwFileCount);
__inline bool ShouldFileBeDeleted(GRIDDELETEITEMS *pGDI, __int64 i64Position, const char *sFileName);
int DeleteFilesFromPackage(char *sSrcName, char *TarFolder,
							LPPASSWORD lpPwd, GRIDDELETEITEMS *pGDI, DWORD dwFileCount);
int MakePackage(LPMKPKGPARAM lpMkPkgParam, LPPASSWORD lpPwd);
int PopGrid(char *sSrcName, LPPASSWORD lpPwd);
bool CreateSelfExtractor(const char *sSourceFile, const char *sTargetFile);
bool RepairCorruptArchive(const char *sSourceFile, const char *sTargetFile, LPPASSWORD lpPwd);
bool DupsSelected(void);
#endif
__inline bool SafeWrite(HANDLE hFile, VOID *lpData, DWORD dwSize);
__inline bool SafeWrite(HWND hWnd, HANDLE hFile, VOID *lpData, DWORD dwSize);
int BreakPackage(char *sSrcName, char *TarFolder, LPPASSWORD lpPwd, bool bTestOnly);
bool CheckFreeSpaceAndPreAllocFile(HWND hWnd, HANDLE hFile, const char *sFileName,
					 unsigned __int64 i64MinSize);

bool SecurelyDeleteSourceFiles(const char *sRoot, FILEPACKAGE *Pkg, bool bUseAlternateRoot);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
