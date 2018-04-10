#ifndef _DRAGDROP_CPP
#define _DRAGDROP_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <CommCtrl.H>
#include <ShlOBJ.H>
#include <Stdlib.H>
#include <Mmsystem.h>
#include <TChar.h>

#include "../Source/Package.H"
#include "../Source/Entry.H"
#include "../Source/Routines.H"
#include "../Source/DragDrop.H"
#include "../Source/ListView.H"

#include "../Dialogs/MainDlg.H"
#include "../Dialogs/ReadingDlg.H"
#include "../Dialogs/ProgressDialog.H"
#include "../Dialogs/ExtractDlg.H"
#include "../Dialogs/OpenPassDlg.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 
using namespace NSWFL::File; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

char sTempDropPath[MAX_PATH];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SetNormalDropAttributes(void)
{
	char sFileName[MAX_PATH];
	char sTempFolder[MAX_PATH];
	char sFullPath[MAX_PATH];

	strcpy_s(sTempFolder, sizeof(sTempFolder), sTempDropPath);

	//Calculate the length of the data.
	for(int iIndex = -1; (iIndex = ListView_GetNextItem(ghFileList, iIndex, LVNI_SELECTED)) != -1; )
    {
        ListView_GetItemText(ghFileList, iIndex, LIST_POS_NAME, sFileName, sizeof(sFileName));
		sprintf_s(sFullPath, sizeof(sFullPath), "%s\\%s", sTempFolder, sFileName);
		SetFileAttributes(sFullPath, FILE_ATTRIBUTE_NORMAL);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DeleteSelectedTempDrops(void)
{
	char sFileName[MAX_PATH];
	char sTempFolder[MAX_PATH];
	char sFullPath[MAX_PATH];

	strcpy_s(sTempFolder, sizeof(sTempFolder), sTempDropPath);

	//Calculate the length of the data.
	for(int iIndex = -1; (iIndex = ListView_GetNextItem(ghFileList, iIndex, LVNI_SELECTED)) != -1; )
    {
        ListView_GetItemText(ghFileList, iIndex, LIST_POS_NAME, sFileName, sizeof(sFileName));
		sprintf_s(sFullPath, sizeof(sFullPath), "%s\\%s", sTempFolder, sFileName);
		SetFileAttributes(sFullPath, FILE_ATTRIBUTE_NORMAL);
		SDeleteFile(sFullPath);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DragDropProc(HWND hOwner, HWND hList)
{
	IDataObject *pDataObject;
	IDropSource *pDropSource;
	DWORD dwEffect = 0;
	DWORD dwResult = 0;

	STGMEDIUM stgmed;
	memset(&stgmed, 0, sizeof(stgmed));
	stgmed.tymed = TYMED_FILE;
	stgmed.pUnkForRelease = 0;

	FORMATETC fmtetc;
	memset(&fmtetc, 0, sizeof(fmtetc));
	fmtetc.cfFormat = CF_HDROP;
	fmtetc.ptd = NULL;
	fmtetc.dwAspect = DVASPECT_CONTENT;
	fmtetc.lindex = -1;
	fmtetc.tymed = TYMED_HGLOBAL;

	// Copy the temp file locations into the IDataObject.
	stgmed.hGlobal = GetDropFileNames();

	// Create IDataObject and IDropSource COM objects
	CreateDropSource(&pDropSource);
	CreateDataObject(&fmtetc, &stgmed, 1, &pDataObject);

	DWORD dwINEffect = 0;

	if(gbDragDropCopyInsteadOfMove)
	{
		dwINEffect = DROPEFFECT_COPY;
	}
	else{
		dwINEffect = DROPEFFECT_MOVE;
	}

	//The drag-drop operation starts here!
	if((dwResult = DoDragDrop(pDataObject, pDropSource, dwINEffect, &dwEffect)) == DRAGDROP_S_DROP)
	{
		//Success!
		if(dwINEffect &dwEffect)
		{
			//Delete temp files!!!
		}
	}
	else if(dwResult == DRAGDROP_S_CANCEL)
	{
		//Cancelled!
	}

	DeleteSelectedTempDrops();
	SDeleteFile(sTempDropPath);

	pDataObject->Release();
	pDropSource->Release();

	ReleaseCapture();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HGLOBAL GetDropFileNames()
{
	char sFileName[MAX_PATH];
	char sTemp[16];

	for(int iTry = 0; iTry < 10; iTry++)
	{
		sprintf_s(sTemp, sizeof(sTemp), "%d", GetTickCount());
		if(strlen(gsTempFilePath) == 0)
		{
			GetTempPath(sizeof(sTempDropPath), sTempDropPath);
		}
		else{
			strcpy_s(sTempDropPath, sizeof(sTempDropPath), gsTempFilePath);
			CorrectReversePath(sTempDropPath, sizeof(sTempDropPath));
			CreateFolderStructure(sTempDropPath);

			if(!FileAccess(sTempDropPath, FExist))
			{
				GetTempPath(sizeof(sTempDropPath), sTempDropPath);
			}
		}

		strcat_s(sTempDropPath, sizeof(sTempDropPath), "\\");
		strcat_s(sTempDropPath, sizeof(sTempDropPath), sTemp);
		strcat_s(sTempDropPath, sizeof(sTempDropPath), "\\");

		if(!FileAccess(sTempDropPath, FExist))
		{
			CorrectReversePath(sTempDropPath, sizeof(sTempDropPath));
			CreateFolderStructure(sTempDropPath);
			break;
		}
		else{
			//Already existed, try again.
		}
	}

	int iFileCount = ListView_GetSelectedCount(ghFileList);
	int iFileNameLen = 0;
	int iTempFolderLen = (int)strlen(sTempDropPath);
	DWORD dwDataLength = sizeof(DROPFILES) + 2;
	
	//Calculate the length of the data.
	for(int iIndex = -1; (iIndex = ListView_GetNextItem(ghFileList, iIndex, LVNI_SELECTED)) != -1; )
    {
        ListView_GetItemText(ghFileList, iIndex, LIST_POS_NAME, sFileName, sizeof(sFileName));
		dwDataLength += (DWORD)strlen(sFileName) + iTempFolderLen + 1;
    }

	HGLOBAL hGlobal = GlobalAlloc(GHND|GMEM_SHARE, dwDataLength);
	BYTE* pLock = (BYTE*)GlobalLock(hGlobal);
	if(pLock==NULL)
	{
		return NULL;
	}

	((DROPFILES*)pLock)->fNC=TRUE;
	GetCursorPos(&((DROPFILES*)pLock)->pt);
	((DROPFILES*)pLock)->fWide=FALSE;
	((DROPFILES*)pLock)->pFiles=sizeof(DROPFILES);
	LPSTR pDst=(LPSTR)pLock+sizeof(DROPFILES);

	for(int iIndex = -1; (iIndex = ListView_GetNextItem(ghFileList, iIndex, LVNI_SELECTED)) != -1; )
    {
        ListView_GetItemText(ghFileList, iIndex, LIST_POS_NAME, sFileName, sizeof(sFileName));

		iFileNameLen = (int)strlen(sFileName);

		memcpy(pDst, sTempDropPath, iTempFolderLen);
		pDst += iTempFolderLen;

		memcpy(pDst, sFileName, iFileNameLen+1);
		pDst += iFileNameLen+1;
	}
	*pDst='\0';

	GlobalUnlock(hGlobal);

	return hGlobal;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif