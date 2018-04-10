#ifndef _CParallelProcessing_H
#define _CParallelProcessing_H
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define WORKLOAD_BUFFER_SIZE		524288 // 1/2 MB file buffer size.

#define THREAD_STATUS_INITIALIZING	0 //The thread is initializing.
#define THREAD_STATUS_ACTIVE		1 //The thread is processing a workload.
#define THREAD_STATUS_SLEEPING		2 //The thread is currently sleeping.
#define THREAD_STATUS_DEACTIVATED	4 //The thread has execution has ended.

#define WORKLOAD_STATUS_ERROR		0 //An error has occured, cleanup and return.
#define WORKLOAD_STATUS_EMPTY		1 //Nothing to do, sleep for awhile.
#define WORKLOAD_STATUS_COMPLETE	2 //All workload items are complete, cleanup and return.
#define WORKLOAD_STATUS_OK			3 //Returned a valid workload item, process it.

typedef struct _tag_Parallel_Processing_Thread{
	HANDLE Handle;
	DWORD ID;
	HANDLE SyncHandle;
	DWORD IdealProcessor;
	unsigned short Slot; //Array position of this struct in the PARALLEL_PROCESSING_THREADS struct.
	unsigned short Status;
	LPVOID Class; //Pointer to the owner class object.
} PARALLEL_PROCESSING_THREAD, *LPPARALLEL_PROCESSING_THREAD;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _tag_Parallel_Processing_Threads{
	PARALLEL_PROCESSING_THREAD *Thread;
	unsigned short Count;
	unsigned short ActiveCount;
	HANDLE DataReadyHintEvent;
	HANDLE AllThreadsInactive;
	HANDLE WorkloadCompleteEvent;
} PARALLEL_PROCESSING_THREADS, *LPPARALLEL_PROCESSING_THREADS;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _tag_Parallel_Processing_Info{
	DWORD Processors;
	DWORD ProcessAffinityMask;
	DWORD SystemAffinityMask;
	DWORD ActiveProcessorMask;

	PARALLEL_PROCESSING_THREADS Threads;
} PARALLEL_PROCESSING_INFO, *LPPARALLEL_PROCESSING_INFO;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _tag_Parallel_Processing_Workload{

	char *SourceBuf;
	char *TargetBuf;

	int SourceAlloc;
	int TargetAlloc;

	DWORD BytesRead;
	DWORD BytesWritten;

	BOOL Result;

} PARALLEL_PROCESSING_WORKLOAD, *LPPARALLEL_PROCESSING_WORKLOAD;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _tag_Parallel_Processing_Workload_Info{

	HANDLE SourceHandle; //Source file handle.
	HANDLE TargetHandle; //Target file handle.
	unsigned short Status;

} PARALLEL_PROCESSING_WORKLOAD_INFO, *LPPARALLEL_PROCESSING_WORKLOAD_INFO;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CParallelProcessing {

public:
	bool Destroy(void);
	bool Initialize(void);
	bool Process(HANDLE hSource, HANDLE hTarget);

private:
	PARALLEL_PROCESSING_INFO PPI;
	PARALLEL_PROCESSING_WORKLOAD_INFO WLI;

	unsigned short ConsumeWorkload(PARALLEL_PROCESSING_WORKLOAD *pWorkload);

	friend DWORD WINAPI CParallelProcessing_Worker(LPVOID pVoid);
	CRITICAL_SECTION CS;

	void Lock(void);
	void UnLock(void);
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
