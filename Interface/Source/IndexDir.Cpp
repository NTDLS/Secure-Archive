#ifndef _IndexDir_Cpp
#define _IndexDir_Cpp
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <ShlObj.H>
#include <Stdio.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "Entry.H"
#include "Routines.H"
#include "IndexDir.H"

#include "../Dialogs/ProgressDialog.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::File;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool FilePack::CalcFileSizes(LPFILEPACKAGE lpFilePackage)
{
	__int64 i64FileSz = 0;
	char sFullPath[MAX_PATH + 1];

	lpFilePackage->i64TotalSize = 0;

	for(DWORD dwLoop = 0; dwLoop < lpFilePackage->dwFileCount; dwLoop++)
    {
		sprintf_s(sFullPath, sizeof(sFullPath), "%s%s",
			lpFilePackage->Roots[dwLoop], lpFilePackage->FileNames[dwLoop]);

		if(!DoesPathContainUnicode(sFullPath))
		{
			if(FileSize(sFullPath, &i64FileSz))
			{
				lpFilePackage->i64TotalSize += i64FileSz;
			}
			else{
				return false;
			}
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void FilePack::FreeMemory(LPFILEPACKAGE lpFilePackage)
{
	for(DWORD dwLoop = 0; dwLoop < lpFilePackage->dwFileCount; dwLoop++)
    {
		free(lpFilePackage->FileNames[dwLoop]);
		lpFilePackage->FileNames[dwLoop] = NULL;

		free(lpFilePackage->Roots[dwLoop]);
		lpFilePackage->Roots[dwLoop] = NULL;
	}

	free(lpFilePackage->FileNames);
	free(lpFilePackage->Roots);

	lpFilePackage->FileNames = NULL;
	lpFilePackage->Roots = NULL;
	lpFilePackage->i64TotalSize = 0;

    Allocs = 0;
    lpFilePackage->dwFileCount = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool FilePack::Initialize(HWND hOwnerWindow, LPFILEPACKAGE lpFilePackage)
{
    memset(lpFilePackage, 0, sizeof(lpFilePackage));

	this->bUnicodeAlert = false;
	
	if(hOwnerWindow)
	{
		hOwner = hOwnerWindow;
	}
	else{
		hOwner = GetActiveWindow();
	}

	lpFilePackage->FileNames = NULL;
	lpFilePackage->i64TotalSize = 0;

    Allocs = 0;
    lpFilePackage->dwFileCount = 0;

	if((lpFilePackage->Roots =
		(char **) calloc(sizeof(char *), AllocIncrement + 1)) == NULL)
    {
        return false;
    }

	if((lpFilePackage->FileNames =
		(char **) calloc(sizeof(char *), AllocIncrement + 1)) == NULL)
    {
        return false;
    }
    
    Allocs = AllocIncrement;

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int MakeRelativeRoot(const char *inPath, char *outPath, int inPathLen, int iinRootLen)
{
    int iRPos = iinRootLen;
    int iWPos = 0;
    
    while(iRPos < inPathLen)
    {
        outPath[iWPos++] = inPath[iRPos++];
    }

    outPath[iWPos] = '\0';

    return iWPos;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool FilePack::AddSimpleFile(const char *sFileAndPath,
							 char *sOutRoot,
							 int iRootMaxSz,
							 LPFILEPACKAGE lpFilePackage)
{
	char sDir[MAX_PATH];
	char sDrive[MAX_PATH];
	char sExt[MAX_PATH];
	char sFile[MAX_PATH];

	char sPathOnly[MAX_PATH];
	char sFileOnly[MAX_PATH];
	char sDirTemp[MAX_PATH]; //Just some memory for AddFile() to use.

	int iRootLen = 0;

	_splitpath_s(sFileAndPath, sDrive, sizeof(sDrive),
		sDir, sizeof(sDir), sFile, sizeof(sFile), sExt, sizeof(sExt));

	_makepath_s(sPathOnly, sizeof(sPathOnly), sDrive, sDir, NULL, NULL);
	_makepath_s(sFileOnly, sizeof(sFileOnly), NULL, NULL, sFile, sExt);

	if(iRootMaxSz > 0)
	{
		strcpy_s(sOutRoot, iRootMaxSz, sPathOnly);
	}

	iRootLen = (int)strlen(sPathOnly);

	return AddFile(sPathOnly, iRootLen, sDirTemp, iRootLen, sFileOnly, lpFilePackage);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool FilePack::AddFile(const char *sinFolder,
					   int iinRootLen,
					   char *sDirTemp,
					   int iDirLen,
					   char *sFile,
					   LPFILEPACKAGE lpFilePackage)
{
	int iNeededMemory = 0;

	if(iinRootLen + strlen(sFile) >= 255)
	{
		char sError[2048];
		sprintf_s(sError, sizeof(sError),
			"The full path is to the file is too long:\r\n%s", sFile);
		SafeMsgBox(this->hOwner, sError, gsTitleCaption, MB_ICONSTOP | MB_APPLMODAL);
		return false; 
	}

	if(lpFilePackage->dwFileCount == Allocs)
    {
        lpFilePackage->Roots = (char **)
			realloc(lpFilePackage->Roots, ((Allocs * sizeof(char *))
			+ (AllocIncrement * sizeof(char *))) + 1);

        lpFilePackage->FileNames = (char **)
			realloc(lpFilePackage->FileNames, ((Allocs * sizeof(char *))
			+ (AllocIncrement * sizeof(char *))) + 1);
        Allocs = (Allocs + AllocIncrement);
    }

	iNeededMemory = iinRootLen + 1;
	lpFilePackage->Roots[lpFilePackage->dwFileCount] = (char *) calloc(iNeededMemory + 1, 1);
	strncpy_s(lpFilePackage->Roots[lpFilePackage->dwFileCount], iNeededMemory,
		sinFolder, iinRootLen);

	MakeRelativeRoot(sinFolder, sDirTemp, iDirLen, iinRootLen);

	iNeededMemory = (int)strlen(sFile) + (iDirLen - iinRootLen) + 10;
	lpFilePackage->FileNames[lpFilePackage->dwFileCount] = (char *) calloc(iNeededMemory, 1);
	sprintf_s(lpFilePackage->FileNames[lpFilePackage->dwFileCount],
		iNeededMemory, "%s\\%s", sDirTemp, sFile);

	lpFilePackage->dwFileCount++;
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


bool FilePack::IndexDirectorySimple(const char *sinFolder,
									HWND hStatus,
									bool bIncludeHiddenAndSystem,
									LPFILEPACKAGE lpFilePackage)
{
	int iRootLen = (int)strlen(sinFolder) - 1;

	while(sinFolder[iRootLen] != '\\' && sinFolder[iRootLen] != '/')
	{
		iRootLen--;
	}

	return IndexDirectoryInternal(sinFolder, iRootLen, bIncludeHiddenAndSystem, true, hStatus, lpFilePackage);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool FilePack::IndexDirectoryInternal(const char *sinFolder,
									  int iRootLen,
									  bool bIncludeHiddenAndSystem,
									  bool bRecursive,
									  HWND hStatus,
									  LPFILEPACKAGE lpFilePackage)
{
	return this->IndexDirectoryInternal(sinFolder, iRootLen,
		bIncludeHiddenAndSystem, bRecursive, hStatus, lpFilePackage, 0);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool FilePack::IndexDirectoryInternal(const char *sinFolder,
									  int iRootLen,
									  bool bIncludeHiddenAndSystem,
									  bool bRecursive,
									  HWND hStatus,
									  LPFILEPACKAGE lpFilePackage,
									  int iRecursionCount)
{
	lpFilePackage->dwFolderCount++;

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
			if(bIncludeHiddenAndSystem
				|| (!(FD.dwFileAttributes &FILE_ATTRIBUTE_HIDDEN)
				&& !(FD.dwFileAttributes &FILE_ATTRIBUTE_SYSTEM)))
			{
				if(FD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if(bRecursive)
					{
						if(DoesPathContainUnicode(FD.cFileName))
						{
							if(!this->bUnicodeAlert)
							{
								SafeMsgBox(hOwner,
									"One or more of the folder names under the directory you are accessing"
									" contains UNICODE\r\ncharacters, which are not supported."
									" All files and sub-folders of these directories will be skipped.",
									gsTitleCaption, MB_ICONASTERISK);

								this->bUnicodeAlert = true;
							}
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

							SetStatusFileName(hStatus, FD.cFileName);

							if(!this->IndexDirectoryInternal(sTempDir, iRootLen,
								bIncludeHiddenAndSystem, bRecursive, hStatus, lpFilePackage, iRecursionCount + 1))
							{
								if(gdwCurrentJobType == JOB_TYPE_CANCEL)
								{
									return false; //Fail out of recursion.
								}

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

					lpFilePackage->i64TotalSize += FD.nFileSizeLow;

					if(!AddFile(sinFolder, iRootLen, sTempDir, iDirLen, FD.cFileName, lpFilePackage))
					{
						return false; //Fail out of recursion.
					}
				}
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool FilePack::IndexDirectory(char *sinFolder, bool bIncludeHiddenAndSystem, LPFILEPACKAGE lpFilePackage)
{
	lpFilePackage->i64TotalSize = 0;
	return IndexDirectoryInternal(sinFolder, (int)strlen(sinFolder),
		bIncludeHiddenAndSystem, true, NULL, lpFilePackage);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool FilePack::IndexDirectoryEx(char *sinFolder,
								bool bIncludeHiddenAndSystem,
								bool bRecursive,
								HWND hStatus,
								LPFILEPACKAGE lpFilePackage)
{
	lpFilePackage->i64TotalSize = 0;
	return IndexDirectoryInternal(sinFolder, (int)strlen(sinFolder),
		bIncludeHiddenAndSystem, bRecursive, hStatus, lpFilePackage);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
