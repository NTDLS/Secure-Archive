///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright © NetworkDLS 2002, All rights reserved
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _NASCCLStream_H_
#define _NASCCLStream_H_
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class NASCCLStream{

	public:
		NASCCLStream(void);
    	NASCCLStream(const char *sKey);
		NASCCLStream(const void *lpKey, unsigned int iKeySz);
		NASCCLStream(const void *lpKey, unsigned int iKeySz, bool bAutoReset);

		bool Initialize(const void *lpKey, unsigned int iKeySz, bool bAutoReset);
		bool Initialize(const void *lpKey, unsigned int iKeySz);
    	bool Initialize(const char *sKey);

    	void Destroy(void);
    	void Cipher(void *lpBuf, unsigned int iBufSz);
    	void Cipher(const void *lpSource, void *lpTarget, unsigned int iBufSz);
    	void Reset(void);
    	void MutateKey(void);

		NBYTE sKeySalt[NASCCL_SALT_BOXES][NASCCL_SALT_VALUES];
		NBYTE sPreservedKeySalt[NASCCL_SALT_BOXES][NASCCL_SALT_VALUES];

	private:
		bool bReset;

    	void BuildSBox(void);
		void ZeroKeyBox(void);

    	NBYTE *sKeyBuf;
    	NBYTE *sPreservedKeyBuf;
		
    	unsigned int iKeySz;
    	unsigned int iKeyPos;
    	unsigned int iSBox;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
