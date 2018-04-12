#ifndef _HardKey_Cpp
#define _HardKey_Cpp
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <ShlObj.H>
#include <Stdio.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "../Source/Entry.H"
#include "../Source/Routines.H"
#include "../Source/HardKey.H"

#ifndef _SELFEXTRACTOR_APP
#include "../Dialogs/HardKeyDlg.H"
#include "../Dialogs/HardKeyStatDlg.H"
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows;
using namespace NSWFL::String;
using namespace NSWFL::Conversion;
using namespace NSWFL::Hashing;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define HARDKEYBUFSZ	1024

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ReadHardKey(HWND hOwner, const char *sFileName, const char *sPwd, int iPwdLen, LPPASSWORD lpPwd)
{
	long lChecksum = 0;
	int iKeySize = 0;
	unsigned long ulDigest[5];

	NASCCLStream DfltCode;
	SHA1 SHA;
	FILE *fSource = NULL;

    memset(&DfltCode, 0, sizeof(DfltCode));

	if(fopen_s(&fSource, sFileName, "rb") != 0)
	{
		SafeMsgBox(hOwner, "Failed to open the selected HardKey.\r\n"
			"Ensure that you have permission to access this file.", gsTitleCaption, MB_ICONERROR);
		return false;
	}

    DfltCode.Initialize((char *)sPwd, iPwdLen, false);

	//Read the Application Checksum.
	fread(&lChecksum, sizeof(lChecksum), 1, fSource);
	if(lChecksum != MakeChecksum(gsTitleCaption, (int)strlen(gsTitleCaption)))
	{
		SafeMsgBox(hOwner, "An invalid HardKey file was specified.", gsTitleCaption, MB_ICONINFORMATION);

		fclose(fSource);
		DfltCode.Destroy();
		return false;
	}

	//Read the Application Version Checksum.
	fread(&lChecksum, sizeof(lChecksum), 1, fSource);
	if(lChecksum != MakeChecksum(gsCompatibility, (int)strlen(gsCompatibility)))
	{
		SafeMsgBox(hOwner,
			"The selected HardKey is not compatible with this version of Secure Archive.",
			gsTitleCaption, MB_ICONINFORMATION);

		fclose(fSource);
		DfltCode.Destroy();
		return false;
	}

	//Read the KeySize in Bytes.
	fread(&iKeySize, sizeof(iKeySize), 1, fSource);
	DfltCode.Cipher(&iKeySize, sizeof(iKeySize));

	//Read the Password Checksum (Begin)
	SHA.Input(sPwd, iPwdLen);
	SHA.Result(ulDigest);
	SHA.Reset();

	for(int i = 0; i < 5 ; i++)
	{
		unsigned int iHashSeg = 0;

		if(fread(&iHashSeg, sizeof(unsigned int), 1, fSource) != 1)
		{
			SafeMsgBox(hOwner, "Failed to read key hash.", gsTitleCaption, MB_ICONINFORMATION);
			fclose(fSource);
			return false;
		}
		DfltCode.Cipher(&iHashSeg, &iHashSeg, sizeof(iHashSeg));
		if(iHashSeg != ulDigest[i])
		{
			SafeMsgBox(hOwner, "The password you entered does not correspond to the selected HardKey.\r\n"
				"Enter the password that you used when creating the HardKey file.", gsTitleCaption, MB_ICONINFORMATION);
			fclose(fSource);
			DfltCode.Destroy();
			return false;
		}
	}
	//Read the Password Checksum (End)

	lpPwd->iLength = iKeySize;
	lpPwd->sPassword = (char *) calloc(sizeof(char), lpPwd->iLength + 1);

	int iSingleRead = 0;
	int iTotalRead = 0;

	do{
		iSingleRead = (int)fread(lpPwd->sPassword + iTotalRead, sizeof(char), HARDKEYBUFSZ, fSource);
		DfltCode.Cipher(lpPwd->sPassword + iTotalRead, iSingleRead);
		iTotalRead += iSingleRead;
	}while(iSingleRead == HARDKEYBUFSZ);

	fclose(fSource);
	DfltCode.Destroy();

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
bool CreateKeyFile(HWND hST, HWND hPB, const char *sFileName, const char *sPwd, int iPwdLen, int iKeyGenLen)
{
	char sBits[64];
	char sBuf[HARDKEYBUFSZ + 1];
	char sOfBits[64];
	char sStatus[1024];

    long lChecksum = 0;

	FILE *fTarget = NULL;

	float fPercentComplete = 0;

	int iPos = 0;
	int iSeg = 0;
	int iEncryptedGenLen = 0;
	unsigned long ulDigest[5];

	NASCCLStream DfltCode;
	SHA1 SHA;

    memset(&DfltCode, 0, sizeof(DfltCode));

	if(fopen_s(&fTarget, sFileName, "wb") != 0)
	{
		SafeMsgBox(hHardKeyStatDialog, "Failed to create the HardKey file.\r\n"
			"Do you have permission to access the target location?", gsTitleCaption, MB_ICONERROR);
		fclose(fTarget);
		return false;
	}

	FormatInteger(sOfBits, sizeof(sOfBits), iKeyGenLen * 8);

    DfltCode.Initialize((char *)sPwd, iPwdLen, false);

	//unsigned long FullCRC(unsigned char *sData, unsigned long ulLength);

	srand(gCRC32->FullCRC((unsigned char *)sPwd, iPwdLen) + GetTickCount());

	//Write the Application Checksum.
	lChecksum = MakeChecksum(gsTitleCaption, (int)strlen(gsTitleCaption));
	fwrite(&lChecksum, sizeof(lChecksum), 1, fTarget);

	//Write the Application Version Checksum.
	lChecksum = MakeChecksum(gsCompatibility, (int)strlen(gsCompatibility));
	fwrite(&lChecksum, sizeof(lChecksum), 1, fTarget);

	//Write the KeySize in Bytes.
	DfltCode.Cipher(&iKeyGenLen, &iEncryptedGenLen, sizeof(iKeyGenLen));
	fwrite(&iEncryptedGenLen, sizeof(iEncryptedGenLen), 1, fTarget);

	//Write the Password Checksum (Begin)
	SHA.Input(sPwd, iPwdLen);
	SHA.Result(ulDigest);
	SHA.Reset();
	for(int i = 0; i < 5 ; i++)
	{
		DfltCode.Cipher(&ulDigest[i], &ulDigest[i], sizeof(ulDigest[i]));
		if(fwrite(&ulDigest[i], sizeof(unsigned int), 1, fTarget) != 1)
		{
			SafeMsgBox(hHardKeyStatDialog, "Failed to write key hash.", gsTitleCaption, MB_ICONINFORMATION);
			fclose(fTarget);
			DfltCode.Destroy();
			return false;
		}
	}
	//Write the Password Checksum (End)

	while(iPos < iKeyGenLen)
	{
		for(iSeg = 0; iSeg < HARDKEYBUFSZ && iPos < iKeyGenLen; iSeg++)
		{
			sBuf[iSeg] = rand();
			iPos++;
		}

		DfltCode.Cipher(sBuf, iSeg);

		if(fwrite(sBuf, sizeof(char), iSeg, fTarget) != iSeg)
		{
			SafeMsgBox(hHardKeyStatDialog, "Failed to write key value.", gsTitleCaption, MB_ICONINFORMATION);
			fclose(fTarget);
			DfltCode.Destroy();
			return false;
		}

		fPercentComplete = (((float)iPos) / ((float)iKeyGenLen)) * 100.0f;
		SendMessage(hPB, PBM_SETPOS, (DWORD)fPercentComplete, 0);

		FormatInteger(sBits, sizeof(sBits), iPos * 8);
		sprintf_s(sStatus, sizeof(sStatus), "%s of %s Bits / %.2f%%", sBits, sOfBits, fPercentComplete);
		Set_Text(hST, sStatus);

		Sleep(10);
	}

	fclose(fTarget);
	DfltCode.Destroy();

	return true;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
