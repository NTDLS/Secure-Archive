#ifndef _PACKHEADER_H
#define _PACKHEADER_H
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Entry.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//For extra header information, see the "Header Makeup.doc" file.

#define SELFEXTRACTOREXEEND		700416    
#define PACKAGE_HEADER_LEN		1024
#define RESERVE_HEADER_SZ		844
#define THUMB_PRINT_SZ			128
#define FILE_COUNT_POS			PACKAGE_HEADER_LEN - sizeof(DWORD)
#define CHECKSUM_LEN			256
#define HARDCHECKSUMBASE		9864678 //Just a random number.

#define CRYPT_ALGO_NASCCL		0
#define CRYPT_ALGO_RIJNDAEL		1
#define CRYPT_ALGO_BLOWFISH		2

#define ATTRIB_COMPRESSION		0x0001
#define ATTRIB_DATETIME			0x0002
#define ATTRIB_CRCCHECK			0x0004
#define ATTRIB_SAVEPATHS		0x0010
#define ATTRIB_ATTRIBUTES		0x0020
#define ATTRIB_NOPASSWORD		0x0040L
#define ATTRIB_INCLUDEHIDDEN	0x0080L
//#define ATTRIB_RESERVED		0x0100L

typedef struct _TAG_HEADER{
    int iPkgAttribs;
	int iCompressLevel;

	DWORD dwCreationDate;
	DWORD dwCreationTime;
    DWORD dwFileCount;

	char sThumbPrint[THUMB_PRINT_SZ + 1];
} HEADER, *LPHEADER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool IsUsingBlankPassword(HWND hOwner, const char *sFileName, bool *bIsBlank);

bool ReadPackageHeader(__int64 i64ExeEnd, HANDLE hfSource, HWND hOwner,
					   LPPASSWORD lpPwd, NASCCLStream *lpDfltCode, LPHEADER lpHeader, bool bPublicOnly);

__int64 GetExeEnd(const char *sFileName);

#ifndef _SELFEXTRACTOR_APP
bool WritePackageHeader(__int64 i64ExeEnd, HANDLE hfTarget, HWND hOwner,
						LPPASSWORD lpPwd, NASCCLStream *lpDfltCode, LPHEADER lpHeader);
typedef unsigned long (WINAPI *PGetExeEnd)(void);
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
