#ifndef _CParallelProcessing_Cpp
#define _CParallelProcessing_Cpp
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <StdIO.H>
#include <StdLib.H>

#include "CParallelProcessing.H"
#include "Helper.H"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI CParallelProcessing_Worker(LPVOID pVoid)
{
	PARALLEL_PROCESSING_THREAD *pThread = (PARALLEL_PROCESSING_THREAD *)pVoid;
	CParallelProcessing *pClass = (CParallelProcessing *) pThread->Class;

	PARALLEL_PROCESSING_WORKLOAD WL;
	memset(&WL, 0, sizeof(PARALLEL_PROCESSING_WORKLOAD));
	unsigned short iWorkloadStatus = WORKLOAD_STATUS_EMPTY;

	WL.SourceAlloc = WORKLOAD_BUFFER_SIZE;
	if(!(WL.SourceBuf = (char *) calloc(WL.SourceAlloc, sizeof(char))))
	{
		return 0;
	}

	WL.TargetAlloc = WORKLOAD_BUFFER_SIZE * 2;
	if(!(WL.TargetBuf = (char *) calloc(WL.TargetAlloc, sizeof(char))))
	{
		free(WL.SourceBuf);
		return 0;
	}

	pThread->Status = THREAD_STATUS_SLEEPING;
	SetEvent(pThread->SyncHandle);

	while((iWorkloadStatus = pClass->ConsumeWorkload(&WL)) != WORKLOAD_STATUS_COMPLETE)
	{
		if(iWorkloadStatus == WORKLOAD_STATUS_OK)
		{
			{ pClass->Lock();
				if(pThread->Status != THREAD_STATUS_ACTIVE)
				{
					pThread->Status = THREAD_STATUS_ACTIVE;
					pClass->PPI.Threads.ActiveCount++;
				}
			} pClass->UnLock();

			//---------------------------------------------------------------------------------------
			//---( Begin Process workload ) ---------------------------------------------------------
			//---------------------------------------------------------------------------------------
			//...
			//---------------------------------------------------------------------------------------
			//---(  End Process workload  ) ---------------------------------------------------------
			//---------------------------------------------------------------------------------------
		}
		else if(iWorkloadStatus == WORKLOAD_STATUS_EMPTY)
		{
			{ pClass->Lock();
				if(pThread->Status != THREAD_STATUS_SLEEPING)
				{
					pThread->Status = THREAD_STATUS_SLEEPING;
					pClass->PPI.Threads.ActiveCount--;

					//If we decrmented the thread count to zero, all the treads must be done.
					if(pClass->PPI.Threads.ActiveCount == 0)
					{
						SetEvent(pClass->PPI.Threads.WorkloadCompleteEvent);
					}
				}
			} pClass->UnLock();

			WaitForSingleObject(pClass->PPI.Threads.DataReadyHintEvent, 1000);
		}
		else {
			//An unknow error has occured!
			break;
		}
	}

	{ pClass->Lock();
		//If the thread was not sleeping, then deincrement the active thread count.
		if(pThread->Status != THREAD_STATUS_SLEEPING)
		{
			pClass->PPI.Threads.ActiveCount--;
		}
				
		//Set the thread to inactive, and deincrement the total available thread count.
		pThread->Status = THREAD_STATUS_DEACTIVATED;
		pClass->PPI.Threads.Count--;

		if(pClass->PPI.Threads.Count == 0)
		{
			SetEvent(pClass->PPI.Threads.AllThreadsInactive);
		}
	} pClass->UnLock();

	free(WL.SourceBuf);
	free(WL.TargetBuf);

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned short CParallelProcessing::ConsumeWorkload(PARALLEL_PROCESSING_WORKLOAD *pWL)
{
	unsigned short iStatus = WORKLOAD_STATUS_EMPTY;

	{ this->Lock();
		if((iStatus = this->WLI.Status) == WORKLOAD_STATUS_OK)
		{
			pWL->Result = ReadFile(this->WLI.SourceHandle,
				pWL->SourceBuf, pWL->SourceAlloc, &pWL->BytesRead, NULL);

			if(pWL->Result && pWL->BytesRead > 0)
			{
				iStatus = WORKLOAD_STATUS_OK;
			}
			else if(pWL->BytesRead == 0 && pWL->Result)
			{
				iStatus = WORKLOAD_STATUS_EMPTY;
			}
			else{
				iStatus = WORKLOAD_STATUS_ERROR;
			}
		}
	} this->UnLock();

	return iStatus;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CParallelProcessing::Process(HANDLE hSource, HANDLE hTarget)
{
	this->Lock();

	if(this->WLI.Status != WORKLOAD_STATUS_EMPTY)
	{
		this->UnLock();
		return false;
	}

	this->WLI.SourceHandle = hSource;
	this->WLI.TargetHandle = hTarget;
	this->WLI.Status = WORKLOAD_STATUS_OK;

	this->UnLock();

	SetEvent(this->PPI.Threads.DataReadyHintEvent);
	WaitForSingleObject(PPI.Threads.WorkloadCompleteEvent, INFINITE);

	this->WLI.Status = WORKLOAD_STATUS_EMPTY;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CParallelProcessing::Initialize(void)
{
	char sEvent[512];

	SYSTEM_INFO SI;
	memset(&SI, 0, sizeof(SI));
	GetSystemInfo(&SI);

	memset(&this->WLI, 0, sizeof(PARALLEL_PROCESSING_WORKLOAD_INFO));
	this->WLI.Status = WORKLOAD_STATUS_EMPTY;

	memset(&this->PPI, 0, sizeof(PARALLEL_PROCESSING_INFO));
	memset(&this->PPI.Threads, 0, sizeof(PARALLEL_PROCESSING_THREADS));

	sprintf_s(sEvent, sizeof(sEvent), "PP_DRH%d", GetTickCount());
	PPI.Threads.DataReadyHintEvent = CreateEvent(NULL, false, false, sEvent);

	sprintf_s(sEvent, sizeof(sEvent), "PP_ATI%d", GetTickCount());
	PPI.Threads.AllThreadsInactive = CreateEvent(NULL, false, false, sEvent);

	sprintf_s(sEvent, sizeof(sEvent), "PP_WLC%d", GetTickCount());
	PPI.Threads.WorkloadCompleteEvent = CreateEvent(NULL, false, false, sEvent);

	this->PPI.Processors = SI.dwNumberOfProcessors;
	this->PPI.ActiveProcessorMask = SI.dwActiveProcessorMask;

	if(!GetProcessAffinityMask(GetCurrentProcess(),
		&this->PPI.ProcessAffinityMask,
		&this->PPI.SystemAffinityMask))
	{
		return false;
	}

	InitializeCriticalSection(&this->CS);

	for(unsigned short iCPU = 0; iCPU < this->PPI.Processors; iCPU++)
	{
		if(IsBitSet(this->PPI.ActiveProcessorMask, iCPU))
		{
			//printf("CPU %d is Active\n", iCPU);
			if(IsBitSet(this->PPI.SystemAffinityMask, iCPU))
			{
				//printf("System is allowed to run on CPU %d\n", iCPU);
				if(IsBitSet(this->PPI.ProcessAffinityMask, iCPU))
				{
					//printf("Process is allowed to run on CPU %d\n", iCPU);

					int iThread = PPI.Threads.Count++;

					sprintf_s(sEvent, sizeof(sEvent), "ParProc%d", GetTickCount());

					PPI.Threads.Thread = (PARALLEL_PROCESSING_THREAD *)
						realloc(PPI.Threads.Thread,
						sizeof(PARALLEL_PROCESSING_THREAD) * (iThread + 1));

					if(!PPI.Threads.Thread)
					{
						printf("Failed to allocate thread info space.\n");
						return false;
					}

					PPI.Threads.Thread[iThread].Status = THREAD_STATUS_INITIALIZING;
					PPI.Threads.Thread[iThread].Slot = iThread;
					PPI.Threads.Thread[iThread].Class = this;
					PPI.Threads.Thread[iThread].IdealProcessor = iCPU;

					PPI.Threads.Thread[iThread].SyncHandle =
						CreateEvent(NULL, false, false, sEvent);

					PPI.Threads.Thread[iThread].Handle =
						CreateThread(NULL, NULL, CParallelProcessing_Worker,
						&PPI.Threads.Thread[iThread],
						CREATE_SUSPENDED, &PPI.Threads.Thread[iThread].ID);

					SetThreadIdealProcessor(PPI.Threads.Thread[iThread].Handle, iCPU);

					ResumeThread(PPI.Threads.Thread[iThread].Handle);

					if(WaitForSingleObject(PPI.Threads.Thread[iThread].SyncHandle, 10000) != WAIT_OBJECT_0)
					{
						printf("Thread initialization timeout expired for CPU %d\n", iCPU);
					}

				}else{
					printf("Process is NOT allowed to run on CPU %d\n", iCPU);
				}
			}else{
				printf("System is NOT allowed to run on CPU %d\n", iCPU);
			}
		}
		else{
			printf("CPU %d is NOT Active\n", iCPU);
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CParallelProcessing::Destroy(void)
{
	this->Lock();
	this->WLI.Status = WORKLOAD_STATUS_COMPLETE;
	this->UnLock();

	WaitForSingleObject(PPI.Threads.AllThreadsInactive, INFINITE);

	DeleteCriticalSection(&this->CS);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CParallelProcessing::Lock(void)
{
	EnterCriticalSection(&this->CS);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CParallelProcessing::UnLock(void)
{
	LeaveCriticalSection(&this->CS);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
