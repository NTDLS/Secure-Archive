#ifndef _WORKERS_H
#define _WORKERS_H
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
DWORD WINAPI CreateFolderArchiveProc(LPVOID lpVoid);
DWORD WINAPI ViewExistingProc(LPVOID lpVoid);
DWORD WINAPI CreateFileArchiveProc(LPVOID lpVoid);
DWORD WINAPI AddFilesProc(LPVOID lpVoid);
DWORD WINAPI DeleteArchiveFilesProc(LPVOID lpVoid);
DWORD WINAPI CreateSelfExtractorProc(LPVOID lpVoid);
DWORD WINAPI RepairCorruptArchiveProc(LPVOID lpVoid);
DWORD WINAPI TestArchiveProc(LPVOID lpVoid);
#endif
DWORD WINAPI ExtractEntireArchiveProc(LPVOID lpVoid);

bool CreateWorker(LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern HANDLE hWorkerProc;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
