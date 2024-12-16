///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright © NetworkDLS 2002, All rights reserved
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _NASCCLBlock_Cpp_
#define _NASCCLBlock_Cpp_
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "NASCCL.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool NASCCLBlock::Clone(NASCCLBlock *pSource)
{
	this->bAutoReset = pSource->bAutoReset;

	srand(GetTickCount()); //Random functions will be used in AllocKey() & FreeKey().

	if(!this->DuplicateKey(&pSource->Key, &this->Key))
	{
		this->FreeKey(&this->Key);
		return false;
	}

	if(!this->DuplicateKey(&this->Key, &this->DupKey))
	{
		this->FreeKey(&this->Key);
		return false;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	Allocates RAM, stores and hashes the given input data.

	Notes:
	1) If the input is less than 1,024 bytes, it will be hashed out to 1024.
	2) The given key will also be hashed out to fill the required MOD 8 key length.
*/
bool NASCCLBlock::AllocKey(NASCCLKEY *pKey, const NBYTE *pRawKey, unsigned int iRawKeyLength)
{
	pKey->Text = NULL;
	pKey->Size = 0;
	pKey->Char = 0;
	pKey->Box = 0;
	pKey->Block = 0;

	if(iRawKeyLength >= 1024)
	{
		pKey->Size = iRawKeyLength + (sizeof(NBlock) - (iRawKeyLength % sizeof(NBlock)));
	}
	else{
		pKey->Size = 1024;
	}

	if((pKey->Size % sizeof(NBlock)) != 0)
	{
		return false;
	}

	if((pKey->Text = (NBYTE *) calloc(pKey->Size, sizeof(NBYTE))) == NULL)
	{
		return false;
	}

	HashKey(pRawKey, iRawKeyLength, pKey);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	Destroys the given key by overwriting it with random
		data before freeing the RAM back to the public pool.
*/
bool NASCCLBlock::FreeKey(NASCCLKEY *pKey)
{
	if(pKey->Text)
	{
		int iRand = rand();

		for(unsigned int iWPos = 0; iWPos < pKey->Size; iWPos++)
		{
			while(!(iRand = rand()))
			{
				//Create non-zero random value.
			}

			for(int iRPos = NASCCL_NUKE_ROUNDS; iRPos != -1; iRPos--)
			{
				//The last set operation will set byte to Zero (Rand * 0).
				pKey->Text[iWPos] = (NBYTE) (iRand * iRPos);
			}
		}

		free(pKey->Text);

		pKey->Text = NULL;
		pKey->Size = 0;
		pKey->Block = 0;
		pKey->Char = 0;

		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	Makes a perfect copy of the given key.

	Notes:
	1) This function will allocate & initialize everything, there are no prerequisites for pDest.
*/
bool NASCCLBlock::DuplicateKey(const NASCCLKEY *pSource, NASCCLKEY *pDest)
{
	if((pDest->Text = (NBYTE *) calloc(pSource->Size, sizeof(NBYTE))) == NULL)
	{
		return false;
	}

	pDest->Block = pSource->Block;
	pDest->Char = pSource->Char;
	pDest->Box = pSource->Box;
	pDest->Size = pSource->Size;

	memcpy(pDest->Text, pSource->Text, pSource->Size);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	See originating Initialize() overload.
*/
NASCCLBlock::NASCCLBlock(const void *pKey, unsigned int iKeySz, bool bInAutoReset)
{
	this->Initialize(pKey, iKeySz, bInAutoReset);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	See originating Initialize() overload.
*/
NASCCLBlock::NASCCLBlock(const void *pKey, unsigned int iKeySz)
{
	this->Initialize(pKey, iKeySz, true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

NASCCLBlock::NASCCLBlock(void)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	This function initializes the class and calls the appropriate functions to prepare the key.

	Notes:
	This function must be called (and succeed) before calling any of the other class functions.
*/
bool NASCCLBlock::Initialize(const void *pKey, unsigned int iKeySz, bool bInAutoReset)
{
	this->bAutoReset = bInAutoReset;

	srand(GetTickCount()); //Random functions will be used in AllocKey() & FreeKey().

	if(!this->AllocKey(&this->Key, (const NBYTE *)pKey, iKeySz))
	{
		return false;
	}

	if(!this->DuplicateKey(&this->Key, &this->DupKey))
	{
		this->FreeKey(&this->Key);
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	See originating Initialize() overload.
*/
bool NASCCLBlock::Initialize(const void *pKey, unsigned int iKeySz)
{
	return this->Initialize(pKey, iKeySz, false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	See originating Initialize() overload.
*/
bool NASCCLBlock::Initialize(const char *sKey)
{
	return this->Initialize(sKey, (int)strlen(sKey), false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	See originating Initialize() overload.
*/
bool NASCCLBlock::Initialize(const char *sKey, bool bInAutoReset)
{
	return this->Initialize(sKey, (int)strlen(sKey), bInAutoReset);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	Resets the internal state of the class back to the original initialized state.

	Notes:
	1) This function should be called before switching from encrypting to decrypting (or vise-versa).
	2) This function should be called between encrypting logical blocks of data. For example: If you
		encrypt 2 files and DO NOT call this function between them... You would have to decrypt them
		in the same order that you encrypted them.
*/

void NASCCLBlock::Reset(void)
{
	//Reset key counters.
	this->Key.Box = 0;
	this->Key.Char = 0;
	this->Key.Block = 0;

	//Reset the key bytes with the original.
	memcpy(this->Key.Text, this->DupKey.Text, this->DupKey.Size);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	This is the main encipherment / decipherment routine.

	Notes:
	1) This function is used for both "Encrypting" and "Decrypting" data.
*/
void NASCCLBlock::Cipher(const void *pIn, const void *pOut, unsigned int iInSz)
{
	NBlock iBlock;
	NBlock *pKeyBlock1;
	NBlock *pKeyBlock2;
	unsigned short iReadSz = sizeof(iBlock);

	if(this->bAutoReset)
	{
		this->Reset();
	}

	for(unsigned int iDatPos = 0; iDatPos < iInSz; iDatPos += iReadSz)
	{
		if((iInSz - iDatPos) < iReadSz)
		{
			//Since we don't have enough data to fill the data buffer, reset it's bits to zero.
			iBlock = 0;

			//Calculate our remaining bytes.
			iReadSz = (iInSz - iDatPos);

			//Copy the remaining bytes to the buffer.
			memcpy(&iBlock, ((NBYTE *)pIn) + iDatPos, iReadSz);
		}
		else{
			iBlock = (*(NBlock *)(((NBYTE *)pIn) + iDatPos));
		}

		pKeyBlock1 = ((NBlock *)(((NBYTE *)this->Key.Text) + this->Key.Block));
		pKeyBlock2 = ((NBlock *)(((NBYTE *)this->Key.Text) + (this->Key.Size - this->Key.Block)));

		//Calculate the output text.
		iBlock ^= (((NBlock)*pKeyBlock1) ^ gsHardSalt[this->Key.Box++][this->Key.Text[this->Key.Char++]]);

		//Push the bytes back to the caller. 
		memcpy(((NBYTE *)pOut) + iDatPos, &iBlock, iReadSz);

		//Scramble our key.
		((NBlock)*pKeyBlock1) ^= (((NBlock)*pKeyBlock2) * (iDatPos + this->Key.Block));

		//Increment our key bytes - If we have exhausted our Key Bytes, reset back to the beginning.
		if((this->Key.Block += iReadSz) + iReadSz > this->Key.Size)
		{
			this->Key.Block = 0;
		}

		//If we have exhausted our boxs, reset back to the beginning.
		if(this->Key.Box == NASCCL_SALT_BOXES)
		{
			this->Key.Box = 0;
		}

		//If we have exhausted our key bytes, reset back to the beginning.
		if(this->Key.Char == this->Key.Size)
		{
			this->Key.Char = 0;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	See originating Cipher() overload.
*/
void NASCCLBlock::Cipher(void *pInAndOut, unsigned int iInSz)
{
	this->Cipher(pInAndOut, pInAndOut, iInSz);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	Destroys and frees the class variables.

	Notes:
	1) This function should be called when you are done using the class.
*/
bool NASCCLBlock::Destroy(void)
{
	bool bResult = true;

	bResult = (bResult && this->FreeKey(&this->Key));
	bResult = (bResult && this->FreeKey(&this->DupKey));

	return bResult;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
