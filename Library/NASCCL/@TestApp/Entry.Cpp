///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <Stdlib.H>

#include "../NASCCL.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define BUFFERSZ 1048576

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Cipher(const char *sSource, const char *sTarget, const char *sPwd)
{
	FILE *hSource = NULL;
	FILE *hTarget = NULL;

	NASCCLStream N;

	int iBytesRead = 0;

	printf("The password used is [%s].\n", KeyWeightString(sPwd));

	char *sBuffer = (char *) calloc(sizeof(char), BUFFERSZ + 1);
	if(sBuffer == NULL)
	{
		printf("Failed to allocate %d bytes of RAM. Out of memory?\n", BUFFERSZ);
		return false;
	}

	if(fopen_s(&hSource, sSource, "rb") != 0)
	{
		printf("Failed to open [%s] for read access.\n", sSource);
		free(sBuffer);
		return false;
	}

	if(fopen_s(&hTarget, sTarget, "wb") != 0)
	{
		printf("Failed to open [%s] for write access.\n", sTarget);
		fclose(hSource);
		free(sBuffer);
		return false;
	}

	N.Initialize(sPwd);

	do{
		iBytesRead = fread(sBuffer, sizeof(char), BUFFERSZ, hSource);

		N.Cipher(sBuffer, iBytesRead);

		if(fwrite(sBuffer, sizeof(char), iBytesRead, hTarget) != iBytesRead)
		{
			printf("Failed to write data to the disk. Out of free space?\n");
			break;
		}
	} while(iBytesRead == BUFFERSZ);

	N.Destroy();

	fclose(hSource);
	fclose(hTarget);

	free(sBuffer);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void PrintKeyWeight(char *sKey)
{
	printf("[%s] is [%s]\n", sKey, KeyWeightString(sKey));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void NASCCL_Block_Timed(void)
{
	double dSamples[100];

	NASCCLBlock N;

	N.Initialize("My-Short-Key");

	int iBufferSize = (1024 * 1024) * 5;
	int iMaxSamples = sizeof(dSamples) / sizeof(double);

	char *sSource = (char *)calloc(iBufferSize, sizeof(char));
	char *sTarget = (char *)calloc(iBufferSize, sizeof(char));
	char *sOrigin = (char *)calloc(iBufferSize, sizeof(char));

	srand(GetTickCount());

	for (int iWPos = 0; iWPos < iBufferSize; iWPos++)
	{
		sSource[iWPos] = (char)rand();
		if (sSource[iWPos] == 0)
		{
			iWPos--;
		}
	}

	memcpy(sOrigin, sSource, iBufferSize);

	double dBefore = 0;
	double dAfter = 0;
	int iRounds = 20;

	printf("NASCCL Block:\n");
	printf("Please Wait...\r");

	for (int iSamples = 0; iSamples < iMaxSamples;)
	{
		dBefore = (double)GetTickCount();
		for (int iRound = 0; iRound < iRounds; iRound++)
		{
			N.Cipher(sSource, sTarget, iBufferSize); //Encrypt.
			N.Reset();								 //Reset.

			N.Cipher(sTarget, sSource, iBufferSize); //Decrypt.
			N.Reset();								 //Reset.
		}
		dAfter = (double)GetTickCount();

		for (int iPos = 0; iPos < iBufferSize; iPos++)
		{
			if (sSource[iPos] != sOrigin[iPos])
			{
				printf("Bad CRC!\n");
				N.Destroy();
				return;
			}
		}

		dSamples[iSamples++] = ((1000.0 / ((dAfter - dBefore) / ((double)iRounds)))
			* ((double)(iBufferSize / (1024.0*1024.0)))) * 2.0;

		double dResult = 0;

		for (int iSamp = 0; iSamp < iSamples; iSamp++)
		{
			dResult += dSamples[iSamp];
		}

		printf("%.2fMB/s (%d of %d)              \r", dResult / (double)iSamples, iSamples, iMaxSamples);
	}

	printf("\n\n");

	N.Destroy();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void NASCCL_Stream_Timed(void)
{
	double dSamples[100];

	NASCCLStream N;

	N.Initialize("My-Short-Key");

	int iBufferSize = (1024 * 1024) * 5;
	int iMaxSamples = sizeof(dSamples) / sizeof(double);

	char *sSource = (char *)calloc(iBufferSize, sizeof(char));
	char *sTarget = (char *)calloc(iBufferSize, sizeof(char));
	char *sOrigin = (char *)calloc(iBufferSize, sizeof(char));

	srand(GetTickCount());

	for(int iWPos = 0; iWPos < iBufferSize; iWPos++)
	{
		sSource[iWPos] = (char)rand();
		if(sSource[iWPos] == 0)
		{
			iWPos--;
		}
	}

	memcpy(sOrigin, sSource, iBufferSize);

	double dBefore = 0;
	double dAfter = 0;
	int iRounds = 20;

	printf("NASCCL Stream:\n");
	printf("Please Wait...\r");

	for(int iSamples = 0; iSamples < iMaxSamples;)
	{
		dBefore = (double)GetTickCount();
		for(int iRound = 0; iRound < iRounds; iRound++)
		{
			N.Cipher(sSource, sTarget, iBufferSize); //Encrypt.
			N.Reset();

			N.Cipher(sTarget, sSource, iBufferSize); //Decrypt.
			N.Reset();
		}
		dAfter = (double)GetTickCount();

		for(int iPos = 0; iPos < iBufferSize; iPos++)
		{
			if(sSource[iPos] != sOrigin[iPos])
			{
				printf("Bad CRC!\n");
				N.Destroy();
				return;
			}
		}

		dSamples[iSamples++] = ((1000.0 / ((dAfter - dBefore) / ((double)iRounds)))
								* ((double)(iBufferSize / (1024.0*1024.0)))) * 2.0;
		
		double dResult = 0;

		for(int iSamp = 0; iSamp < iSamples; iSamp++)
		{
			dResult += dSamples[iSamp];
		}

		printf("%.2fMB/s (%d of %d)              \r", dResult / (double)iSamples, iSamples, iMaxSamples);
	}

	printf("\n\n");

	N.Destroy();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{

	NASCCL_Stream_Timed();
	NASCCL_Block_Timed();

	//char *sPwd = "1";
	//Cipher("C:\\A's.txt", "C:\\E-A's.txt", sPwd);
	//Cipher("C:\\E-A's.txt", "C:\\D-A's.txt", sPwd);

	PrintKeyWeight("test");
	PrintKeyWeight("Test");
	PrintKeyWeight("test1");
	PrintKeyWeight("Test1");
	PrintKeyWeight("Test-1");
	PrintKeyWeight("Scuba663");
	PrintKeyWeight("46&2");
	PrintKeyWeight("Fourty6&2!");
	PrintKeyWeight("FourtySix&Two");
	PrintKeyWeight("ThisIs1ReallyLongPassword!");

	system("Pause");

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
