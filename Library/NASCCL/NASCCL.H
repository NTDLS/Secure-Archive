///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright © NetworkDLS 2002, All rights reserved
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _NASCCL_H_
#define _NASCCL_H_
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <Stdlib.H>
#include <Math.H>

#define NASCCL_NUKE_ROUNDS		3
#define NASCCL_SALT_BOXES		64
#define NASCCL_SALT_VALUES		256
#define NASCCLStreamSWAP(x,y)   x ^= y ^= x ^= y

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef unsigned char NBYTE;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum KeyWeight {
	KW_UNACCEPTABLE,
	KW_VERYWEAK,
	KW_WEAK,
	KW_ACCEPTABLE,
	KW_STRONG,
	KW_VERYSTRONG,
	KW_EXCELLENT
};

typedef struct NASCCL_KEY_TYPE {
	NBYTE *Text;
	unsigned int Size;
	unsigned int Block;
	unsigned int Char;
	unsigned int Box;
} NASCCLKEY, *LPNASCCLKEY;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef unsigned char NBYTE;
typedef unsigned __int64 NBlock;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern const NBYTE gsHardSalt[NASCCL_SALT_BOXES][NASCCL_SALT_VALUES];
extern const char *gsKeyWeights[];

void CipherKey(const void *sRawKey, void *sKeyHash, int iRawKeySz);
int KeyWeight(const char *sKey);
int KeyWeight(const void *lpKey, int iKeySz);
const char * KeyWeightString(const void *lpKey, int iKeySz);
const char * KeyWeightString(const char *sKey);
void HashKey(const char *sRawKey, void *sKeyHash, int iKeyHashSz);
void HashKey(const void *sRawKey, int iRawKeySz, void *sKeyHash, int iKeyHashSz);
void HashKey(const NBYTE *pKey, unsigned int iKeySz, NASCCLKEY *pOutKey);


#include "NASCCLBlock.H"
#include "NASCCLStream.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
