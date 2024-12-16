///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright © NetworkDLS 2002, All rights reserved
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _NASCCL_Cpp_
#define _NASCCL_Cpp_
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "NASCCL.H"


#ifdef _USE_GLOBAL_MEMPOOL
#include "../NSWFL/NSWFL.h"
using namespace NSWFL::Memory;
extern MemoryPool *pMem; //pMem must be defined and initalized elsewhere.
#endif

const char *gsKeyWeights[] = {
	"Unacceptable",
	"Very Weak",
	"Weak",
	"Acceptable",
	"Strong",
	"Very Strong",
	"Excellent",
	NULL
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const char * KeyWeightString(const char *sKey)
{
	return gsKeyWeights[KeyWeight(sKey)];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const char * KeyWeightString(const void *lpKey, int iKeySz)
{
	return gsKeyWeights[KeyWeight(lpKey, iKeySz)];
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int KeyWeight(const void *lpKey, int iKeySz)
{
	int iV = 0;
	int iNums = 0;
	int iSymbols = 0;
	int iCapChars = 0;
	int iLowChars = 0;
	int iExtChars = 0;
	int iSeg = (iKeySz / 8); //How many 8-character segments is the password comprised of?

	//Figure out how many different types of characters are used in the password.
	for (int iRPos = 0; iRPos < iKeySz; iRPos++)
	{
		iV = ((NBYTE *)lpKey)[iRPos];

		if (iV >= 48 && iV <= 57) {
			iNums++;
		}
		else if (iV >= 65 && iV <= 90) {
			iCapChars++;
		}
		else if (iV >= 97 && iV <= 121) {
			iLowChars++;
		}
		else if ((iV >= 32 && iV <= 47) || (iV >= 58 && iV <= 64) || (iV >= 91 && iV <= 96) || (iV >= 123 && iV <= 126)) {
			iSymbols++;
		}

		else {
			iExtChars++;
		}
	}

	//The score is based first on the number of 8-character segments in the password.
	//	If the password is comprised of more than 6 of these segments then set the
	//		score to 6, there will be no extra credit for a really-really long password
	//		as it could simply be a string of 1 million A's.
	if ((iV = iSeg) > KW_EXCELLENT)
	{
		iV = KW_EXCELLENT;
	}

	//Take one point away from the score for each character type that is not used.
	//Also, we do not take away credit for not using extended characters... but we do give it.
	iV -= ((iSeg == 0) + (iNums == 0) + (iSymbols == 0) + (iCapChars == 0) + (iLowChars == 0));

	//We do not allow the score to go below -1 before we start to
	//	credit the score. So if the score is below -1 reset it to -1.
	if (iV < -1)
	{
		iV = -1;
	}

	//Give one point for each character type that is used.
	iV += ((iNums>0) + (iSymbols>0) + (iCapChars>0) + (iLowChars>0) + (iExtChars>0));

	if (iV < KW_UNACCEPTABLE)
	{
		return KW_UNACCEPTABLE;
	}
	else if (iV > KW_EXCELLENT)
	{
		return KW_EXCELLENT;
	}
	else {
		return iV;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int KeyWeight(const char *sKey)
{
	return KeyWeight(sKey, (int) strlen(sKey));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

