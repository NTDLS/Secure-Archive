///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright © NetworkDLS 2002, All rights reserved
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _NASCCLBlock_H_
#define _NASCCLBlock_H_
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class NASCCLBlock {

public:

	NASCCLBlock(const void *pKey, unsigned int iKeySz, bool bInAutoReset);
	NASCCLBlock(const void *pKey, unsigned int iKeySz);
	NASCCLBlock(void);

	bool Initialize(const void *pKey, unsigned int iKeySz, bool bInAutoReset);
	bool Initialize(const void *pKey, unsigned int iKeySz);
	bool Initialize(const char *sKey, bool bInAutoReset);
	bool Initialize(const char *sKey);
	bool Clone(NASCCLBlock *pSource);

	void Reset(void);
	bool Destroy(void);

	void Cipher(const void *pIn, const void *pOut, unsigned int iInSz);
	void Cipher(void *pInAndOut, unsigned int iInSz);

private:
	bool DuplicateKey(const NASCCLKEY *pSource, NASCCLKEY *pDest);
	bool FreeKey(NASCCLKEY *pKey);
	bool AllocKey(NASCCLKEY *pKey, const NBYTE *pRawKey, unsigned int iRawKeyLength);

	bool bAutoReset;

	NASCCLKEY Key;
	NASCCLKEY DupKey;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
