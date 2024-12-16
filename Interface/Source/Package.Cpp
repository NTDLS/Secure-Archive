#ifndef _PACKAGE_CPP
#define _PACKAGE_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <ShlObj.H>
#include <Stdio.H>
#include <Stdlib.H>
#include <IO.H>
#include <FCNTL.h>
#include <Sys/Types.h>
#include <Sys/Stat.h>
#include <Share.h>

#include "IndexDir.H"
#include "Package.H"
#include "Entry.H"
#include "Routines.H"
#include "PackHeader.H"
#include "Workers.H"
#include "ListView.H"

#include "../Dialogs/MainDlg.H"
#include "../Dialogs/OpenPassDlg.H"
#include "../Dialogs/ReadingDlg.H"
#include "../Dialogs/ProgressDialog.H"
#include "../Dialogs/ExtractDlg.H"
#include "../Dialogs/ReplaceFileDlg.H"

#include "../../Library/Compression/zLib/ZLibEncapsulation.H"

#include "../../Library/Rijndael/AES.H"
#include "../../Library/Rijndael/AESOpt.H"
#include "../../Library/Rijndael/Rijndael.H"
#include "../../Library/BlowFish/BlowFish.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::File;
using namespace NSWFL::String;
using namespace NSWFL::Windows;
using namespace NSWFL::Conversion;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float fPackageProgress = 0;
float fFileProgress = 0;
bool gbOverwriteDlft = false;
bool gbUseFolderNames = true;

FILEPACKAGE AltFilePackage;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
bool RepairCorruptArchive(const char *sSourceFile, const char *sTargetFile, LPPASSWORD lpPwd)
{
	bool bResult = false;

	char sPartFileName[MAX_PATH + 1];
    char sStatusText[5120];

    __int64 i64CompressedFileSize = 0;
	__int64 i64FileEndPos = 0;
    __int64 i64FilePosition = 0;
    __int64 i64FileSize = 0;

	DWORD dwCompatVersion = 0;
	DWORD dwBytesRead = 0;
	DWORD dwFileAttributes = 0;

	FILETIME ftCreationTime;
    FILETIME ftLastWriteTime;

	HANDLE hfSource = NULL;

    long lReadChecksum = 0;

	unsigned long ulReadCRC = 0;
    unsigned short usFileNameLen = 0;

    NASCCLStream DfltCode;
	HEADER Header;

    memset(&DfltCode, 0, sizeof(DfltCode));
    memset(&Header, 0, sizeof(Header));

	__int64 i64ExeEnd = GetExeEnd(sSourceFile);

	hfSource = CreateFile(
        sSourceFile,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if(hfSource == INVALID_HANDLE_VALUE)
    {
		sprintf_s(sStatusText, sizeof(sStatusText), "Failed to open read file:\r\n'%s'.", sSourceFile);
        SafeMsgBox(hReadingDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
		return false;
    }

    DfltCode.Initialize(lpPwd->sPassword, lpPwd->iLength, true);

	if(!ReadPackageHeader(i64ExeEnd, hfSource, hReadingDialog, lpPwd, &DfltCode, &Header, false))
	{
		DfltCode.Destroy();
		CloseHandle(hfSource);
		return false;
	}

	i64FileEndPos = FileSeek(hfSource);

	int iFilesRead = 0;

    do {
		i64FilePosition = FileSeek(hfSource, i64FileEndPos, FILE_BEGIN);

		//-----------------------------------------------------------------------------------------------|
		//-----[BEGIN READ FILE HEADER]------------------------------------------------------------------|
		//-----------------------------------------------------------------------------------------------|

		//Read the file name length.
		ReadFile(hfSource, &usFileNameLen, sizeof(unsigned short), &dwBytesRead, NULL);
		if(dwBytesRead == 0)
		{
			//This is where this loop ends.
			bResult = true;
		}

		if(usFileNameLen <= 0 || usFileNameLen >= MAX_PATH) //Sanity check.
		{
			DfltCode.Destroy();

			sprintf_s(sStatusText, sizeof(sStatusText),
				"Corrupt data file detected. Error code: (RepairCorruptArchive->usFileNameLen:%d|%d)", __LINE__, usFileNameLen);

            SafeMsgBox(hReadingDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
			bResult = false;
			break;
        }

		//Read (and decode) the file name.
		ReadFile(hfSource, sPartFileName, usFileNameLen, &dwBytesRead, NULL);
        DfltCode.Cipher(sPartFileName, sPartFileName, usFileNameLen);
    	sPartFileName[usFileNameLen] = '\0';

		//Read the file size.
        ReadFile(hfSource, &i64FileSize, sizeof(i64FileSize), &dwBytesRead, NULL);

		//Read file attributes
		if(Header.iPkgAttribs &ATTRIB_ATTRIBUTES)
        {
            ReadFile(hfSource, &dwFileAttributes, sizeof(dwFileAttributes), &dwBytesRead, NULL);
        }

		//Read the creation and last modified date / time.
		if(Header.iPkgAttribs &ATTRIB_DATETIME)
		{
			ReadFile(hfSource, &ftCreationTime, sizeof(FILETIME), &dwBytesRead, NULL);
			ReadFile(hfSource, &ftLastWriteTime, sizeof(FILETIME), &dwBytesRead, NULL);
		}
		
		//Read the compressed file size.
		ReadFile(hfSource, &i64CompressedFileSize, sizeof(i64CompressedFileSize), &dwBytesRead, NULL);
		if(i64CompressedFileSize == -1) //Sanity check.
        {
			DfltCode.Destroy();

			sprintf_s(sStatusText, sizeof(sStatusText),
				"Corrupt data file detected. Error code: (RepairCorruptArchive->i64CompressedFileSize:%d|%lld)", __LINE__, i64CompressedFileSize);

            SafeMsgBox(hReadingDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

            bResult = false;
			break;
        }
		
		//Read the file CRC.
		if(Header.iPkgAttribs &ATTRIB_CRCCHECK)
        {
            ReadFile(hfSource, &ulReadCRC, sizeof(unsigned long), &dwBytesRead, NULL);
        }

		//Read the end-of-file position.
		ReadFile(hfSource, &i64FileEndPos, sizeof(i64FileEndPos), &dwBytesRead, NULL);
		if(i64FileEndPos == -1) //Sanity check.
        {
			DfltCode.Destroy();

			sprintf_s(sStatusText, sizeof(sStatusText),
				"Corrupt data file detected. Error code: (RepairCorruptArchive->i64FileEndPos:%d|%lld)", __LINE__, i64FileEndPos);
			
            SafeMsgBox(hReadingDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

            bResult = false;
			break;
        }
				
		FileSeek(hfSource, i64FileEndPos, FILE_CURRENT);

		//-----------------------------------------------------------------------------------------------|
		//-----[END READ FILE HEADER]--------------------------------------------------------------------|
		//-----------------------------------------------------------------------------------------------|
		
        if(gdwCurrentJobType == JOB_TYPE_CANCEL)
        {
			bResult = false;
            Sleep(1000);
            break;
        }

		iFilesRead++;
        fPackageProgress = (float)iFilesRead;

	} while(dwBytesRead != 0);

    CloseHandle(hfSource);
	DfltCode.Destroy();

	return bResult;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
int MakePackage(LPMKPKGPARAM lpMkPkgParam, LPPASSWORD lpPwd)
{
	unsigned char *sBlockBuffer1 = NULL;
    unsigned char *sBlockBuffer2 = NULL;
    
	char sExt[MAX_PATH + 1];
    char sFileName[MAX_PATH + 1];
    char sFullFileName[MAX_PATH + 1];
    char sFileNameToSave[MAX_PATH + 1];
    char sStatusText[5120];
    char sTempBuf[1024];

    __int64 i64CompressedFileSize = 0;
    __int64 i64HeaderGapPos = 0;
	__int64 i64TopOfFileDataPosition = 0;
    __int64 i64FileSize = 0;
	__int64 i64TotalBytesRead = 0;
	__int64 i64Temp = 0;
	__int64 i64EndOfLastFile = 0;
	int iActionResult = PACK_RESULT_OK;

	LARGE_INTEGER liFileSize;

	DWORD dwCompatVersion = 0;
	DWORD dwBytesRead = 0;
    DWORD dwDeflSize = 0;
    DWORD dwBlockSize = -1;
	DWORD dwTemp = 0;
	DWORD dwSkippedFiles = 0;

	bool bTargetExist = false;
	bool bSafeToDeleteOutput = true;

	HANDLE hfSource = NULL;
    HANDLE hfTarget = NULL;

	int iCompressResult = 0;
    const int iBufferSize = DEFAULT_BUF_SZ;
	int iFlush = 0;

	unsigned char *sSourceBuf = NULL;
    unsigned char *sTargetBuf = NULL;
    unsigned long ulFinalCRC = 0;
    unsigned long ulFilesProcessed = 0;
    unsigned long ulPreExistingFiles = 0;
    unsigned short usFileNameLen = 0;

	NASCCLStream SS;
	NASCCLStream DfltCode;
	BlowFishCTX BlowFish;
	aes_ctx Aes_Ctx;
	aes_ctx Aes_CtxPerFile;
    FilePack fpPack;
	z_stream zStream;
	FILEPACKAGE Pkg;
	HEADER Header;
	BY_HANDLE_FILE_INFORMATION SourceFileInfo;

    memset(&DfltCode, 0, sizeof(DfltCode));
	memset(&fpPack, 0, sizeof(fpPack));
    memset(&sTempBuf, 0, sizeof(sTempBuf));
    memset(&Pkg, 0, sizeof(FILEPACKAGE));
    memset(&Header, 0, sizeof(Header));

	if(FileAccess(lpMkPkgParam->sTarName, FExist))
	{
		bTargetExist = true;
	}
	else {
		bTargetExist = false;
	}

	if(lpMkPkgParam->iOptions &OPTION_USEALTFILEPACK)
	{
		memcpy(&Pkg, &AltFilePackage, sizeof(FILEPACKAGE));

		if(!fpPack.CalcFileSizes(&Pkg))
		{
			sprintf_s(sStatusText, sizeof(sStatusText),
				"Failed to retrieve the total directory data size: \r\n'%s'",
				lpMkPkgParam->sSrcFolder);

			SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
			return PACK_RESULT_NOACTION;
		}
	}
	else{
		fpPack.Initialize(hProgressDialog, &Pkg);

		Set_Text(hPackingActionText, "Searching:");

		if(!fpPack.IndexDirectoryEx(lpMkPkgParam->sSrcFolder,
			(lpMkPkgParam->iPkgAttribs &ATTRIB_INCLUDEHIDDEN) > 0, true, hPackingStatusText, &Pkg))
		{
			if(gdwCurrentJobType == JOB_TYPE_CANCEL)
			{
				return PACK_RESULT_NOACTION;
			}
			else{
				sprintf_s(sStatusText, sizeof(sStatusText),
					"An error occurred while trying to index the directory:\r\n'%s'",
					lpMkPkgParam->sSrcFolder);

				SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
				return PACK_RESULT_NOACTION;
			}
		}
	}

	if(Pkg.dwFileCount <= 0)
	{
		SafeMsgBox(hProgressDialog, "No suitable files were found in the directory that you specified.\r\n"
			"There is nothing for Secure Archive to do.",
			gsTitleCaption, MB_ICONINFORMATION|MB_OK|MB_APPLMODAL);
		return PACK_RESULT_NOACTION;
	}

    DfltCode.Initialize(lpPwd->sPassword, lpPwd->iLength, true);

	//If the overwrite and add files options got signaled somehow, default to append. It's safer!
	if(lpMkPkgParam->iOptions &OPTION_OVERWRITE)
	{
		if(lpMkPkgParam->iOptions &OPTION_ADDFILES)
		{
			lpMkPkgParam->iOptions |= OPTION_OVERWRITE;
		}
	}

	if(!(lpMkPkgParam->iOptions &OPTION_OVERWRITE))
	{
		lpMkPkgParam->iOptions |= OPTION_ADDFILES;
	}
	else if(!(lpMkPkgParam->iOptions &OPTION_ADDFILES))
	{
		lpMkPkgParam->iOptions |= OPTION_OVERWRITE;
	}

	__int64 i64ExeEnd = GetExeEnd(lpMkPkgParam->sTarName);

	if((lpMkPkgParam->iOptions &OPTION_ADDFILES) && bTargetExist)
	{
		hfTarget = CreateFile(
			lpMkPkgParam->sTarName,
			GENERIC_READ|GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
	}
	else{
		hfTarget = CreateFile(
			lpMkPkgParam->sTarName,
			GENERIC_WRITE,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
	}

	if(hfTarget == INVALID_HANDLE_VALUE)
    {
		DfltCode.Destroy();

		sprintf_s(sStatusText, sizeof(sStatusText),
			"Failed to open write file:\r\n'%s'",
			lpMkPkgParam->sTarName);

        SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
        return PACK_RESULT_NOACTION;
    }

	if(gbUsePreAllocation)
	{
		char sPreAllocate[64];
		FileSizeFriendly(Pkg.i64TotalSize, sPreAllocate, sizeof(sPreAllocate));
		sprintf_s(sStatusText, sizeof(sStatusText),
				"Pre-allocating %s of disk space.", sPreAllocate);
		Set_Text(hPackingActionText, sStatusText);
		SetStatusFileName(hPackingStatusText, "This could take a few minutes.");
	}

	__int64 i64EOF = FileSeek(hfTarget, 0, FILE_END);
	FileSeek(hfTarget, 0, FILE_BEGIN);

	i64EndOfLastFile = i64EOF;

	__int64 i64PreAllocSz = Pkg.i64TotalSize
		+ (Pkg.dwFileCount * MAX_PATH)
		+ (Pkg.dwFileCount * 1024);

	if(!CheckFreeSpaceAndPreAllocFile(hProgressDialog, hfTarget, lpMkPkgParam->sTarName, i64PreAllocSz))
	{
		DfltCode.Destroy();

		CloseHandle(hfTarget);

		if(gbUsePreAllocation)
		{
			TruncateFile(lpMkPkgParam->sTarName, i64EOF + i64ExeEnd);
		}

		return PACK_RESULT_CANCEL;
	}

	//Read or Write the header.
	if((lpMkPkgParam->iOptions &OPTION_ADDFILES) && bTargetExist)
	{
		if(!ReadPackageHeader(i64ExeEnd, hfTarget, hProgressDialog, lpPwd, &DfltCode, &Header, false))
		{
			DfltCode.Destroy();
			CloseHandle(hfTarget);
			return PACK_RESULT_NOACTION;
		}

		ulPreExistingFiles = Header.dwFileCount;

		lpMkPkgParam->iPkgAttribs = Header.iPkgAttribs;
		giCompressLevel = Header.iCompressLevel;

		FileSeek(hfTarget, i64EOF, FILE_BEGIN);
	}
	else{
		Header.dwFileCount = Pkg.dwFileCount;
		Header.iPkgAttribs = lpMkPkgParam->iPkgAttribs;
		Header.iCompressLevel = giCompressLevel;

		if(!WritePackageHeader(i64ExeEnd, hfTarget, hProgressDialog, lpPwd, &DfltCode, &Header))
		{
			DfltCode.Destroy();
			CloseHandle(hfTarget);
			return PACK_RESULT_ERROR;
		}
	}

	//#PATCH(6/18/2008)
	//	If the compression level is zero, then were not compressing.
	if((Header.iPkgAttribs &ATTRIB_COMPRESSION) && Header.iCompressLevel <= 0)
	{
		Header.iPkgAttribs &= ~ATTRIB_COMPRESSION;
	}

    SendMessage(hProgressStatusBar, PBM_SETRANGE32, 0, (LPARAM) Pkg.dwFileCount);

    sSourceBuf = (unsigned char *) calloc(iBufferSize + 1, sizeof(char));
    sTargetBuf = (unsigned char *) calloc((iBufferSize + EXTRA_SIZE) + 1, sizeof(char));

    SendMessage(hProgressStatusBar, PBM_SETRANGE32, 0, (LPARAM) Pkg.dwFileCount);
    Set_Text(hPackingActionText, "Adding:");

	if(giCryptAlgo == CRYPT_ALGO_NASCCL)
	{
		memset(&SS, 0, sizeof(SS));
	
		SS.Initialize(lpPwd->sPassword, lpPwd->iLength, false);

		dwBlockSize = -1;
	}
	else if(giCryptAlgo == CRYPT_ALGO_RIJNDAEL)
	{
		memset(&Aes_Ctx, 0, sizeof(Aes_Ctx));
	
		aes_enc_key((unsigned char *)lpPwd->sPassword, lpPwd->iLength, &Aes_Ctx);
		dwBlockSize = 32;

		sBlockBuffer1 = (unsigned char *) calloc(dwBlockSize + 1, sizeof(unsigned char));
		sBlockBuffer2 = (unsigned char *) calloc(dwBlockSize + 1, sizeof(unsigned char));
	}
	else if(giCryptAlgo == CRYPT_ALGO_BLOWFISH)
	{
		memset(&BlowFish, 0, sizeof(BlowFish));
	
		Blowfish_Init(&BlowFish, (unsigned char *)lpPwd->sPassword, lpPwd->iLength);
		dwBlockSize = 8; //Two DWORDS's

		sBlockBuffer1 = (unsigned char *) calloc(dwBlockSize + 1, sizeof(unsigned char));
		sBlockBuffer2 = (unsigned char *) calloc(dwBlockSize + 1, sizeof(unsigned char));
	}

	while(ulFilesProcessed < Pkg.dwFileCount)
    {
		if(lpMkPkgParam->iPkgAttribs &ATTRIB_COMPRESSION)
		{
			memset(&zStream, 0, sizeof(zStream));
			if((iCompressResult = deflateInit(&zStream, Header.iCompressLevel)) != Z_OK)
			{
				ThrowFatalError("A call to DeflateInit() has failed");
			}
		}

		if(giCryptAlgo == CRYPT_ALGO_NASCCL)
		{
			SS.Reset();
		}

        if(gdwCurrentJobType == JOB_TYPE_CANCEL)
        {
			iActionResult = PACK_RESULT_CANCEL;
			break;
		}

		char *sRoot = NULL;

		if(lpMkPkgParam->iOptions &OPTION_USEALTFILEPACK)
		{
			sRoot = Pkg.Roots[ulFilesProcessed];
		}
		else{
			sRoot = lpMkPkgParam->sSrcFolder;
		}

		sprintf_s(sFullFileName, sizeof(sFullFileName), "%s%s", sRoot, Pkg.FileNames[ulFilesProcessed]);

		if(lpMkPkgParam->iPkgAttribs &ATTRIB_SAVEPATHS)
        {
			if(lpMkPkgParam->iOptions &OPTION_USEALTFILEPACK)
			{
				strcpy_s(sFileNameToSave, sizeof(sFileNameToSave), Pkg.FileNames[ulFilesProcessed]); 
			}
			else{
				GetLastDirectoryInPath(lpMkPkgParam->sSrcFolder, sFileNameToSave);
				strcat_s(sFileNameToSave, sizeof(sFileNameToSave), Pkg.FileNames[ulFilesProcessed]);
			}
        }
        else{
            _splitpath_s(Pkg.FileNames[ulFilesProcessed],
				NULL, 0,
				NULL, 0,
				sFileName, sizeof(sFileName),
				sExt, sizeof(sExt));

            _makepath_s(sFileNameToSave, sizeof(sFileNameToSave), NULL, NULL, sFileName, sExt);
        }

		SetStatusFileName(hPackingStatusText, Pkg.FileNames[ulFilesProcessed]);

		//Try to open the file while denying write access.
		hfSource = CreateFile(
            sFullFileName,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );
	
		//If deny write access fails, then we will attempt to open the file with read/write share.
		if(hfSource == INVALID_HANDLE_VALUE)
        {
			hfSource = CreateFile(
				sFullFileName,
				GENERIC_READ,
				FILE_SHARE_READ|FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				0,
				NULL
			);
			//FIXFIX: We should probably tell the user that this file could be corrupt!!
		}
		
		if(hfSource == INVALID_HANDLE_VALUE)
        {
			if(DoesPathContainUnicode(sFullFileName))
			{
				sprintf_s(sStatusText, sizeof(sStatusText),
					"A file name containing UNICODE characters cannot be encrypted.\r\n"
					" Would you like to skip this file and continue with the next one?\r\n\r\n\"%s\"",
					Pkg.FileNames[ulFilesProcessed]);
			}
			else{
	            sprintf_s(sStatusText, sizeof(sStatusText),
					"A file appears to be locked and cannot be encrypted.\r\n"
					" Would you like to skip this file and continue with the next one?\r\n\r\n\"%s\"",
					Pkg.FileNames[ulFilesProcessed]);
			}

            if(SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONQUESTION | MB_YESNO) == IDYES)
			{
				dwSkippedFiles++;
				ulFilesProcessed++;
				continue;
			}
			
			if((lpMkPkgParam->iOptions &OPTION_ADDFILES) && bTargetExist)
			{
				iActionResult = PACK_RESULT_OK;
			}
			else{
				iActionResult = PACK_RESULT_CANCEL;
			}

			if(!(lpMkPkgParam->iOptions &OPTION_USEALTFILEPACK))
			{
				fpPack.FreeMemory(&Pkg);
			}

            break;
        }

		memset(&SourceFileInfo, 0, sizeof(SourceFileInfo));
		GetFileInformationByHandle(hfSource, &SourceFileInfo);

		if(giCryptAlgo == CRYPT_ALGO_RIJNDAEL)
		{
			memcpy(&Aes_CtxPerFile, &Aes_Ctx, sizeof(Aes_Ctx));
		}

		fFileProgress = 0;
		i64TotalBytesRead = 0;

		SendMessage(hProgressFileStatusBar, PBM_SETPOS, 0, (LPARAM) 0);

		//-----------------------------------------------------------------------------------------------|
		//-----[BEGIN FILE HEADER]-----------------------------------------------------------------------|
		//-----------------------------------------------------------------------------------------------|

		//Write file name length.
		usFileNameLen = (short)strlen(sFileNameToSave);
		if(!SafeWrite(hProgressDialog, hfTarget, &usFileNameLen, sizeof(unsigned short)))
		{
			iActionResult = PACK_RESULT_ERROR;
			break;
		}

        //Write file name.
		DfltCode.Cipher(sFileNameToSave, sFileNameToSave, usFileNameLen);
		if(!SafeWrite(hProgressDialog, hfTarget, sFileNameToSave, usFileNameLen))
		{
			iActionResult = PACK_RESULT_ERROR;
			break;
		}

		//Write file size.
		liFileSize.HighPart = SourceFileInfo.nFileSizeHigh;
		liFileSize.LowPart  = SourceFileInfo.nFileSizeLow;
		i64FileSize = liFileSize.QuadPart;
        if(!SafeWrite(hProgressDialog, hfTarget, &i64FileSize, sizeof(i64FileSize)))
		{
			iActionResult = PACK_RESULT_ERROR;
			break;
		}

		//Write file attributes
		if(lpMkPkgParam->iPkgAttribs &ATTRIB_ATTRIBUTES)
        {
            if(!SafeWrite(hProgressDialog, hfTarget, &SourceFileInfo.dwFileAttributes, sizeof(SourceFileInfo.dwFileAttributes)))
			{
				iActionResult = PACK_RESULT_ERROR;
				break;
			}
        }

		//Write file creation and modification date / time.
		if(lpMkPkgParam->iPkgAttribs &ATTRIB_DATETIME)
        {
			if(!SafeWrite(hProgressDialog, hfTarget, &SourceFileInfo.ftCreationTime, sizeof(SourceFileInfo.ftCreationTime)))
			{
				iActionResult = PACK_RESULT_ERROR;
				break;
			}
            if(!SafeWrite(hProgressDialog, hfTarget, &SourceFileInfo.ftLastWriteTime, sizeof(SourceFileInfo.ftLastWriteTime)))
			{
				iActionResult = PACK_RESULT_ERROR;
				break;
			}
        }

		//Save this position, we will have to seek back to it to fill in the missing header information later.
		i64HeaderGapPos = FileSeek(hfTarget);

		//Make room for the compressed size, it will be written after the file is written.
		i64Temp = -1;
		if(!SafeWrite(hProgressDialog, hfTarget, &i64Temp, sizeof(i64Temp)))
		{
			iActionResult = PACK_RESULT_ERROR;
			break;
		}

		//Make room for the file CRC, it will be written after the file is written.
		if(lpMkPkgParam->iPkgAttribs &ATTRIB_CRCCHECK)
		{
			if(!SafeWrite(hProgressDialog, hfTarget, &dwTemp, sizeof(dwTemp)))
			{
				iActionResult = PACK_RESULT_ERROR;
				break;
			}
		}

		//Make room for the end file position, it will be written after the file is written.
		i64Temp = -1;
		if(!SafeWrite(hProgressDialog, hfTarget, &i64Temp, sizeof(__int64)))
		{
			iActionResult = PACK_RESULT_ERROR;
			break;
		}

		//End of header, save the position of the top of the data.
		i64TopOfFileDataPosition = FileSeek(hfTarget);
		//-----------------------------------------------------------------------------------------------|
		//-----[END FILE HEADER]-------------------------------------------------------------------------|
		//-----------------------------------------------------------------------------------------------|

		if(lpMkPkgParam->iPkgAttribs &ATTRIB_CRCCHECK)
        {
        	ulFinalCRC = 0xffffffff;
        }

        bool bCompression = true;

        do{
			if(gdwCurrentJobType == JOB_TYPE_CANCEL)
			{
				iActionResult = PACK_RESULT_CANCEL;
				break;
			}

            ReadFile(hfSource, sSourceBuf, iBufferSize, &dwBytesRead, NULL);
			i64TotalBytesRead += dwBytesRead;
			
			if(dwBytesRead != iBufferSize || i64TotalBytesRead == i64FileSize)
			{
				iFlush = Z_FINISH;
			}
			else{
				iFlush = Z_FULL_FLUSH;
			}

			fFileProgress = (float)(((float)i64TotalBytesRead / (float)i64FileSize) * 100.0f);

            if(lpMkPkgParam->iPkgAttribs &ATTRIB_CRCCHECK)
            {
                gCRC32->PartialCRC((unsigned int *)&ulFinalCRC, sSourceBuf, dwBytesRead);
            }

			if(lpMkPkgParam->iPkgAttribs &ATTRIB_COMPRESSION)
            {
				zStream.avail_in = dwBytesRead;

				zStream.avail_out = (iBufferSize + EXTRA_SIZE);
				zStream.next_in = (Bytef *) sSourceBuf;
				zStream.next_out = (Bytef *) sTargetBuf;

				iCompressResult = deflate(&zStream, iFlush);

				if(iCompressResult < Z_OK)
				{
					ThrowFatalError("A call to Deflate() has failed");
				}

				dwDeflSize = ((iBufferSize + EXTRA_SIZE) - zStream.avail_out);
            }
            else iCompressResult = -1;

			if(iCompressResult < 0)
            {
                memcpy(sTargetBuf, sSourceBuf, dwBytesRead);
                dwDeflSize = dwBytesRead;
                bCompression = false;
                if(!SafeWrite(hProgressDialog, hfTarget, &bCompression, sizeof(bool)))
				{
					iActionResult = PACK_RESULT_ERROR;
					break;
				}
            }
            else{
                bCompression = true;
                if(!SafeWrite(hProgressDialog, hfTarget, &bCompression, sizeof(bool)))
				{
					iActionResult = PACK_RESULT_ERROR;
					break;
				}
            }

            sTargetBuf[dwDeflSize] = '\0';

			if(!SafeWrite(hProgressDialog, hfTarget, &dwBytesRead, sizeof(DWORD)))
			{
				iActionResult = PACK_RESULT_ERROR;
				break;
			}
			if(!SafeWrite(hProgressDialog, hfTarget, &dwDeflSize, sizeof(DWORD)))
			{
				iActionResult = PACK_RESULT_ERROR;
				break;
			}

			if(giCryptAlgo == CRYPT_ALGO_NASCCL)
			{
				SS.Cipher((char *)sTargetBuf, dwDeflSize);
				if(!SafeWrite(hProgressDialog, hfTarget, sTargetBuf, dwDeflSize))
				{
					iActionResult = PACK_RESULT_ERROR;
					break;
				}
			}
			else if(giCryptAlgo == CRYPT_ALGO_RIJNDAEL) {
				
				DWORD dwBlockGap = dwBlockSize - (dwDeflSize % dwBlockSize);

				if(!SafeWrite(hProgressDialog, hfTarget, &dwBlockGap, sizeof(DWORD)))
				{
					iActionResult = PACK_RESULT_ERROR;
					break;
				}

				DWORD dwRPos = 0;
				DWORD dwWPos = 0;
				DWORD dwFill = 0;

				while(dwRPos < dwDeflSize)
				{
					dwWPos = 0;
					while(dwWPos < dwBlockSize && dwRPos < dwDeflSize)
						sBlockBuffer1[dwWPos++] = sTargetBuf[dwRPos++];

					dwFill = dwWPos;
					while(dwFill < dwBlockSize)
						sBlockBuffer1[dwFill++] = '*';
				
					aes_enc_blk((unsigned char *)sBlockBuffer1, (unsigned char *)sBlockBuffer2, &Aes_CtxPerFile);

					if(!SafeWrite(hProgressDialog, hfTarget, sBlockBuffer2, dwFill))
					{
						iActionResult = PACK_RESULT_ERROR;
						break;
					}
				}
			}
			else if(giCryptAlgo == CRYPT_ALGO_BLOWFISH) {
				DWORD dwBlockGap = dwBlockSize - (dwDeflSize % dwBlockSize);
				DWORD dwL = 0;
				DWORD dwR = 0;

				if(!SafeWrite(hProgressDialog, hfTarget, &dwBlockGap, sizeof(DWORD)))
				{
					iActionResult = PACK_RESULT_ERROR;
					break;
				}

				DWORD dwRPos = 0;
				DWORD dwWPos = 0;
				DWORD dwFill = 0;

				while(dwRPos < dwDeflSize)
				{
					dwWPos = 0;
				    while(dwWPos < sizeof(DWORD) && dwRPos < dwDeflSize)
						((char *)&dwL)[dwWPos++] = sTargetBuf[dwRPos++];

					dwFill = dwWPos;
					while(dwFill < sizeof(DWORD))
						((char *)&dwL)[dwFill++] = '0';
				
					dwWPos = 0;
					while(dwWPos < sizeof(DWORD) && dwRPos < dwDeflSize)
						((char *)&dwR)[dwWPos++] = sTargetBuf[dwRPos++];

					dwFill = dwWPos;
					while(dwFill < sizeof(DWORD))
						((char *)&dwR)[dwFill++] = '0';
				
					Blowfish_Encrypt(&BlowFish, &dwL, &dwR);

					if(!SafeWrite(hProgressDialog, hfTarget, &dwL, sizeof(DWORD)))
					{
						iActionResult = PACK_RESULT_ERROR;
						break;
					}
					if(!SafeWrite(hProgressDialog, hfTarget, &dwR, sizeof(DWORD)))
					{
						iActionResult = PACK_RESULT_ERROR;
						break;
					}
				}
			}
        } while(dwBytesRead == iBufferSize);

		if(iActionResult != PACK_RESULT_OK)
		{
	        CloseHandle(hfSource);
			break;
		}

		fFileProgress = 100;
		SendMessage(hProgressFileStatusBar, PBM_SETPOS, 100, (LPARAM) 0);

		if(lpMkPkgParam->iPkgAttribs &ATTRIB_CRCCHECK)
        {
            ulFinalCRC = (ulFinalCRC ^ 0xffffffff);
        }

		//-----------------------------------------------------------------------------------------------|
		//-----[FILL IN MISSING HEADER INFORMATION BEGIN]------------------------------------------------|
		//-----------------------------------------------------------------------------------------------|
		
		//We need to write some more information to the header. Save where we are.
		i64EndOfLastFile = FileSeek(hfTarget); //The position of the last completed file.

		//Then, seek back to the top of this file.
		FileSeek(hfTarget, i64HeaderGapPos, FILE_BEGIN);

		//Write the compressed file size.
		i64CompressedFileSize = (i64EndOfLastFile - i64TopOfFileDataPosition);
		if(!SafeWrite(hProgressDialog, hfTarget, &i64CompressedFileSize, sizeof(i64CompressedFileSize)))
		{
			iActionResult = PACK_RESULT_ERROR;
			break;
		}

		//Write the file CRC.
		if(lpMkPkgParam->iPkgAttribs &ATTRIB_CRCCHECK)
        {
            if(!SafeWrite(hProgressDialog, hfTarget, &ulFinalCRC, sizeof(DWORD)))
			{
				iActionResult = PACK_RESULT_ERROR;
				break;
			}
        }

		//12/10/2008 - Fixed bug which was causing data corruption when adding files to self-extractors.
		__int64 i64EOFPosition = i64EndOfLastFile - i64ExeEnd;

		//Write the End-of-File position.
		if(!SafeWrite(hProgressDialog, hfTarget, &i64EOFPosition, sizeof(i64EOFPosition)))
		{
			iActionResult = PACK_RESULT_ERROR;
			break;
		}

		//Seek back to the end of the file, but we can't
		//	use FILE_END because we might be using pre-allocation.
		FileSeek(hfTarget, i64EndOfLastFile, FILE_BEGIN);

		//-----------------------------------------------------------------------------------------------|
		//-----[FILL IN MISSING HEADER INFORMATION END]--------------------------------------------------|
		//-----------------------------------------------------------------------------------------------|

		if(lpMkPkgParam->iPkgAttribs &ATTRIB_COMPRESSION)
		{
			deflateEnd(&zStream);
		}

        CloseHandle(hfSource);

        ulFilesProcessed++;

        fPackageProgress = (float)ulFilesProcessed;
	} //----------End Main Loop----------

	UpdateFileCount(i64ExeEnd, hfTarget, &DfltCode, ulPreExistingFiles + (ulFilesProcessed - dwSkippedFiles));

    free(sSourceBuf);
    free(sTargetBuf);

	//Free memory for the RIJNDAEL algo.
	if(giCryptAlgo == CRYPT_ALGO_RIJNDAEL)
	{
		free(sBlockBuffer1);
		free(sBlockBuffer2);
	}
	else if(giCryptAlgo == CRYPT_ALGO_NASCCL){
		//Free memory used by the simple algo.
		SS.Destroy();
	}

	DfltCode.Destroy();

	CloseHandle(hfTarget);

	//Delete archives with no files.
	if((ulPreExistingFiles + (ulFilesProcessed - dwSkippedFiles)) <= 0)
	{
		SDeleteFile(gsWorkingOutput);
	}
	else{
		//6/18/2008 - If the file didn't exist to begin with, delete it on error or cancel.
		if(iActionResult != PACK_RESULT_OK && !bTargetExist)
		{
			DeleteFile(lpMkPkgParam->sTarName);
			//No need to securely delete encrypted data.
			//SDeleteFile(lpMkPkgParam->sTarName);
		}
		else{
			//12/10/2008 - Fixed bug which was causing data corruption when adding files to self-extractors.
			TruncateFile(lpMkPkgParam->sTarName, i64EndOfLastFile);
		}
	}

	//11/28/2010 - Implemented the ability to delete source files upon encryption.
	if(lpMkPkgParam->iOptions &OPTION_DELETESOURCE)
	{
		if(iActionResult == PACK_RESULT_OK)
		{
			if(dwSkippedFiles == 0)
			{
				SecurelyDeleteSourceFiles(lpMkPkgParam->sSrcFolder, &Pkg, (lpMkPkgParam->iOptions &OPTION_USEALTFILEPACK) > 0);
			}
			else {
				SafeMsgBox(hProgressDialog,
					"The original files were not automatically deleted because at least one file was skipped during the encryption process.",
					gsTitleCaption, MB_ICONINFORMATION);
			}
		}

		if(!(lpMkPkgParam->iOptions &OPTION_USEALTFILEPACK))
		{
			fpPack.FreeMemory(&Pkg);
		}
	}

	return iActionResult;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
bool SecurelyDeleteSourceFiles(const char *sRoot, FILEPACKAGE *Pkg, bool bUseAlternateRoot)
{
    char sFullFileName[MAX_PATH + 1];
	bool bResult = true;

    SendMessage(hProgressStatusBar, PBM_SETRANGE32, 0, (LPARAM) Pkg->dwFileCount);
    Set_TextF(hPackingActionText, "%s Deleting %d files in %d folders",
		gdwSecureDeletePasses > 0 ? "Securely " : "", Pkg->dwFileCount, Pkg->dwFolderCount);

	for(DWORD dwFile = 0; dwFile < Pkg->dwFileCount; dwFile++)
	{
		sprintf_s(sFullFileName, sizeof(sFullFileName), "%s%s", Pkg->Roots[dwFile], Pkg->FileNames[dwFile]);
		bResult = SDeleteFile(sFullFileName, gdwSecureDeletePasses > 0 ? gdwSecureDeletePasses : 1) && bResult;
		fPackageProgress = (float)dwFile;
	}

	if(bUseAlternateRoot)
	{
		char sFullPath[MAX_PATH];

		for(DWORD dwFile = 0 ; dwFile < Pkg->dwFileCount; dwFile++)
		{
			size_t iFullPathLength = sprintf_s(sFullPath, sizeof(sFullPath), "%s%s", Pkg->Roots[dwFile], Pkg->FileNames[dwFile]);
			size_t iRootLength = strlen(Pkg->Roots[dwFile]);

			size_t iSlashPos = (int)LastIndexOf(sFullPath, (int)iFullPathLength, '\\');

			while(iSlashPos > iRootLength)
			{
				sFullPath[iSlashPos] = '\0';
				RemoveDirectory(sFullPath);
				iSlashPos = (int)LastIndexOf(sFullPath, (int)iSlashPos, '\\');
			}
		}
	}
	else {
		SDeleteFile(sRoot, gdwSecureDeletePasses > 0 ? gdwSecureDeletePasses : 1);
	}

	return bResult;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
__inline bool SafeWrite(HANDLE hFile, VOID *lpData, DWORD dwSize)
{
	DWORD dwBytesWritten = 0;

	if(WriteFile(hFile, lpData, dwSize, &dwBytesWritten, NULL))
	{
		if(dwBytesWritten == dwSize)
		{
			return true;
		}
	}

	CloseHandle(hFile);

	return false;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

__inline bool SafeWrite(HWND hWnd, HANDLE hFile, VOID *lpData, DWORD dwSize)
{
	DWORD dwBytesWritten = 0;

	if(WriteFile(hFile, lpData, dwSize, &dwBytesWritten, NULL))
	{
		if(dwBytesWritten == dwSize)
		{
			return true;
		}
	}

	SafeMsgBox(hWnd,
		"The destination disk has unexpectedly run out of space.\r\n"
		"The destination file will be removed!",
		gsTitleCaption, MB_ICONERROR | MB_SYSTEMMODAL);

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
__inline bool UpdateFileCount(__int64 i64ExeEnd, HANDLE hSource, NASCCLStream *lpCode, DWORD dwFileCount)
{
	DWORD dwEncryptedFC = dwFileCount;

	//Save the current file position.
	__int64 i64CurrentPos = FileSeek(hSource);

	//Skip past all of the header data that we dont want to overwrite.
	FileSeek(hSource, ((__int64)FILE_COUNT_POS) + i64ExeEnd, FILE_BEGIN);

	//Write the new file count.
	lpCode->Cipher(&dwEncryptedFC, &dwEncryptedFC, sizeof(dwEncryptedFC));
	if(!SafeWrite(hSource, &dwEncryptedFC, sizeof(dwEncryptedFC)))
	{
		return false;
	}

	//Reset the file position.
	FileSeek(hSource, i64CurrentPos, FILE_BEGIN);

	return true;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
__inline bool ShouldFileBeDeleted(GRIDDELETEITEMS *pGDI, __int64 i64Position, const char *sFileName)
{
	for(unsigned long ilItem = 0; ilItem < pGDI->ulCount; ilItem++)
	{
		if(pGDI->Items[ilItem].i64FilePosition == i64Position)
		{
			char sBuf[1024];
			char sFile[MAX_PATH];

			ListView_GetItemText(pGDI->hGrid,
				pGDI->Items[ilItem].ulGridPosition,
				LIST_POS_NAME, sBuf, sizeof(sBuf));

			if(GetFileName(sFileName, sFile, sizeof(sFile)))
			{
				return(_strcmpi(sFile, sBuf) == 0);
			}
		}
	}

	return false;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
bool CreateSelfExtractor(const char *sSourceFile, const char *sTargetFile)
{
    char sStatusText[5120];

	HANDLE hfSource = NULL;
    HANDLE hfTarget = NULL;

	__int64 i64FileSize = 0;
	__int64 i64BytesCopied = 0;

	DWORD dwBytesRead = 0;

	char sSelfExtractor[MAX_PATH];
	sprintf_s(sSelfExtractor, sizeof(sSelfExtractor), "%s\\SASE.exe", gsPath);
	if(!CopyFile(sSelfExtractor, sTargetFile, FALSE))
	{
        sprintf_s(sStatusText, sizeof(sStatusText),
			"Failed to copy executable template to file:\r\n'%s'.", sTargetFile);
	    SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
        return false;
	}

	hfSource = CreateFile(
        sSourceFile,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if(hfSource == INVALID_HANDLE_VALUE)
    {
        sprintf_s(sStatusText, sizeof(sStatusText), "Failed to open read file:\r\n'%s'.", sSourceFile);
	    SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
        return false;
    }

	hfTarget = CreateFile(
		sTargetFile,
		GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
    if(hfTarget == INVALID_HANDLE_VALUE)
    {
        sprintf_s(sStatusText, sizeof(sStatusText), "Failed to open write file:\r\n'%s'.", sTargetFile);
	    SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
		CloseHandle(hfSource);
        return false;
    }

	if(!FileSize(hfSource, &i64FileSize))
	{
        sprintf_s(sStatusText, sizeof(sStatusText), "Failed to get size of file:\r\n'%s'.", sSourceFile);
	    SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
		CloseHandle(hfSource);
		CloseHandle(hfTarget);
	}

	SetStatusFileName(hPackingActionText, "Merging encrypted package with executable.");
	SetStatusFileName(hPackingStatusText, sSourceFile);

	SendMessage(hProgressStatusBar, PBM_SETRANGE32, 0, (LPARAM) 100);
	SendMessage(hProgressFileStatusBar, PBM_SETRANGE32, 0, (LPARAM) 100);
	SendMessage(hProgressStatusBar, PBM_SETPOS, 0, (LPARAM) 0);
	SendMessage(hProgressFileStatusBar, PBM_SETPOS, 0, (LPARAM) 0);

	unsigned char *sDataBuf = (unsigned char *) calloc(DEFAULT_BUF_SZ + 1, sizeof(char));

	FileSeek(hfTarget, 0, FILE_END); //Move to the end of the file.

	bool bResult = true;

	do{
		ReadFile(hfSource, sDataBuf, DEFAULT_BUF_SZ, &dwBytesRead, NULL);
		if(!SafeWrite(hProgressDialog, hfTarget, sDataBuf, dwBytesRead))
		{
			bResult = false;
			break;
		}

		i64BytesCopied += dwBytesRead;

		fFileProgress = ((float)((((double)i64BytesCopied) / ((double)i64FileSize)) * 100.0f));
		fPackageProgress = fFileProgress;

		if(gdwCurrentJobType == JOB_TYPE_CANCEL)
        {
            Sleep(1000);
			bResult = true;
            break;
        }
	} while(dwBytesRead == DEFAULT_BUF_SZ);

	CloseHandle(hfSource);
	CloseHandle(hfTarget);
	free(sDataBuf);

	return bResult;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
int DeleteFilesFromPackage(char *sSrcName, char *sTempPath, LPPASSWORD lpPwd,
							GRIDDELETEITEMS *pGDI, DWORD dwFileCount)
{
    char sPartFileName[MAX_PATH + 1];
    char sStatusText[5120];
	char sStatName[MAX_PATH];
	char sStatExt[MAX_PATH];

    int ilvItemPos = -1;
	int iBytesToRead = 0;
	int iActionResult = PACK_RESULT_OK;

	__int64 i64CompressedFileSize = 0;
	__int64 i64FileEndPos = 0;
    __int64 i64FileSize = 0;
	__int64 i64Temp = -1;
	__int64 i64HeaderGapPos = 0;
	__int64 i64EndOfLastFile = 0;
    __int64 iFileTopPosition = 0;

    DWORD dwInflSize = 0;
    DWORD dwOrigSize = 0;
	DWORD dwBytesRead = 0;
    DWORD dwFileNumber = 0;
	DWORD dwFileAttributes = 0;
	DWORD dwDeletedCount = 0;

	HANDLE hfSource = NULL;
    HANDLE hfTarget = NULL;

	FILETIME ftCreationTime;
    FILETIME ftLastWriteTime;

    unsigned long ulReadCRC = 0;

	unsigned short usFileNameLen = 0;

	NASCCLStream DfltCode;
	HEADER Header;

    memset(&DfltCode, 0, sizeof(DfltCode));
    memset(&Header, 0, sizeof(Header));

    unsigned char *sDataBuf = (unsigned char *) calloc(DEFAULT_BUF_SZ + 1, sizeof(char));

	DfltCode.Initialize(lpPwd->sPassword, lpPwd->iLength, true);

	__int64 i64ExeEnd = GetExeEnd(sSrcName);

	hfSource = CreateFile(
        sSrcName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if(hfSource == INVALID_HANDLE_VALUE)
    {
        sprintf_s(sStatusText, sizeof(sStatusText), "Failed to open read file:\r\n'%s'.", sSrcName);
	    SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
		DfltCode.Destroy();
		free(sDataBuf);
        return PACK_RESULT_CANCEL;
    }

	hfTarget = CreateFile(
		sTempPath,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_HIDDEN,
		NULL
	);
    if(hfTarget == INVALID_HANDLE_VALUE)
    {
        sprintf_s(sStatusText, sizeof(sStatusText), "Failed to open temp file:\r\n'%s'.", sTempPath);
	    SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
		DfltCode.Destroy();
		free(sDataBuf);
		CloseHandle(hfSource);
        return PACK_RESULT_CANCEL;
    }

	//Copy header to the new archive.
	ReadFile(hfSource, sDataBuf, PACKAGE_HEADER_LEN + (unsigned long)i64ExeEnd, &dwBytesRead, NULL);
	if(!SafeWrite(hProgressDialog, hfTarget, sDataBuf, dwBytesRead))
	{
		DfltCode.Destroy();
		CloseHandle(hfSource);
		CloseHandle(hfTarget);
		free(sDataBuf);
		return PACK_RESULT_CANCEL;
	}

	if(!ReadPackageHeader(i64ExeEnd, hfSource, hProgressDialog, lpPwd, &DfltCode, &Header, false))
	{
		DfltCode.Destroy();
		CloseHandle(hfSource);
		CloseHandle(hfTarget);
		free(sDataBuf);
		return PACK_RESULT_CANCEL;
	}

	if(dwFileCount >= Header.dwFileCount)
	{
		SafeMsgBox(hProgressDialog,
			"You cannot delete all items from an archive.",
			gsTitleCaption, MB_ICONINFORMATION|MB_OK|MB_APPLMODAL);

		DfltCode.Destroy();
		CloseHandle(hfSource);
		CloseHandle(hfTarget);
		free(sDataBuf);
		return PACK_RESULT_CANCEL;
	}

    fPackageProgress = 0;

	//--------------------------------------------------------------------------------

    Set_Text(hPackingActionText, "Repacking:");

	SetStatusFileName(hPackingStatusText, "Please wait...");
	SendMessage(hProgressStatusBar, PBM_SETRANGE32, 0, (LPARAM) Header.dwFileCount);
	SendMessage(hProgressFileStatusBar, PBM_SETRANGE32, 0, (LPARAM) 100);
	SendMessage(hProgressFileStatusBar, PBM_SETPOS, 0, (LPARAM) 0);

	while(dwFileNumber < Header.dwFileCount)
	{
		if(i64CompressedFileSize > DEFAULT_BUF_SZ) //This reduces status bar flashing.
		{
			SendMessage(hProgressFileStatusBar, PBM_SETPOS, 0, (LPARAM) 0);
		}

		if(gdwCurrentJobType == JOB_TYPE_CANCEL)
        {
            Sleep(1000);
            break;
        }

		bool bSkipFile = false;

		iFileTopPosition = FileSeek(hfSource);

		//-----------------------------------------------------------------------------------------------|
		//-----[BEGIN READ FILE HEADER]------------------------------------------------------------------|
		//-----------------------------------------------------------------------------------------------|

		//Read the file name length.
		ReadFile(hfSource, &usFileNameLen, sizeof(unsigned short), &dwBytesRead, NULL);
		if(dwBytesRead == 0)
		{
			//This is where this loop ends.
			iActionResult = PACK_RESULT_OK;
			break;
		}
		if(usFileNameLen <= 0 || usFileNameLen >= MAX_PATH) //Sanity check.
		{
			sprintf_s(sStatusText, sizeof(sStatusText),
				"Corrupt data file detected. Error code: (DeleteFilesFromPackage->usFileNameLen:%d|%d)", __LINE__, usFileNameLen);

			SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

			iActionResult = PACK_RESULT_CANCEL;
			break;
		}

		//Read (and decode) the file name.
		ReadFile(hfSource, sPartFileName, usFileNameLen, &dwBytesRead, NULL);
		DfltCode.Cipher(sPartFileName, sPartFileName, usFileNameLen);
		sPartFileName[usFileNameLen] = '\0';

		//Read the file size.
		ReadFile(hfSource, &i64FileSize, sizeof(i64FileSize), &dwBytesRead, NULL);

		//Read file attributes
		if(Header.iPkgAttribs &ATTRIB_ATTRIBUTES)
        {
            ReadFile(hfSource, &dwFileAttributes, sizeof(dwFileAttributes), &dwBytesRead, NULL);
        }

		//Read the creation and last modified date / time.
		if(Header.iPkgAttribs &ATTRIB_DATETIME)
		{
			ReadFile(hfSource, &ftCreationTime, sizeof(FILETIME), &dwBytesRead, NULL);
			ReadFile(hfSource, &ftLastWriteTime, sizeof(FILETIME), &dwBytesRead, NULL);
		}

		//Read the compressed file size.
		ReadFile(hfSource, &i64CompressedFileSize, sizeof(i64CompressedFileSize), &dwBytesRead, NULL);
		if(i64CompressedFileSize == -1) //Sanity check.
		{
			sprintf_s(sStatusText, sizeof(sStatusText),
				"Corrupt data file detected. Error code: (DeleteFilesFromPackage->i64CompressedFileSize:%d|%lld)",
				__LINE__, i64CompressedFileSize);

			SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

			DfltCode.Destroy();
			free(sDataBuf);

			iActionResult = PACK_RESULT_CANCEL;
			break;
		}
		
		if(Header.iPkgAttribs &ATTRIB_CRCCHECK)
		{
			ReadFile(hfSource, &ulReadCRC, sizeof(unsigned long), &dwBytesRead, NULL);
		}

		//Read the end-of-file position.
		ReadFile(hfSource, &i64FileEndPos, sizeof(i64FileEndPos), &dwBytesRead, NULL);
		if(i64FileEndPos == -1) //Sanity check.
		{
			sprintf_s(sStatusText, sizeof(sStatusText),
				"Corrupt data file detected. Error code: (DeleteFilesFromPackage->i64FileEndPos:%d|%lld)",
				__LINE__, i64FileEndPos);

			SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

			DfltCode.Destroy();
			free(sDataBuf);

			iActionResult = PACK_RESULT_CANCEL;
			break;
		}

		//-----------------------------------------------------------------------------------------------|
		//-----[END READ FILE HEADER]------------------------------------------------------------------|
		//-----------------------------------------------------------------------------------------------|

		if(ShouldFileBeDeleted(pGDI, iFileTopPosition, sPartFileName))
		{
			//Skip this file, it needs to be excluded from the resulting package.
			bSkipFile = true;
		}

		if(!bSkipFile)
		{
            _splitpath_s(
				sPartFileName,
				NULL, 0,
				NULL, 0,
				sStatName, sizeof(sStatName),
				sStatExt, sizeof(sStatExt)
			);

			sprintf_s(sStatusText, "%s%s", sStatName, sStatExt);
			SetStatusFileName(hPackingStatusText, sStatusText);

			//-----------------------------------------------------------------------------------------------|
			//-----[BEGIN FILE HEADER]-----------------------------------------------------------------------|
			//-----------------------------------------------------------------------------------------------|

			//Write file name length.
			if(!SafeWrite(hProgressDialog, hfTarget, &usFileNameLen, sizeof(unsigned short)))
			{
				iActionResult = PACK_RESULT_CANCEL;
				break;
			}

			//Write file name.
			DfltCode.Cipher(sPartFileName, sPartFileName, usFileNameLen);
			if(!SafeWrite(hProgressDialog, hfTarget, sPartFileName, usFileNameLen))
			{
				iActionResult = PACK_RESULT_CANCEL;
				break;
			}

			//Write file size.
			if(!SafeWrite(hProgressDialog, hfTarget, &i64FileSize, sizeof(i64FileSize)))
			{
				iActionResult = PACK_RESULT_CANCEL;
				break;
			}

			//Write file attributes
			if(Header.iPkgAttribs &ATTRIB_ATTRIBUTES)
			{
				if(!SafeWrite(hProgressDialog, hfTarget, &dwFileAttributes, sizeof(dwFileAttributes)))
				{
					iActionResult = PACK_RESULT_CANCEL;
					break;
				}
			}

			//Write file creation and modification date / time.
			if(Header.iPkgAttribs &ATTRIB_DATETIME)
			{
				if(!SafeWrite(hProgressDialog, hfTarget, &ftCreationTime, sizeof(FILETIME)))
				{
					iActionResult = PACK_RESULT_CANCEL;
					break;
				}
				if(!SafeWrite(hProgressDialog, hfTarget, &ftLastWriteTime, sizeof(FILETIME)))
				{
					iActionResult = PACK_RESULT_CANCEL;
					break;
				}
			}

			//Write the compressed file size.
			if(!SafeWrite(hProgressDialog, hfTarget, &i64CompressedFileSize, sizeof(i64CompressedFileSize)))
			{
				iActionResult = PACK_RESULT_CANCEL;
				break;
			}

			//Make room for the file CRC, it will be written after the file is written.
			if(Header.iPkgAttribs &ATTRIB_CRCCHECK)
			{
				if(!SafeWrite(hProgressDialog, hfTarget, &ulReadCRC, sizeof(ulReadCRC)))
				{
					iActionResult = PACK_RESULT_CANCEL;
					break;
				}
			}

			//Save this position, we will have to seek back to it to fill in the missing header information later.
			i64HeaderGapPos = FileSeek(hfTarget);

			//Make room for the end file position, it will be written after the file is written.
			i64Temp = -1;
			if(!SafeWrite(hProgressDialog, hfTarget, &i64Temp, sizeof(__int64)))
			{
				iActionResult = PACK_RESULT_CANCEL;
				break;
			}

			//End of header, save the position of the top of the data.
			//i64TopOfFileDataPosition = FileSeek(hfTarget);
			//-----------------------------------------------------------------------------------------------|
			//-----[END FILE HEADER]-------------------------------------------------------------------------|
			//-----------------------------------------------------------------------------------------------|

			__int64 i64BytesRemaining = i64CompressedFileSize;

			do{
				if(i64BytesRemaining > DEFAULT_BUF_SZ)
				{
					iBytesToRead = DEFAULT_BUF_SZ;
				}
				else{
					iBytesToRead = (int)i64BytesRemaining;
				}

				ReadFile(hfSource, sDataBuf, iBytesToRead, &dwBytesRead, NULL);
				if(!SafeWrite(hProgressDialog, hfTarget, sDataBuf, iBytesToRead))
				{
					iActionResult = PACK_RESULT_CANCEL;
					break;
				}

				i64BytesRemaining = i64BytesRemaining - iBytesToRead;

				if(gdwCurrentJobType == JOB_TYPE_CANCEL)
				{
					Sleep(1000);

					iActionResult = PACK_RESULT_CANCEL;
					break;
				}

				fFileProgress = 100.0f - ((float)((((float)i64BytesRemaining) / ((float)i64CompressedFileSize)) * 100.0f));

			}while(i64BytesRemaining > 0);

			if(iActionResult != PACK_RESULT_OK)
			{
				break;
			}

			fFileProgress = 100;
			SendMessage(hProgressFileStatusBar, PBM_SETPOS, 100, (LPARAM) 0);

			//We need to write some more information to the header. Save where we are.
			i64EndOfLastFile = FileSeek(hfTarget);

			//Then, seek back to the top of this file.
			FileSeek(hfTarget, i64HeaderGapPos, FILE_BEGIN);

			//Write the End-of-File position.
			//12/10/2008 - Fixed bug which was causing data corruption when adding files to self-extractors.
			__int64 i64EOFPosition = i64EndOfLastFile - i64ExeEnd;
			if(!SafeWrite(hProgressDialog, hfTarget, &i64EOFPosition, sizeof(i64EOFPosition)))
			{
				iActionResult = PACK_RESULT_CANCEL;
				break;
			}

			//Seek back to the end of the file.
			FileSeek(hfTarget, i64EndOfLastFile, FILE_BEGIN);
		}
		else{
			dwDeletedCount++;
		}

		//Skip File.
		FileSeek(hfSource, i64FileEndPos + i64ExeEnd, FILE_BEGIN);

        fPackageProgress++;

		dwFileNumber++;
	}

    //--------------------------------------------------------------------------------

	if(dwDeletedCount > Header.dwFileCount)
	{
		SafeMsgBox(hProgressDialog,
			"The delete process has been cancelled to prevent corruption.",
			gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

		iActionResult = PACK_RESULT_CANCEL;
	}

	if(iActionResult != PACK_RESULT_CANCEL)
	{
		UpdateFileCount(i64ExeEnd, hfTarget, &DfltCode, Header.dwFileCount - dwDeletedCount);
	}

    CloseHandle(hfSource);
    CloseHandle(hfTarget);

	SetFileAttributes(sTempPath, FILE_ATTRIBUTE_NORMAL);

	if(iActionResult != PACK_RESULT_CANCEL)
	{
		SetFileAttributes(sSrcName, FILE_ATTRIBUTE_NORMAL);

		if(DeleteFile(sSrcName))
		{
			if(!MoveFile(sTempPath, sSrcName))
			{
				SafeMsgBox(hProgressDialog, "Failed to replace the original archive.",
					gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
			}
		}
		else{
			SafeMsgBox(hProgressDialog, "Failed to delete the original archive.",
				gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);		
		}

		SetFileAttributes(sSrcName, FILE_ATTRIBUTE_NORMAL);
	}

	DfltCode.Destroy();
	free(sDataBuf);

    return iActionResult;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
bool CheckFreeSpaceAndPreAllocFile(HWND hWnd,
								   HANDLE hFile,
								   const char *sFileName,
								   unsigned __int64 i64MinSize)
{
	DWORD dwWritten = 0;
	char sPath[MAX_PATH];

	//Obtain the current file pointer.
	__int64 iCurrentPos = FileSeek(hFile, 0, FILE_CURRENT);
	__int64 iCurrentSize = FileSeek(hFile, 0, FILE_END);

	if(!GetFilePath(sFileName, sPath, sizeof(sPath)))
	{
		SafeMsgBox(hWnd,
			"Failed to obtain the name of the destination drive.\r\n"
			" Choose a different destination disk.",
			gsTitleCaption, MB_ICONERROR | MB_SYSTEMMODAL);
		return false;
	}

	ULARGE_INTEGER uliBytesFree;
	memset(&uliBytesFree, 0, sizeof(uliBytesFree));
	if(!GetDiskFreeSpaceEx(sPath, &uliBytesFree, NULL, NULL))
	{
		SafeMsgBox(hWnd,
			"Failed to obtain the amount of free disk space on the destination drive.\r\n"
			" Choose a different destination disk.",
			gsTitleCaption, MB_ICONERROR | MB_SYSTEMMODAL);
		return false;
	}

	if(i64MinSize + PREALLOCATION_SLOP >= uliBytesFree.QuadPart)
	{
		if(gbIgnoreFreeSpaceWarning)
		{
			if(SafeMsgBox(hWnd,
				"The destination does not have sufficient disk space to\n"
				"safely archive the selected files. Would you like to\n"
				"continue anyway?\n\nData could be lost durring this process!",
				gsTitleCaption, MB_ICONWARNING | MB_SYSTEMMODAL |MB_YESNO) != IDYES)
			{
				return false;
			}
		}
		else {
			SafeMsgBox(hWnd,
				"The destination does not have sufficient diskspace to\n"
				" safely archive the selected files. Free some space or\r\n"
				" choose a different destination disk.",
				gsTitleCaption, MB_ICONERROR | MB_SYSTEMMODAL);
			return false;
		}
	}

	if(gbUsePreAllocation)
	{
		//Set the file pointer to the desired location.
		FileSeek(hFile, iCurrentSize + i64MinSize + PREALLOCATION_SLOP, FILE_BEGIN);

		//Allocate the file clusters.
		if(!SetEndOfFile(hFile))
		{
			SafeMsgBox(hWnd,
				"Failed to preallocate enough disk space to safely archive the selected files.\r\n"
				"Free some space or choose a different destination disk.",
				gsTitleCaption, MB_ICONERROR | MB_SYSTEMMODAL);
			return false;
		}
	}

	//Reset the file pointer.
	FileSeek(hFile, iCurrentPos, FILE_BEGIN);

	return true;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int BreakPackage(char *sSrcName, char *TarFolder, LPPASSWORD lpPwd, bool bTestOnly)
{
    char *sBlockBuffer1 = NULL;
    char *sBlockBuffer2 = NULL;

	char sFullFileName[MAX_PATH + 1];
    char sPartFileName[MAX_PATH + 1];
    char sStatusText[5120];
    char sTempBuf[1024];

	__int64 i64TotalBytesRead = 0;
    __int64 i64CompressedFileSize = 0;
	__int64 i64FileEndPos = 0;
    __int64 i64FileSize = 0;

    DWORD dwInflSize = 0;
    DWORD dwOrigSize = 0;
	DWORD dwCompatVersion = 0;
	DWORD dwBytesRead = 0;
    DWORD dwDeflSize = 0;
    DWORD dwBlockSize = -1;
	DWORD dwFileAttributes = 0;

	FILETIME ftCreationTime;
    FILETIME ftLastWriteTime;

	HANDLE hfSource = NULL;
    HANDLE hfTarget = NULL;

	int iCompressResult = 0;
    int iBufferSize = DEFAULT_BUF_SZ;
    int iTemp = 0;
    DWORD dwCountOfFilesToExtract = 0;
    int iLoopFileCount = 0;
	int iActionResult = PACK_RESULT_OK;
	int iTestResult = PACK_RESULT_OK;

    long lReadChecksum = 0;

	bool bOverwriteAll = gbOverwriteDlft || gbOverwrite;

	unsigned char *sSourceBuf = NULL;
    unsigned char *sTargetBuf = NULL;

	unsigned long ulFinalCRC = 0;
    unsigned long ulLoopCount = 0;
    unsigned long ulReadCRC = 0;

	unsigned short usFileNameLen = 0;

	NASCCLStream SS;
	NASCCLStream DfltCode;
	BlowFishCTX BlowFish;
	aes_ctx Aes_Ctx;
	aes_ctx Aes_CtxPerFile;
	z_stream zStream;
	HEADER Header;

    memset(&DfltCode, 0, sizeof(DfltCode));
    memset(sTempBuf, 0, sizeof(sTempBuf));
    memset(&Header, 0, sizeof(Header));

	DfltCode.Initialize(lpPwd->sPassword, lpPwd->iLength, true);

	__int64 i64ExeEnd = GetExeEnd(sSrcName);

	hfSource = CreateFile(
        sSrcName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if(hfSource == INVALID_HANDLE_VALUE)
    {
        sprintf_s(sStatusText, sizeof(sStatusText), "Failed to open read file:\r\n'%s'.", sSrcName);
	    SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
		DfltCode.Destroy();
		iTestResult = PACK_RESULT_CANCEL;
        return false;
    }

	if(!ReadPackageHeader(i64ExeEnd, hfSource, hProgressDialog, lpPwd, &DfltCode, &Header, false))
	{
		DfltCode.Destroy();
		CloseHandle(hfSource);
		iTestResult = PACK_RESULT_ERROR;
		return false;
	}

#ifndef _SELFEXTRACTOR_APP
	if(gdwCurrentJobType == JOB_TYPE_EXTRACT_SELECTED)
	{
		dwCountOfFilesToExtract = ListView_GetSelectedCount(ghFileList);
		SendMessage(hProgressStatusBar, PBM_SETRANGE32, 0, (LPARAM) dwCountOfFilesToExtract);
	}
	else{
#endif
		dwCountOfFilesToExtract = Header.dwFileCount;
		SendMessage(hProgressStatusBar, PBM_SETRANGE32, 0, (LPARAM) dwCountOfFilesToExtract);
#ifndef _SELFEXTRACTOR_APP
	}
#endif

    fPackageProgress = 0;

    sSourceBuf = (unsigned char *) calloc((iBufferSize + EXTRA_SIZE) + 1, sizeof(char));
    sTargetBuf = (unsigned char *) calloc((iBufferSize + EXTRA_SIZE) + 1, sizeof(char));

	if(giCryptAlgo == CRYPT_ALGO_NASCCL)
	{
		memset(&SS, 0, sizeof(SS));
		SS.Initialize(lpPwd->sPassword, lpPwd->iLength, false);
		dwBlockSize = -1;
	}
	else if(giCryptAlgo == CRYPT_ALGO_RIJNDAEL)
	{
		memset(&Aes_Ctx, 0, sizeof(Aes_Ctx));
	
		aes_dec_key((unsigned char *)lpPwd->sPassword, lpPwd->iLength, &Aes_Ctx);
		dwBlockSize = 32;

		sBlockBuffer1 = (char *) calloc(dwBlockSize + 1, sizeof(char));
		sBlockBuffer2 = (char *) calloc(dwBlockSize + 1, sizeof(char));
	}
	else if(giCryptAlgo == CRYPT_ALGO_BLOWFISH)
	{
		memset(&BlowFish, 0, sizeof(BlowFish));
	
		Blowfish_Init(&BlowFish, (unsigned char *)lpPwd->sPassword, lpPwd->iLength);
		dwBlockSize = 8;
	}

	//--------------------------------------------------------------------------------

	if(bTestOnly)
	{
		Set_Text(hPackingActionText, "Testing:");
	}
	else{
		Set_Text(hPackingActionText, "Extracting:");
	}

    int ilvItemPos = -1;

	DWORD dwFilePos = 0;

	for(DWORD dwCurrentFile = 0; dwCurrentFile <= dwCountOfFilesToExtract; dwCurrentFile++)
	{
		if(Header.iPkgAttribs &ATTRIB_COMPRESSION)
		{
			memset(&zStream, 0, sizeof(zStream));

			if((iCompressResult = inflateInit(&zStream)) != Z_OK)
			{
				ThrowFatalError("A call to InflateInit() has failed");
				iTestResult = PACK_RESULT_ERROR;
			}
		}

		if(giCryptAlgo == CRYPT_ALGO_NASCCL)
		{
			SS.Reset();
		}

		i64TotalBytesRead = 0;

#ifndef _SELFEXTRACTOR_APP
		if(gdwCurrentJobType == JOB_TYPE_EXTRACT_SELECTED)
        {
			DWORD dwTempfPos = 0;

			while((ilvItemPos = ListView_GetNextItem(ghFileList, ilvItemPos, LVNI_SELECTED)) != -1)
            {
                ListView_GetItemText(ghFileList, ilvItemPos, LIST_POS_POSITION, sTempBuf, 255);
	            FileSeek(hfSource, _atoi64(sTempBuf), FILE_BEGIN);
				break;
            }

			if(ilvItemPos == -1)
			{
				break;
			}
		}
#endif

		//-----------------------------------------------------------------------------------------------|
		//-----[BEGIN READ FILE HEADER]------------------------------------------------------------------|
		//-----------------------------------------------------------------------------------------------|

		//Read the file name length.
		ReadFile(hfSource, &usFileNameLen, sizeof(unsigned short), &dwBytesRead, NULL);
		if(dwBytesRead == 0)
		{
			//This is where this loop ends.
			break;
		}
		if(usFileNameLen <= 0 || usFileNameLen >= MAX_PATH) //Sanity check.
        {
			sprintf_s(sStatusText, sizeof(sStatusText),
				"Corrupt data file detected. Error code: (BreakPackage->usFileNameLen:%d|%d)", __LINE__, usFileNameLen);

            SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
			iTestResult = PACK_RESULT_ERROR;
            break;
        }

		//Read (and decode) the file name.
		ReadFile(hfSource, sPartFileName, usFileNameLen, &dwBytesRead, NULL);
        DfltCode.Cipher(sPartFileName, sPartFileName, usFileNameLen);
    	sPartFileName[usFileNameLen] = '\0';

		//Read the file size.
		ReadFile(hfSource, &i64FileSize, sizeof(i64FileSize), &dwBytesRead, NULL);

		//Read file attributes
		if(Header.iPkgAttribs &ATTRIB_ATTRIBUTES)
        {
            ReadFile(hfSource, &dwFileAttributes, sizeof(dwFileAttributes), &dwBytesRead, NULL);
        }

		//Read the creation and last modified date / time.
		if(Header.iPkgAttribs &ATTRIB_DATETIME)
        {
            ReadFile(hfSource, &ftCreationTime, sizeof(FILETIME), &dwBytesRead, NULL);
            ReadFile(hfSource, &ftLastWriteTime, sizeof(FILETIME), &dwBytesRead, NULL);
        }

		//Read the compressed file size.
		ReadFile(hfSource, &i64CompressedFileSize, sizeof(i64CompressedFileSize), &dwBytesRead, NULL);
		if(i64CompressedFileSize == -1) //Sanity check.
        {
            sprintf_s(sStatusText, sizeof(sStatusText),
				"Corrupt data file detected. Error code: (BreakPackage->i64CompressedFileSize:%d|%lld)",
				__LINE__, i64CompressedFileSize);

            SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

			DfltCode.Destroy();

            iActionResult = PACK_RESULT_ERROR;
			iTestResult = PACK_RESULT_ERROR;
			break;
        }
		
		if(Header.iPkgAttribs &ATTRIB_CRCCHECK)
		{
			ReadFile(hfSource, &ulReadCRC, sizeof(unsigned long), &dwBytesRead, NULL);
		}

		//Read the end-of-file position.
        ReadFile(hfSource, &i64FileEndPos, sizeof(i64FileEndPos), &dwBytesRead, NULL);
		if(i64FileEndPos == -1) //Sanity check.
        {
            sprintf_s(sStatusText, sizeof(sStatusText),
				"Corrupt data file detected. Error code: (BreakPackage->i64FileEndPos:%d|%lld)",
				__LINE__, i64FileEndPos);

            SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

			DfltCode.Destroy();

            iActionResult = PACK_RESULT_ERROR;
			iTestResult = PACK_RESULT_ERROR;
			break;
        }

		//-----------------------------------------------------------------------------------------------|
		//-----[END READ FILE HEADER]------------------------------------------------------------------|
		//-----------------------------------------------------------------------------------------------|

		if(!bTestOnly)
		{
			if(strlen(TarFolder) + usFileNameLen > 255)
			{
				sprintf_s(sStatusText, sizeof(sStatusText),
					"The file name is too long to be extracted into the selected directory. "
					"You must extract this file to a folder that is not as deep or extract "
					"files without using folder names.\r\n[%s]\r\n\r\n"
					"Do you want to skip this file and continue?", sPartFileName);

				iTestResult = PACK_RESULT_ERROR;

				if(SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONQUESTION|MB_YESNO|MB_APPLMODAL) == IDYES)
				{
					#ifdef _SELFEXTRACTOR_APP
						FileSeek(hfSource, i64FileEndPos + SELFEXTRACTOREXEEND, FILE_BEGIN);
					#else
						FileSeek(hfSource, i64FileEndPos, FILE_BEGIN);
					#endif

					continue;
				}
				else{
					iActionResult = PACK_RESULT_CANCEL;
					break;
				}
			}
		}

		if(gdwCurrentJobType == JOB_TYPE_CANCEL)
        {
            iActionResult = PACK_RESULT_CANCEL;
			iTestResult = PACK_RESULT_CANCEL;
            break;
        }

        SetStatusFileName(hPackingStatusText, sPartFileName);

		if(!bTestOnly)
		{
			strcpy_s(sFullFileName, sizeof(sFullFileName), TarFolder);
			if(gbUseFolderNames)
			{
		        strcat_s(sFullFileName, sizeof(sFullFileName), sPartFileName);
				CorrectReversePath(sFullFileName, sizeof(sFullFileName));
			}
			else{
				char sUFNFile[MAX_PATH];
				char sUFNExt[MAX_PATH];
				_splitpath_s(sPartFileName, NULL, 0, NULL, 0, sUFNFile, sizeof(sUFNFile), sUFNExt, sizeof(sUFNExt));
				_makepath_s(sFullFileName, sizeof(sFullFileName), NULL, TarFolder, sUFNFile, sUFNExt);
			}

			CustomCreateFolderStructure(sFullFileName);
		}

		bool bOverwrite = true;

		if(!bOverwriteAll && !bTestOnly)
		{
			if(FileAccess(sFullFileName, FExist))
			{
				strcpy_s(sReplaceFileFull, sizeof(sReplaceFileFull), sFullFileName);
				strcpy_s(sReplaceFile, sizeof(sReplaceFile), sPartFileName);

				memcpy_s(&ftGCreated, sizeof(ftGCreated), &ftCreationTime, sizeof(ftCreationTime));
				memcpy_s(&ftGModified, sizeof(ftGModified), &ftLastWriteTime, sizeof(ftLastWriteTime));
				i64GFileSz = i64FileSize;
				iGPkgAttribs = Header.iPkgAttribs;

				INT_PTR iAction = DialogBox(ghAppInstance,
					MAKEINTRESOURCE(IDD_REPLACEFILE),
					hProgressDialog,
					ReplaceFile
				);

				if(iAction == IDC_OPYES)
				{
					bOverwrite = true;
				}
				else if(iAction == IDC_OPNO)
				{
					bOverwrite = false;
				}
				else if(iAction == IDC_OPYESTOALL)
				{
					bOverwriteAll = true;
				}
				else if(iAction == IDC_OPCANCEL)
				{
					iActionResult = PACK_RESULT_CANCEL;
					break;
				}
			}
		}

		if(bOverwrite)
		{
			if(!bTestOnly)
			{
				SetFileAttributes(sFullFileName, FILE_ATTRIBUTE_NORMAL);

				hfTarget = CreateFile(
					sFullFileName, //DO NOT alter this buffer after this call. The file name is used again.
					GENERIC_WRITE,
					0,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL
				);

				if(hfTarget == INVALID_HANDLE_VALUE)
				{
					sprintf_s(sStatusText, sizeof(sStatusText),
						"The destination file is locked and cannot be overwritten.\r\n"
						"Do you want to skip this file and continue?"
						"\r\n\r\n%s", sFullFileName);

					if(SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONQUESTION|MB_YESNO|MB_APPLMODAL) == IDYES)
					{
						#ifdef _SELFEXTRACTOR_APP
							FileSeek(hfSource, i64FileEndPos + SELFEXTRACTOREXEEND, FILE_BEGIN);
						#else
							FileSeek(hfSource, i64FileEndPos, FILE_BEGIN);
						#endif

						continue;
					}
					else{
						iActionResult = PACK_RESULT_CANCEL;
						break;
					}
				}
			}

			fFileProgress = 0;
			SendMessage(hProgressFileStatusBar, PBM_SETPOS, 0, (LPARAM) 0);

			if(giCryptAlgo == CRYPT_ALGO_RIJNDAEL)
			{
				memcpy(&Aes_CtxPerFile, &Aes_Ctx, sizeof(Aes_Ctx));
			}

			if(Header.iPkgAttribs &ATTRIB_CRCCHECK)
			{
        		ulFinalCRC = 0xffffffff;
			}

			bool bCompression = true;
			DWORD dwBlockGap = 0;
			DWORD dwAdjustedSize = 0;
			
			do{
				ReadFile(hfSource, &bCompression, sizeof(bool), &dwBytesRead, NULL);
				ReadFile(hfSource, &dwOrigSize, sizeof(DWORD), &dwBytesRead, NULL);
				ReadFile(hfSource, &dwDeflSize, sizeof(DWORD), &dwBytesRead, NULL);

				i64TotalBytesRead = (i64TotalBytesRead + dwOrigSize);
				
				fFileProgress = ((float)((((float)i64TotalBytesRead) / ((float)i64FileSize)) * 100.0f));

				if(giCryptAlgo == CRYPT_ALGO_RIJNDAEL || giCryptAlgo == CRYPT_ALGO_BLOWFISH)
				{
					ReadFile(hfSource, &dwBlockGap, sizeof(DWORD), &dwBytesRead, NULL);
					if(dwBlockGap == dwBlockSize)
					{
						dwBlockGap = 0;
					}
				}

				dwAdjustedSize = (dwDeflSize + dwBlockGap);

				ReadFile(hfSource, sSourceBuf, dwAdjustedSize, &dwBytesRead, NULL);

				sSourceBuf[dwBytesRead] = 0;

				dwInflSize = (iBufferSize + EXTRA_SIZE);

				if(giCryptAlgo == CRYPT_ALGO_NASCCL)
				{
					//SaveDebugHex("C:\\Temp", "aBefore", (char *)sSourceBuf, dwDeflSize);
					SS.Cipher((char *)sSourceBuf, dwDeflSize);
					//SaveDebugHex("C:\\Temp", "xAfter", (char *)sSourceBuf, dwDeflSize);
				}
				else if(giCryptAlgo == CRYPT_ALGO_RIJNDAEL){
					DWORD dwRPos = 0;
					DWORD dwWPos = 0;

					DWORD dwCypRPos = 0;
					DWORD dwCypWPos = 0;

					while(dwRPos < dwAdjustedSize)
					{
						dwWPos = 0;
						while(dwWPos < dwBlockSize)
							sBlockBuffer1[dwWPos++] = sSourceBuf[dwRPos++];

						aes_dec_blk((unsigned char *)sBlockBuffer1, (unsigned char *)sBlockBuffer2, &Aes_CtxPerFile);

						dwCypRPos = 0;
						while(dwCypRPos < dwWPos)					
							sSourceBuf[dwCypWPos++] = sBlockBuffer2[dwCypRPos++];
					}
					
					sSourceBuf[dwCypWPos - dwBlockGap] = '\0';
					dwBytesRead = (dwBytesRead - dwBlockGap);
				}
				else if(giCryptAlgo == CRYPT_ALGO_BLOWFISH) {
					DWORD dwL = 0;
					DWORD dwR = 0;

					DWORD dwRPos = 0;
					DWORD dwWPos = 0;

					DWORD dwCypRPos = 0;
					DWORD dwCypWPos = 0;

					while(dwRPos < dwAdjustedSize)
					{
						dwWPos = 0;
						while(dwWPos < sizeof(DWORD))
							((char *)&dwL)[dwWPos++] = sSourceBuf[dwRPos++];

						dwWPos = 0;
						while(dwWPos < sizeof(DWORD))
							((char *)&dwR)[dwWPos++] = sSourceBuf[dwRPos++];

						Blowfish_Decrypt(&BlowFish, &dwL, &dwR);

						dwCypRPos = 0;
						while(dwCypRPos < sizeof(DWORD))					
							sSourceBuf[dwCypWPos++] = ((char *)&dwL)[dwCypRPos++];

						dwCypRPos = 0;
						while(dwCypRPos < sizeof(DWORD))					
							sSourceBuf[dwCypWPos++] = ((char *)&dwR)[dwCypRPos++];
					}
					
					sSourceBuf[dwCypWPos - dwBlockGap] = '\0';
					dwBytesRead = (dwBytesRead - dwBlockGap);
				}

				if(bCompression && (Header.iPkgAttribs &ATTRIB_COMPRESSION))
				{
					zStream.avail_in = dwDeflSize;
					zStream.avail_out = (iBufferSize + EXTRA_SIZE);

					zStream.next_out = (Bytef *) sTargetBuf;
					zStream.next_in = (Bytef *) sSourceBuf;

					if((iCompressResult = inflate(&zStream, Z_NO_FLUSH)) != Z_OK)
					{
						if(iCompressResult < Z_OK)
						{
							//if(_strcmpi(zStream.msg, "incorrect data check") == 0)
							//{
								//(8/1/2008) [MAJOR ERROR!] I have no idea wy this is happening, but the CRC checks out.
							//}
							//else
							{
								sprintf_s(sStatusText, sizeof(sStatusText),
									"In memory decompression failed while reading [%s],\r\n"
									"this file is probably corrupt and should be removed from the archive!\r\n\r\n"
									"Do you want to skip this file and continue?",
									sPartFileName);

								if(SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONERROR|MB_YESNO|MB_APPLMODAL) == IDYES)
								{
									#ifdef _SELFEXTRACTOR_APP
										FileSeek(hfSource, i64FileEndPos + SELFEXTRACTOREXEEND, FILE_BEGIN);
									#else
										FileSeek(hfSource, i64FileEndPos, FILE_BEGIN);
									#endif

									iTestResult = PACK_RESULT_ERROR;

									if(!bTestOnly)
									{
										CloseHandle(hfTarget);
										hfTarget = NULL;

										if(!SDeleteFile(sFullFileName))
										{
											SafeMsgBox(hProgressDialog, "Failed to delete the corrupted file.\r\n"
												"You should delete this file manually.",
												gsTitleCaption, MB_ICONINFORMATION|MB_OK|MB_APPLMODAL);
										}
									}

									iActionResult = PACK_RESULT_RESUME;
									break;
								}
								else{
									iActionResult = PACK_RESULT_ERROR;
									break;
								}

								if(iCompressResult != Z_STREAM_END)
								{
									break; //Done with file.
								}
							}
						}
					}

					dwInflSize = ((iBufferSize + EXTRA_SIZE) - zStream.avail_out);

					if(Header.iPkgAttribs &ATTRIB_CRCCHECK)
					{
						gCRC32->PartialCRC((unsigned int *)&ulFinalCRC, sTargetBuf, dwInflSize);
					}
					if(!bTestOnly)
					{
						if(!SafeWrite(hProgressDialog, hfTarget, sTargetBuf, dwInflSize))
						{
				            iActionResult = PACK_RESULT_CANCEL;
							break;
						}
					}
				}
				else {
					if(Header.iPkgAttribs &ATTRIB_CRCCHECK)
					{
						gCRC32->PartialCRC((unsigned int *)&ulFinalCRC, sSourceBuf, dwBytesRead);
					}
					if(!bTestOnly)
					{
						if(!SafeWrite(hProgressDialog, hfTarget, sSourceBuf, dwBytesRead))
						{
				            iActionResult = PACK_RESULT_CANCEL;
							break;
						}
					}
				}

				if(gdwCurrentJobType == JOB_TYPE_CANCEL)
				{
					iActionResult = PACK_RESULT_CANCEL;
					break;
				}
			} while(dwOrigSize == iBufferSize);

            if(iActionResult == PACK_RESULT_RESUME)
			{
				iActionResult = PACK_RESULT_OK;
				continue;
			}

            if(iActionResult != PACK_RESULT_OK)
			{
				if(!bTestOnly)
				{
					if(hfTarget) CloseHandle(hfTarget);
					SDeleteFile(sFullFileName);
				}
				break;
			}

			fFileProgress = 100;
			SendMessage(hProgressFileStatusBar, PBM_SETPOS, 100, (LPARAM) 0);

			if(Header.iPkgAttribs &ATTRIB_CRCCHECK)
			{
				ulFinalCRC = (ulFinalCRC ^ 0xffffffff);

				if(ulFinalCRC != ulReadCRC)
				{
					sprintf_s(sStatusText, sizeof(sStatusText),
						"Bad CRC %X should be %X\r\n[%s]\r\n\r\n"
						"Do you want to skip this file and continue?",
						ulFinalCRC, ulReadCRC, sPartFileName);

					iTestResult = PACK_RESULT_ERROR;

					if(SafeMsgBox(hProgressDialog, sStatusText, gsTitleCaption, MB_ICONQUESTION|MB_YESNO|MB_APPLMODAL) == IDYES)
					{
						#ifdef _SELFEXTRACTOR_APP
							FileSeek(hfSource, i64FileEndPos + SELFEXTRACTOREXEEND, FILE_BEGIN);
						#else
							FileSeek(hfSource, i64FileEndPos, FILE_BEGIN);
						#endif

						if(!bTestOnly)
						{
							CloseHandle(hfTarget);
							hfTarget = NULL;

							if(!SDeleteFile(sFullFileName))
							{
					            SafeMsgBox(hProgressDialog, "Failed to delete the corrupted file.\r\n"
									"You should delete this file manually.",
									gsTitleCaption, MB_ICONINFORMATION|MB_OK|MB_APPLMODAL);
							}
						}

						continue;
					}
					else{
			            iActionResult = PACK_RESULT_CANCEL;
						break;
					}
				}
			}

			if(!bTestOnly)
			{
				if(Header.iPkgAttribs &ATTRIB_DATETIME)
				{
					SetFileTime(hfTarget, &ftCreationTime, NULL, &ftLastWriteTime);
				}

				CloseHandle(hfTarget);

				if(Header.iPkgAttribs &ATTRIB_ATTRIBUTES)
				{
					SetFileAttributes(sFullFileName, dwFileAttributes);
				}
			}
		}
		else{
			//Skip File.
			#ifdef _SELFEXTRACTOR_APP
				FileSeek(hfSource, i64FileEndPos + SELFEXTRACTOREXEEND, FILE_BEGIN);
			#else
				FileSeek(hfSource, i64FileEndPos, FILE_BEGIN);
			#endif
		}

        fPackageProgress++;

		if(Header.iPkgAttribs &ATTRIB_COMPRESSION)
		{
			inflateEnd(&zStream);
		}
	}

	//--------------------------------------------------------------------------------

#ifndef _SELFEXTRACTOR_APP
	if(bTestOnly)
	{
		if(gdwCurrentJobType == JOB_TYPE_CANCEL)
		{
			iTestResult = PACK_RESULT_CANCEL;
		}
		else{
			__int64 i64EOFTest = 0;
			if(FileSize(hfSource, &i64EOFTest))
			{
				if(FileSeek(hfSource) != i64EOFTest)
				{
					SafeMsgBox(hProgressDialog,
						"This archive contains orphaned files. You should make a backup\r\n"
						"then extract and re-archive it\'s contents as soon as possible!",
						DISPLAYACRONYM"Corrupt Archive!", MB_ICONERROR|MB_OK|MB_APPLMODAL);

					iTestResult = PACK_RESULT_ERROR;
				}
			}
		}
	}
#endif

	//--------------------------------------------------------------------------------

    free(sSourceBuf);
    free(sTargetBuf);
	
	//Free memory for the RIJNDAEL algo.
	if(giCryptAlgo == CRYPT_ALGO_RIJNDAEL)
	{
		free(sBlockBuffer1);
		free(sBlockBuffer2);
	}
	else if(giCryptAlgo == CRYPT_ALGO_NASCCL){
		//Free memory used by the simple algo.
		SS.Destroy();
	}

    CloseHandle(hfSource);

	DfltCode.Destroy();

#ifndef _SELFEXTRACTOR_APP
	if(bTestOnly)
	{
		if(iTestResult == PACK_RESULT_OK)
		{
			SafeMsgBox(hProgressDialog,
				"This archive is healthy.",
				gsTitleCaption, MB_ICONINFORMATION);
		}
		else if(iTestResult == PACK_RESULT_CANCEL)
		{
			SafeMsgBox(hProgressDialog,
				"The archive test was cancelled.",
				gsTitleCaption, MB_ICONINFORMATION);
		}
		else if(iTestResult == PACK_RESULT_ERROR)
		{
			SafeMsgBox(hProgressDialog,
				"This archive is inconsistent! You should make a backup\r\n"
				"then extract and re-archive it\'s contents as soon as possible",
				gsTitleCaption, MB_ICONERROR|MB_OK|MB_APPLMODAL);
		}
	}
#endif

    return iActionResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
int PopGrid(char *sSrcName, LPPASSWORD lpPwd)
{
	int iImageIndex = -1;
	int iActionResult = PACK_RESULT_OK;

	char sDirectory[MAX_PATH + 1];
    char sFileName[MAX_PATH + 1];
    char sExt[MAX_PATH + 1];
    char sDescr[2048];

	char sPartFileName[MAX_PATH + 1];
    char sStatusText[5120];
    char sTempBuf[1024];
    char TempBuf0[1024];
    char TempBuf1[1024];

	__int64 i64TotalFileSizes = 0;
    __int64 i64CompressedFileSize = 0;
	__int64 i64FileEndPos = 0;
    __int64 i64FilePosition = 0;
    __int64 i64FileSize = 0;

	DWORD dwCompatVersion = 0;
	DWORD dwBytesRead = 0;
	DWORD dwFileAttributes = 0;

	FILETIME ftCreationTime;
    FILETIME ftLastWriteTime;

	HANDLE hfSource = NULL;

    long lReadChecksum = 0;

	unsigned long ulReadCRC = 0;
    unsigned short usFileNameLen = 0;

	NASCCLStream DfltCode;
    LV_ITEM lvItem;
	HEADER Header;

    memset(&lvItem, 0, sizeof(lvItem));
    memset(&DfltCode, 0, sizeof(DfltCode));
    memset(sTempBuf, 0, sizeof(sTempBuf));
    memset(&Header, 0, sizeof(Header));

    ImageList_RemoveAll(hImageList);
    ImageList_RemoveAll(hImageListBig);
	ListView_DeleteAllItems(ghFileList);

    DfltCode.Initialize(lpPwd->sPassword, lpPwd->iLength, true);

	__int64 i64ExeEnd = GetExeEnd(sSrcName);

    hfSource = CreateFile(
        sSrcName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if(hfSource == INVALID_HANDLE_VALUE)
    {
		DfltCode.Destroy();

		sprintf_s(sStatusText, sizeof(sStatusText), "Failed to open read file:\r\n'%s'.", sSrcName);
        SafeMsgBox(hReadingDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
		return PACK_RESULT_ERROR;
    }

	if(!ReadPackageHeader(i64ExeEnd, hfSource, hReadingDialog, lpPwd, &DfltCode, &Header, false))
	{
		DfltCode.Destroy();
		CloseHandle(hfSource);
		return PACK_RESULT_CANCEL;
	}

	SendMessage(hReadingStatusBar, PBM_SETRANGE32, 0, (LPARAM) Header.dwFileCount);

	HICON hSmallIcon = NULL;
    HICON hLargeIcon = NULL;

    int IconIndexCount = 0;
	DBGICONLIST* IcoList = NULL;

	if(gbShowGridIcons)
	{
		IcoList = (DBGICONLIST *) calloc(sizeof(DBGICONLIST), Header.dwFileCount + 1);

		hSmallIcon = LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_UNKNOWN16));
		hLargeIcon = LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_UNKNOWN32));
	
		ImageList_AddIcon(hImageList, hSmallIcon);
		ImageList_AddIcon(hImageListBig, hLargeIcon);

		DestroyIcon(hSmallIcon);
		DestroyIcon(hLargeIcon);

		DeleteObject(hSmallIcon);
		DeleteObject(hLargeIcon);
	}

	i64FileEndPos = FileSeek(hfSource) - i64ExeEnd;

	int iFilesRead = 0;

   	for(DWORD dwCurrentFile = 0; dwCurrentFile <= Header.dwFileCount; dwCurrentFile++)
	{
		i64FilePosition = FileSeek(hfSource, i64FileEndPos + i64ExeEnd, FILE_BEGIN);

		//-----------------------------------------------------------------------------------------------|
		//-----[BEGIN READ FILE HEADER]------------------------------------------------------------------|
		//-----------------------------------------------------------------------------------------------|

		//Read the file name length.
		ReadFile(hfSource, &usFileNameLen, sizeof(unsigned short), &dwBytesRead, NULL);
		if(dwBytesRead == 0)
		{
			//This is where this loop ends.
			break;
		}

		if(usFileNameLen <= 0 || usFileNameLen >= MAX_PATH) //Sanity check.
		{
			DfltCode.Destroy();

			sprintf_s(sStatusText, sizeof(sStatusText),
				"Corrupt data file detected. Error code: (PopGrid->usFileNameLen:%d|%d)", __LINE__, usFileNameLen);

            SafeMsgBox(hReadingDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

            iActionResult = PACK_RESULT_ERROR;
			break;
        }

		//Read (and decode) the file name.
		ReadFile(hfSource, sPartFileName, usFileNameLen, &dwBytesRead, NULL);
        DfltCode.Cipher(sPartFileName, sPartFileName, usFileNameLen);
    	sPartFileName[usFileNameLen] = '\0';

		//Read the file size.
        ReadFile(hfSource, &i64FileSize, sizeof(i64FileSize), &dwBytesRead, NULL);

		//Read file attributes
		if(Header.iPkgAttribs &ATTRIB_ATTRIBUTES)
        {
            ReadFile(hfSource, &dwFileAttributes, sizeof(dwFileAttributes), &dwBytesRead, NULL);
        }

		//Read the creation and last modified date / time.
		if(Header.iPkgAttribs &ATTRIB_DATETIME)
		{
			ReadFile(hfSource, &ftCreationTime, sizeof(FILETIME), &dwBytesRead, NULL);
			ReadFile(hfSource, &ftLastWriteTime, sizeof(FILETIME), &dwBytesRead, NULL);
		}
		
        i64TotalFileSizes += i64FileSize; //Add all the file sizes together.

		//Read the compressed file size.
		ReadFile(hfSource, &i64CompressedFileSize, sizeof(i64CompressedFileSize), &dwBytesRead, NULL);
		if(i64CompressedFileSize == -1) //Sanity check.
        {
			DfltCode.Destroy();

			sprintf_s(sStatusText, sizeof(sStatusText),
				"Corrupt data file detected. Error code: (PopGrid->i64CompressedFileSize:%d|%lld)", __LINE__, i64CompressedFileSize);

            SafeMsgBox(hReadingDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

            iActionResult = PACK_RESULT_ERROR;
			break;
        }
		
		//Read the file CRC.
		if(Header.iPkgAttribs &ATTRIB_CRCCHECK)
        {
            ReadFile(hfSource, &ulReadCRC, sizeof(unsigned long), &dwBytesRead, NULL);
        }

		//Read the end-of-file position.
		ReadFile(hfSource, &i64FileEndPos, sizeof(i64FileEndPos), &dwBytesRead, NULL);
		if(i64FileEndPos == -1) //Sanity check.
        {
			DfltCode.Destroy();

			sprintf_s(sStatusText, sizeof(sStatusText),
				"Corrupt data file detected. Error code: (PopGrid->i64FileEndPos:%d|%lld)", __LINE__, i64FileEndPos);

            SafeMsgBox(hReadingDialog, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

            iActionResult = PACK_RESULT_ERROR;
			break;
        }
				
		FileSeek(hfSource, i64FileEndPos, FILE_CURRENT);

		//-----------------------------------------------------------------------------------------------|
		//-----[END READ FILE HEADER]--------------------------------------------------------------------|
		//-----------------------------------------------------------------------------------------------|

		_splitpath_s(sPartFileName,
			NULL, 0,
			sDirectory, sizeof(sDirectory),
			sFileName, sizeof(sFileName),
			sExt, sizeof(sExt));

	    iImageIndex = -1;

		if(gbShowGridIcons)
		{
			for(int iIconSCount = 0; iIconSCount < IconIndexCount; iIconSCount++)
			{
				if( _strcmpi(IcoList[iIconSCount].sExt, sExt) == 0)
				{
					iImageIndex = IcoList[iIconSCount].iIndex;
					strcpy_s(sDescr, sizeof(sDescr), IcoList[iIconSCount].sDescr);
					break;
				}
			}

			if(iImageIndex == -1)
			{
    			char IconFile[255 + 1];
    			int IconIndex = 0;

    			if(GetIconForExt(sExt, IconFile, sizeof(IconFile), &IconIndex, sDescr, sizeof(sDescr)))
				{
					if(strlen(IconFile) > 0)
					{
						hSmallIcon = NULL;
						hLargeIcon = NULL;
						ExtractIconEx(IconFile, IconIndex, &hLargeIcon, &hSmallIcon, 1);
						if(hSmallIcon)
						{
							int xIndex = ImageList_AddIcon(hImageList, hSmallIcon);
							ImageList_AddIcon(hImageListBig, hLargeIcon);

							if(xIndex != -1)
							{
								DestroyIcon(hSmallIcon);
								DestroyIcon(hLargeIcon);
								DeleteObject(hSmallIcon);
								DeleteObject(hLargeIcon);

								int iExtMem = (int)strlen(sExt);
								int iDescrMem = (int)strlen(sDescr);

								AllocIconList(&IcoList[IconIndexCount], iExtMem, iDescrMem);

								strcpy_s(IcoList[IconIndexCount].sExt, iExtMem + 1, sExt);
								strcpy_s(IcoList[IconIndexCount].sDescr, iDescrMem + 1, sDescr);

								IcoList[IconIndexCount].iIndex = xIndex;

								iImageIndex = IcoList[IconIndexCount].iIndex;
								strcpy_s(sDescr, sizeof(sDescr), IcoList[IconIndexCount].sDescr);

								IconIndexCount++;
							}
						}
					}
				}
			}
		}

        if(iImageIndex == -1)
        {
			if(_strupr_s(sExt, sizeof(sExt)) == 0)
			{
				char sAltExt[MAX_PATH + 1];
				int iExtAltLen = (int)strlen(sExt);

				if(Right(sAltExt, sizeof(sAltExt), sExt, iExtAltLen, iExtAltLen - 1) > 0)
				{
					sprintf_s(sDescr, sizeof(sDescr), "%s File", sAltExt);
				}
				else{
					strcpy_s(sDescr, sizeof(sDescr), "Unknown File");
				}
			}
			else{
				strcpy_s(sDescr, sizeof(sDescr), "Unknown File");
			}
			iImageIndex = 0;
		}

        sprintf_s(sTempBuf, sizeof(sTempBuf), "%s%s", sFileName, sExt);
        lvItem.pszText  = sTempBuf;
        if(Header.iPkgAttribs &ATTRIB_DATETIME)
        {
            lvItem.mask = LVIF_TEXT|LVIF_PARAM;
            lvItem.lParam = (LPARAM) ftLastWriteTime.dwHighDateTime;
        }
        else lvItem.mask = LVIF_TEXT;
        lvItem.iImage   = iImageIndex;
        lvItem.iSubItem = 0;

		if(gbShowGridIcons)
		{
			lvItem.mask |= LVIF_IMAGE;
		}

        ListView_InsertItem(ghFileList, &lvItem);

        lvItem.pszText  = sDescr; // Type
        lvItem.mask     = LVIF_TEXT;
        lvItem.iSubItem = 1;
        ListView_SetItem(ghFileList, &lvItem);

		FileTimeToString(&ftLastWriteTime,
			(bool)!(Header.iPkgAttribs &ATTRIB_DATETIME), sTempBuf, sizeof(sTempBuf));

        lvItem.pszText  = sTempBuf; // Modified
        lvItem.mask     = LVIF_TEXT;
        lvItem.iSubItem = 2;
        ListView_SetItem(ghFileList, &lvItem);

        sprintf_s(sTempBuf, sizeof(sTempBuf), "%I64d", i64FileSize);
        lvItem.pszText  = FormatiString(TempBuf0, sizeof(TempBuf0), sTempBuf); // Size
        lvItem.mask     = LVIF_TEXT;
        lvItem.iSubItem = 3;
        ListView_SetItem(ghFileList, &lvItem);

        if(Header.iPkgAttribs &ATTRIB_COMPRESSION)
        {
            if(i64CompressedFileSize != 0 && i64FileSize != 0 && i64FileSize > i64CompressedFileSize)
            {
                float fCompressedFileSize = (float) i64CompressedFileSize;
                float fFileSize = (float) i64FileSize;
                float Ratio = 100.0f - ((fCompressedFileSize/fFileSize) * 100.0f);
                sprintf_s(sTempBuf, sizeof(sTempBuf), "%.2f%%", Ratio);
            }
            else strcpy_s(sTempBuf, sizeof(sTempBuf), "0.00%");
        }
        else strcpy_s(sTempBuf, sizeof(sTempBuf), "(n/a)");

        lvItem.pszText  = sTempBuf; // Ratio
        lvItem.mask     = LVIF_TEXT;
        lvItem.iSubItem = 4;
        ListView_SetItem(ghFileList, &lvItem);

        sprintf_s(sTempBuf, sizeof(sTempBuf), "%I64d", i64CompressedFileSize);
        lvItem.pszText  = FormatiString(TempBuf0, sizeof(TempBuf0), sTempBuf); // Packed
        lvItem.mask     = LVIF_TEXT;
        lvItem.iSubItem = 5;
        ListView_SetItem(ghFileList, &lvItem);

        if(Header.iPkgAttribs &ATTRIB_CRCCHECK)
        {
            sprintf_s(sTempBuf, sizeof(sTempBuf), "%X", ulReadCRC);
        }
        else strcpy_s(sTempBuf, sizeof(sTempBuf), "(n/a)");

        lvItem.pszText  = sTempBuf; // CRC
        lvItem.mask     = LVIF_TEXT;
        lvItem.iSubItem = 6;
        ListView_SetItem(ghFileList, &lvItem);

        if(Header.iPkgAttribs &ATTRIB_ATTRIBUTES)
        {
			MakeAttributeString(dwFileAttributes, sTempBuf, sizeof(sTempBuf));
        }
        else strcpy_s(sTempBuf, sizeof(sTempBuf), "(n/a)");

        lvItem.pszText  = sTempBuf; // Attributes
        lvItem.mask     = LVIF_TEXT;
        lvItem.iSubItem = 7;
        ListView_SetItem(ghFileList, &lvItem);

        if(Header.iPkgAttribs &ATTRIB_SAVEPATHS)
        {
            strcpy_s(sTempBuf, sizeof(sTempBuf), sDirectory);
        }
        else strcpy_s(sTempBuf, sizeof(sTempBuf), "(n/a)");

        lvItem.pszText  = sTempBuf; // Path
        lvItem.mask     = LVIF_TEXT;
        lvItem.iSubItem = 8;
        ListView_SetItem(ghFileList, &lvItem);

        sprintf_s(sTempBuf, sizeof(sTempBuf), "%I64d", i64FilePosition);
        lvItem.pszText  = sTempBuf; // Position
        lvItem.mask     = LVIF_TEXT;
        lvItem.iSubItem = 9;
        ListView_SetItem(ghFileList, &lvItem);

        if(gdwCurrentJobType == JOB_TYPE_CANCEL)
        {
			iActionResult = PACK_RESULT_CANCEL;
            Sleep(1000);
            break;
        }

		iFilesRead++;
        fPackageProgress = (float)iFilesRead;
	}

	if(gdwCurrentJobType != JOB_TYPE_CANCEL)
	{
		__int64 i64EOFTest = 0;
		if(FileSize(hfSource, &i64EOFTest))
		{
			if(FileSeek(hfSource) != i64EOFTest)
			{
		        SafeMsgBox(hReadingDialog,
					"This archive contains orphaned files. You should make a backup\r\n"
					"then extract and re-archive it\'s contents as soon as possible!",
					DISPLAYACRONYM"Corrupt Archive!", MB_ICONERROR|MB_OK|MB_APPLMODAL);
			}
		}
	}

	//FIXFIX: Detect if the file was loaded successfully.
    CloseHandle(hfSource);
	DfltCode.Destroy();

    sprintf_s(sTempBuf, sizeof(sTempBuf), "%d", Header.dwFileCount);
    FormatiString(TempBuf0, sizeof(TempBuf0), sTempBuf);

	FileSizeFriendly(i64TotalFileSizes, TempBuf1, sizeof(TempBuf1));
    sprintf_s(sStatusText, sizeof(sStatusText), "Total %s files, %s", TempBuf0, TempBuf1);
    SendMessage(hStatusBar, (UINT)SB_SETTEXT, (WPARAM)1, (LPARAM)sStatusText);
    SendMessage(hStatusBar, (UINT)SB_SETTEXT, (WPARAM)0, (LPARAM)"Selected: 0 Files, 0.00 B");

    // Free the icon search list.
    for(int iIconSCount = 0; iIconSCount < IconIndexCount; iIconSCount++)
    {
        FreeIconList(&IcoList[iIconSCount]);
	}

	if(iActionResult == PACK_RESULT_OK)
	{
		SortFileList(ghFileList, gdwDefaultSortColumn, gdwDefaultSortDirection);
		AutoSizeGridCols();
	}

	return iActionResult;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
bool DupsSelected(void)
{
	char sParentName[MAX_PATH];
	char sChildName[MAX_PATH];

	if(ListView_GetSelectedCount(ghFileList) > 0)
	{
		for(int iParent = -1; ((iParent = ListView_GetNextItem(ghFileList, iParent, LVNI_SELECTED))) != -1;)
		{
			ListView_GetItemText(ghFileList, iParent, LIST_POS_NAME, sParentName, sizeof(sParentName));

			for(int iChild = -1; ((iChild = ListView_GetNextItem(ghFileList, iChild, LVNI_SELECTED))) != -1;)
			{
				if(iChild != iParent)
				{
					ListView_GetItemText(ghFileList, iChild, LIST_POS_NAME, sChildName, sizeof(sChildName));
					if(_strcmpi(sParentName, sChildName) == 0)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
