///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright © NetworkDLS 2002, All rights reserved
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _NASCCLStream_Cpp_
#define _NASCCLStream_Cpp_
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "NASCCL.H"

#ifdef _USE_GLOBAL_MEMPOOL
#include "../NSWFL/NSWFL.h"
using namespace NSWFL::Memory;
extern MemoryPool *pMem; //pMem must be defined and initalized elsewhere.
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void NASCCLStream::MutateKey(void)
{
	this->Cipher(sKeyBuf, iKeySz); //Scramble the users key.
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void NASCCLStream::BuildSBox(void)
{
	NBYTE cValue = 0;

	for(int ilBox = 0; ilBox < NASCCL_SALT_BOXES; ilBox++)
	{
		for(int ilVal = 0; ilVal < NASCCL_SALT_VALUES; ilVal++)
		{
			if(iKeyPos == iKeySz)
			{
				iKeyPos = 0;
			}

			cValue = (this->sKeyBuf[iKeyPos] * ((ilVal + 1) * (ilBox + 1)))
				^ gsHardSalt[ilBox][(this->sKeyBuf[iKeyPos] ^ cValue) % 255];

			this->sKeySalt[ilBox][ilVal] = cValue;

			iKeyPos++;
		}
	}

	memcpy(&this->sPreservedKeySalt, &this->sKeySalt, sizeof(this->sKeySalt));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

NASCCLStream::NASCCLStream(const void *lpKey, unsigned int iKeySz, bool bAutoReset)
{
	this->Initialize((NBYTE *)lpKey, iKeySz, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

NASCCLStream::NASCCLStream(const void *lpKey, unsigned int iKeySz)
{
	this->Initialize((NBYTE *)lpKey, iKeySz, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

NASCCLStream::NASCCLStream(void)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

NASCCLStream::NASCCLStream(const char *sKey)
{
	this->Initialize((NBYTE *)sKey, (int)strlen(sKey), false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool NASCCLStream::Initialize(const char *sKey)
{
	return this->Initialize((NBYTE *)sKey, (int)strlen(sKey), false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool NASCCLStream::Initialize(const void *lpKey, unsigned int iINKeySz)
{
	return this->Initialize((NBYTE *)lpKey, iINKeySz, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool NASCCLStream::Initialize(const void *lpKey, unsigned int iINKeySz, bool bAutoReset)
{
	unsigned int iPos = 0;

	this->ZeroKeyBox();

	//Save the size of the users key.
	iKeyPos = (iINKeySz - 1);
	iKeySz = iINKeySz;
	iSBox = 0;

	this->bReset = false;

	//Allocate enough RAM to store the users key.
	#ifdef _USE_GLOBAL_MEMPOOL
		this->sKeyBuf = (NBYTE *) pMem->Allocate(sizeof(NBYTE), iKeySz + 1);
	#else
		this->sKeyBuf = (NBYTE *) calloc(sizeof(NBYTE), iKeySz + 1);
	#endif

	if(this->sKeyBuf)
	{
		#ifdef _USE_GLOBAL_MEMPOOL
			this->sPreservedKeyBuf = (NBYTE *) pMem->Allocate(sizeof(NBYTE), iKeySz + 1);
		#else
			this->sPreservedKeyBuf = (NBYTE *) calloc(sizeof(NBYTE), iKeySz + 1);
		#endif

		if(this->sPreservedKeyBuf)
		{
			memcpy(this->sKeyBuf, lpKey, iKeySz);
			memcpy(this->sPreservedKeyBuf, lpKey, iKeySz);

			this->BuildSBox();
			this->MutateKey();

			bReset = bAutoReset;

			this->Reset();
		}

		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void NASCCLStream::Reset(void)
{
	iKeyPos = (iKeySz - 1);
	iSBox = 0;

	memcpy(&this->sKeySalt, &this->sPreservedKeySalt, sizeof(this->sKeySalt));
	memcpy(this->sKeyBuf, this->sPreservedKeyBuf, iKeySz);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void NASCCLStream::Destroy(void)
{
	//Free the RAM used by the copy of the users key.
	memset(sKeyBuf, 0, iKeySz);
	#ifdef _USE_GLOBAL_MEMPOOL
		pMem->Free(sKeyBuf);
	#else
		free(sKeyBuf);
	#endif
	sKeyBuf = NULL;

	memset(sPreservedKeyBuf, 0, iKeySz);
	#ifdef _USE_GLOBAL_MEMPOOL
		pMem->Free(sPreservedKeyBuf);
	#else
		free(sPreservedKeyBuf);
	#endif
	sPreservedKeyBuf = NULL;

	//ZERO out the size of the users key.
	iKeySz = 0;
	iKeyPos = 0;
	iSBox = 0;

	this->ZeroKeyBox();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void NASCCLStream::ZeroKeyBox(void)
{
	for(int ilBox = 0; ilBox < NASCCL_SALT_BOXES; ilBox++)
	{
		for(int ilVal = 0; ilVal < NASCCL_SALT_VALUES; ilVal++)
		{
			this->sKeySalt[ilBox][ilVal] = 0;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void NASCCLStream::Cipher(const void *lpSource, void *lpTarget, unsigned int iBufSz)
{
	if(bReset)
	{
		this->Reset();
	}

	for(unsigned int iPos = 0; iPos < iBufSz; iPos++)
	{
		if(iKeyPos == -1)
		{
			iKeyPos = (iKeySz - 1);
		}

		if(iSBox == NASCCL_SALT_BOXES)
		{
			iSBox = 0;
		}

		NASCCLStreamSWAP(this->sKeySalt[iSBox][((NBYTE *)this->sKeyBuf)[iKeyPos]],
			((NBYTE *)this->sKeyBuf)[iKeyPos]);

		((NBYTE *)lpTarget)[iPos] =
			((NBYTE *)lpSource)[iPos] ^ this->sKeySalt[iSBox][((NBYTE *)this->sKeyBuf)[iKeyPos]];

		iKeyPos--;
		iSBox++;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void NASCCLStream::Cipher(void *lpBuf, unsigned int iBufSz)
{
	this->Cipher(lpBuf, lpBuf, iBufSz);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
