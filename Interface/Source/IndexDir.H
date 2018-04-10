#ifndef _IndexDir_H
#define _IndexDir_H
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define AllocIncrement 10

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct FilePackage_Info{
    char **FileNames;
    char **Roots;
    DWORD dwFileCount;
	DWORD dwFolderCount;
	__int64 i64TotalSize;
} FILEPACKAGE, *LPFILEPACKAGE;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class FilePack{
    public:
        bool IndexDirectory(char *inFolder, bool bIncludeHiddenAndSystem, LPFILEPACKAGE lpFilePackage);
		bool IndexDirectoryEx(char *inFolder, bool bIncludeHiddenAndSystem,
			bool bRecursive, HWND hStatus, LPFILEPACKAGE lpFilePackage);
        bool Initialize(HWND hOwnerWindow, LPFILEPACKAGE lpFilePackage);
        void FreeMemory(LPFILEPACKAGE lpFilePackage);

		bool AddFile(const char *sinFolder, int iRootLen, char *sDirTemp,
			int iDirLen, char *sFile, LPFILEPACKAGE lpFilePackage);

		bool CalcFileSizes(LPFILEPACKAGE lpFilePackage);

		bool AddSimpleFile(const char *sFileAndPath, char *sOutRoot,
			int iRootMaxSz, LPFILEPACKAGE lpFilePackage);

		bool IndexDirectorySimple(const char *sinFolder,
			HWND hStatus, bool bIncludeHiddenAndSystem,
			LPFILEPACKAGE lpFilePackage);

		HWND hOwner;

		bool bUnicodeAlert;

		bool IndexDirectoryInternal(const char *sinFolder, int oRootLen,
			bool bIncludeHiddenAndSystem, bool bRecursive, HWND hStatus, LPFILEPACKAGE lpFilePackage);

		bool IndexDirectoryInternal(const char *sinFolder, int iRootLen,
			bool bIncludeHiddenAndSystem, bool bRecursive, HWND hStatus, LPFILEPACKAGE lpFilePackage,
			int iRecursionCount);

		DWORD Allocs;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

