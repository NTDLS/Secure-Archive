#ifndef _PACKHEADER_CPP
#define _PACKHEADER_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <ShlObj.H>
#include <Stdio.H>
#include <Stdlib.H>

#include "IndexDir.H"
#include "Package.H"
#include "Entry.H"
#include "Routines.H"
#include "PackHeader.H"

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

using namespace NSWFL::File;
using namespace NSWFL::DateTime; 
using namespace NSWFL::KeyGeneration;
using namespace NSWFL::Hashing;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
__int64 GetExeEnd(const char *sFileName)
{
	struct EXEHEAD
	{
		char id[2]; 			// 'M' & 'Z'.
		unsigned lastpg;		// no of bytes on last page.
		unsigned size;			// total no of 512 byte pages.
		unsigned reloc; 		// no of relocation table items.
		unsigned headersize;	// header size in paras.
		unsigned minpara;		// min. paras reqd. by prog.
		unsigned maxpara;		// max. paras reqd. by prog.
		unsigned stackseg;		// initial value of stack seg.
		unsigned stackoff;		// initial value of SP.
		unsigned chksum;		// header check sum.
		unsigned IP;			// entry point IP.
		unsigned CS;			// entry point CS.
		unsigned relocoff;		// offset of 1st relocation item.
		unsigned char overlay;	// overlay number.
	};

	unsigned long lExeEnd = 0;

	if(FileAccess(sFileName, FExist))
	{
		char sExtension[MAX_PATH];
		GetFileExtension(sFileName, sExtension, sizeof(sExtension));
		if(_strcmpi(sExtension, ".EXE") == 0)
		{
			EXEHEAD Head;
			memset(&Head, 0, sizeof(Head));
			FILE *hHeadExe = NULL;
			if(fopen_s(&hHeadExe, sFileName, "rb") != 0)
			{
				return lExeEnd;
			}
			fread(&Head, sizeof(Head), 1, hHeadExe);

			fclose(hHeadExe);

			if(Head.id[0] == 0x4d && Head.id[1] == 0x5a) //MZ header
			{
				HMODULE hExe = LoadLibraryEx(sFileName, 0, LOAD_LIBRARY_AS_DATAFILE); //Is valid EXE or DLL?
				if(hExe)
				{
					FreeLibrary(hExe);

					if((hExe = LoadLibraryEx(sFileName, 0, DONT_RESOLVE_DLL_REFERENCES)))
					{
						PGetExeEnd pGetExeEnd = NULL;
						if((pGetExeEnd = (PGetExeEnd) GetProcAddress(hExe, "ExGetExeEnd")))
						{
							lExeEnd = pGetExeEnd();
						}
						FreeLibrary(hExe);
					}
				}
			}
		}
	}

	return lExeEnd;
}
#else
__int64 GetExeEnd(const char *sFileName)
{
	return 0l;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _SELFEXTRACTOR_APP
extern "C" __declspec(dllexport) __int64 ExGetExeEnd(void)
{
	return SELFEXTRACTOREXEEND;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ReadPackageHeader(__int64 i64ExeEnd, HANDLE hfSource, HWND hOwner, LPPASSWORD lpPwd,
	NASCCLStream *lpDfltCode, LPHEADER lpHeader, bool bPublicOnly)
{
    long lReadChecksum = 0;
	DWORD dwBytesRead = 0;

	memset(lpHeader, 0, sizeof(HEADER));

#ifndef _SELFEXTRACTOR_APP
	FileSeek(hfSource, i64ExeEnd, FILE_BEGIN);
#else
	FileSeek(hfSource, SELFEXTRACTOREXEEND, FILE_BEGIN);
#endif

	//Read the Application Checksum.
	ReadFile(hfSource, &lReadChecksum, sizeof(lReadChecksum), &dwBytesRead, NULL);
	gCode.Cipher(&lReadChecksum, &lReadChecksum, sizeof(lReadChecksum));
	if(lReadChecksum != MakeChecksum(gsTitleCaption, (int)strlen(gsTitleCaption)))
	{
		SafeMsgBox(hOwner,
			"The file you are trying to decrypt was not encrypted with this application.",
			gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

		return false;
	}

	//Read the Application Version Checksum.
	ReadFile(hfSource, &lReadChecksum, sizeof(lReadChecksum), &dwBytesRead, NULL);
	gCode.Cipher(&lReadChecksum, &lReadChecksum, sizeof(lReadChecksum));
	if(lReadChecksum != MakeChecksum(gsCompatibility, (int)strlen(gsCompatibility)))
	{
		SafeMsgBox(hOwner,
			"The file you are trying to decrypt was encrypted using an incompatible version.",
			gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
		return false;
	}

	//Read the Package Attributes.
    ReadFile(hfSource, &lpHeader->iPkgAttribs, sizeof(lpHeader->iPkgAttribs), &dwBytesRead, NULL);
    gCode.Cipher(&lpHeader->iPkgAttribs, &lpHeader->iPkgAttribs, sizeof(lpHeader->iPkgAttribs));

	if(bPublicOnly)
	{
		//Exit here, we don't want to give up the checksums n' stuff.
		return true;
	}

	//Read the Password Checksum (Begin).
	SHA1 SHA;
	unsigned long ulFileDigest[5];
	unsigned long ulUserDigest[5];

	for(int i = 0; i < 5 ; i++)
	{
		ReadFile(hfSource, &ulFileDigest[i], sizeof(ulFileDigest[i]), &dwBytesRead, NULL);
		lpDfltCode->Cipher(&ulFileDigest[i], &ulFileDigest[i], sizeof(ulFileDigest[i]));
	}

	SHA.Input(lpPwd->sPassword, lpPwd->iLength);
	if(!SHA.Result(ulUserDigest))
	{
		ThrowFatalError("Failed to generate the key hash.");
		return false;
	}
	SHA.Reset();

	for(int i = 0; i < 5 ; i++)
	{
		if(ulFileDigest[i] != ulUserDigest[i])
		{
			SafeMsgBox(hOwner, "Wrong key, Can't decrypt.", gsTitleCaption, MB_ICONINFORMATION|MB_OK|MB_APPLMODAL);
			return false;
		}
	}
	//Read the Password Checksum (End)

	//Read the Cryptography Algorithm.
	ReadFile(hfSource, &giCryptAlgo, sizeof(giCryptAlgo), &dwBytesRead, NULL);
    lpDfltCode->Cipher(&giCryptAlgo, &giCryptAlgo, sizeof(giCryptAlgo));

	//Read the Compression Level.
    ReadFile(hfSource, &lpHeader->iCompressLevel, sizeof(lpHeader->iCompressLevel), &dwBytesRead, NULL);
    lpDfltCode->Cipher(&lpHeader->iCompressLevel, &lpHeader->iCompressLevel, sizeof(lpHeader->iCompressLevel));

	//Read the Julian Creation Date.
	ReadFile(hfSource, &lpHeader->dwCreationDate, sizeof(lpHeader->dwCreationDate), &dwBytesRead, NULL);
    lpDfltCode->Cipher(&lpHeader->dwCreationDate, &lpHeader->dwCreationDate, sizeof(lpHeader->dwCreationDate));

	//Read the Julian Creation Time.
	ReadFile(hfSource, &lpHeader->dwCreationTime, sizeof(lpHeader->dwCreationTime), &dwBytesRead, NULL);
    lpDfltCode->Cipher(&lpHeader->dwCreationTime, &lpHeader->dwCreationTime, sizeof(lpHeader->dwCreationTime));

	//Read the File thumb print or serial number.
	ReadFile(hfSource, lpHeader->sThumbPrint, THUMB_PRINT_SZ, &dwBytesRead, NULL);
    lpDfltCode->Cipher(lpHeader->sThumbPrint, lpHeader->sThumbPrint, THUMB_PRINT_SZ);

	//Skip past the Reserved header space.
	FileSeek(hfSource, RESERVE_HEADER_SZ, FILE_CURRENT);

	//Read the File count.
	ReadFile(hfSource, &lpHeader->dwFileCount, sizeof(lpHeader->dwFileCount), &dwBytesRead, NULL);
    lpDfltCode->Cipher(&lpHeader->dwFileCount, &lpHeader->dwFileCount, sizeof(lpHeader->dwFileCount));

	if(lpHeader->dwFileCount == 0 || lpHeader->dwFileCount == (DWORD)-1)
	{
		SafeMsgBox(hOwner, "The archive is corrupt!", gsTitleCaption, MB_ICONERROR|MB_OK|MB_APPLMODAL);
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
bool WritePackageHeader(__int64 i64ExeEnd, HANDLE hfTarget, HWND hOwner, LPPASSWORD lpPwd, NASCCLStream *lpDfltCode, LPHEADER lpHeader)
{
	long lChecksum = 0;
    long iTemp = 0;
	DWORD dwTemp = 0;

	SYSTEMTIME stCreation;

	FileSeek(hfTarget, i64ExeEnd, FILE_BEGIN);

	GetSystemTime(&stCreation);
	lpHeader->dwCreationDate = DateToLong(stCreation.wYear, stCreation.wMonth, stCreation.wDay);
	lpHeader->dwCreationTime = TimeToLong(stCreation.wHour, stCreation.wMinute, stCreation.wSecond);

	GenerateKey(THUMB_PRINT_SZ, GenerateSeed(), GKNONREPETITION | GKBROADKEY, lpHeader->sThumbPrint);

	//Write the Application Checksum.
	lChecksum = MakeChecksum(gsTitleCaption, (int)strlen(gsTitleCaption));
	gCode.Cipher(&lChecksum, &iTemp, sizeof(iTemp));
	if(!SafeWrite(hfTarget, &iTemp, sizeof(iTemp)))
	{
		return false;
	}

	//Write the Application Version Checksum.
	lChecksum = MakeChecksum(gsCompatibility, (int)strlen(gsCompatibility));
	gCode.Cipher(&lChecksum, &iTemp, sizeof(iTemp));
	if(!SafeWrite(hfTarget, &iTemp, sizeof(iTemp)))
	{
		return false;
	}

	//Write the Package Attributes.
	gCode.Cipher(&lpHeader->iPkgAttribs, &iTemp, sizeof(int));
	if(!SafeWrite(hfTarget, &iTemp, sizeof(int)))
	{
		return false;
	}

	//Write the Password Checksum (Begin)
	SHA1 SHA;
	unsigned long ulDigest[5];

	SHA.Input(lpPwd->sPassword, lpPwd->iLength);
	if(!SHA.Result(ulDigest))
	{
		ThrowFatalError("Failed to generate the key hash.");
		return false;
	}
	SHA.Reset();

	for(int i = 0; i < 5 ; i++)
	{
		lpDfltCode->Cipher(&ulDigest[i], &ulDigest[i], sizeof(ulDigest[i]));
		if(!SafeWrite(hfTarget, &ulDigest[i], sizeof(ulDigest[i])))
		{
			return false;
		}
	}
	//Write the Password Checksum (End)

	//Write the Cryptography Algorithm
	lpDfltCode->Cipher(&giCryptAlgo, &iTemp, sizeof(giCryptAlgo));
	if(!SafeWrite(hfTarget, &iTemp, sizeof(giCryptAlgo)))
	{
		return false;
	}

	//Read the Compression Level.
    lpDfltCode->Cipher(&lpHeader->iCompressLevel, &iTemp, sizeof(lpHeader->iCompressLevel));
    if(!SafeWrite(hfTarget, &iTemp, sizeof(iTemp)))
	{
		return false;
	}

	//Write the Julian Creation Date.
	lpDfltCode->Cipher(&lpHeader->dwCreationDate, &lpHeader->dwCreationDate, sizeof(lpHeader->dwCreationDate));
	if(!SafeWrite(hfTarget, &lpHeader->dwCreationDate, sizeof(lpHeader->dwCreationDate)))
	{
		return false;
	}

	//Write the Julian Creation Time.
	lpDfltCode->Cipher(&lpHeader->dwCreationTime, &lpHeader->dwCreationTime, sizeof(lpHeader->dwCreationTime));
	if(!SafeWrite(hfTarget, &lpHeader->dwCreationTime, sizeof(lpHeader->dwCreationTime)))
	{
		return false;
	}

	//Write the File thumb print or serial number.
	lpDfltCode->Cipher(lpHeader->sThumbPrint, lpHeader->sThumbPrint, THUMB_PRINT_SZ);
	if(!SafeWrite(hfTarget, lpHeader->sThumbPrint, THUMB_PRINT_SZ))
	{
		return false;
	}

	//Skip past the Reserved header space.
	FileSeek(hfTarget, RESERVE_HEADER_SZ, FILE_CURRENT);

	//Write the File count.
	lpDfltCode->Cipher(&lpHeader->dwFileCount, &dwTemp, sizeof(lpHeader->dwFileCount));
	if(!SafeWrite(hfTarget, &dwTemp, sizeof(dwTemp)))
	{
		return false;
	}

	return true;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool IsUsingBlankPassword(HWND hOwner, const char *sFileName, bool *bIsBlank)
{
	*bIsBlank = false;

	HEADER Header;

	__int64 i64ExeEnd = GetExeEnd(sFileName);

	HANDLE hfSource = CreateFile(
        sFileName,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if(hfSource == INVALID_HANDLE_VALUE)
    {
		char sStatusText[2048];
        sprintf_s(sStatusText, sizeof(sStatusText),
			"Failed to open read package header of file:\r\n'%s'.", sFileName);
	    SafeMsgBox(hOwner, sStatusText, gsTitleCaption, MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);
        return false;
    }

	if(ReadPackageHeader(i64ExeEnd, hfSource, hOwner, NULL, NULL, &Header, true))
	{
		if(Header.iPkgAttribs &ATTRIB_NOPASSWORD)
		{
			*bIsBlank = true;
		}
	}
	else{
		CloseHandle(hfSource);
		return false;
	}

	CloseHandle(hfSource);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
