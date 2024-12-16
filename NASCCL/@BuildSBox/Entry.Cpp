////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define _CRT_SECURE_NO_WARNINGS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <StdIO.H>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int RandVal(unsigned int iAtLeast, unsigned int iAtMost)
{
    return ((rand() % (iAtMost - iAtLeast)) + iAtLeast);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

char *MakeHex(unsigned int iValue, char *sHex, int iHexSz)
{
	char sValue[100];
	sprintf(sValue, "%X", iValue);
	strcpy(sHex, "0x");

	for(int iLen = strlen(sValue); iLen < 2; iLen++)
	{
		strcat(sHex, "0");
	}

	strcat(sHex, sValue);

	return sHex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool BuildSBox(const char *sFileName, unsigned int iBoxes, unsigned int iValues)
{
	FILE *hTarget = fopen(sFileName, "wb");
	if(!hTarget)
	{
		printf("Failed to open sBox target file.\n");
		return false;
	}
	
	char sHex[255];

	srand(GetTickCount());

	fprintf(hTarget, "const unsigned char gsSalt[SALT_BOXES][SALT_VALUES] = \r\n");
	fprintf(hTarget, "{\r\n");
	for(unsigned int iBox = 0; iBox < iBoxes; iBox++)
	{
		fprintf(hTarget, "\t{\r\n");
		for(unsigned int iValue = 0; iValue < iValues; iValue++)
		{
			if(iValue % 8 == 0 && iValue != 0)
			{
				fprintf(hTarget, "\r\n\t\t");
			}
			else if(iValue == 0)
			{
				fprintf(hTarget, "\t\t");
			}

			fprintf(hTarget, "%s", MakeHex(RandVal(1, 255), sHex, sizeof(sHex)));

			if(iValue != iValues-1)
			{
				fprintf(hTarget, ", ");
			}
		}

		fprintf(hTarget, "\r\n\t}");

		if(iBox != iBoxes-1)
		{
			fprintf(hTarget, ",");
		}

		fprintf(hTarget, "\r\n");
	}
	fprintf(hTarget, "};\r\n");

	fclose(hTarget);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	BuildSBox("C:\\SBoxes.cpp", 64, 256);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
