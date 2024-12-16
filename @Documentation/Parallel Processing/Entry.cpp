#ifndef _Entry_Cpp
#define _Entry_Cpp
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <StdIO.H>
#include <StdLib.H>

#include "CParallelProcessing.H"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	HANDLE hTarget = NULL;
	HANDLE hSource = NULL;

	hSource = CreateFile("C:\\TestNotes.SAef",
		GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hSource == INVALID_HANDLE_VALUE)
	{
		printf("Failed to open target file.");
		return 0;
	}

	hTarget = CreateFile("C:\\TargetNotes.SAef",
		GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hTarget == INVALID_HANDLE_VALUE)
	{
		printf("Failed to open target file.");
		return 0;
	}

	CParallelProcessing PE;

	PE.Initialize();

	PE.Process(hSource, hTarget);

	PE.Destroy();

	system("Pause");

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
