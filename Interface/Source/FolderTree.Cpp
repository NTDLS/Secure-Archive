#ifndef _FolderTree_CPP
#define _FolderTree_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <ShlObj.H>
#include <Stdio.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "Package.H"
#include "Entry.H"
#include "Routines.H"
#include "FolderTree.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::File;
using namespace NSWFL::Registry; 
using namespace NSWFL::Windows; 
using namespace NSWFL::String; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL FolderTree::DeleteAllItems(void)
{
	return TreeView_DeleteAllItems(lhTreeHandle);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int FolderTree::GetTreeItemImage(HTREEITEM htItem)
{
    TVITEMEX tvItemEx;
	memset(&tvItemEx, 0, sizeof(TVITEMEX));
	tvItemEx.hwnd	= this->lhTreeHandle;
    tvItemEx.hItem	= htItem;
    tvItemEx.mask	= TVIF_IMAGE|TVIF_SELECTEDIMAGE;

    if(TreeView_GetItem(lhTreeHandle, &tvItemEx))
	{
		return tvItemEx.iImage;
	}
	else{
		return -1;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL FolderTree::GetTreeItemText(HTREEITEM htItem, char *sOUTText)
{
    TVITEMEX tvItemEx;
	memset(&tvItemEx, 0, sizeof(TVITEMEX));

	tvItemEx.hwnd		= this->lhTreeHandle;
	tvItemEx.hItem		= htItem;
    tvItemEx.mask		= TVIF_TEXT;
    tvItemEx.pszText	= sOUTText;
    tvItemEx.cchTextMax	= MAX_PATH;

    if(TreeView_GetItem(lhTreeHandle, &tvItemEx))
	{
		if(tvItemEx.pszText && strlen(tvItemEx.pszText) > 2)
		{
			if(tvItemEx.pszText[1] == ':')
			{
				tvItemEx.pszText[2] = '\0';
			}
		}
		return TRUE;
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int FolderTree::ChildPathMatchLength(const char *sInParent, const char *sInChild)
{
	char sParent[MAX_PATH];
	strcpy_s(sParent, sizeof(sParent), sInParent);
	int sParentLen = (int)strlen(sParent);
	if(sParent[sParentLen - 1] == '\\')
	{
		sParent[sParentLen - 1] = '\0';
		sParentLen--;
	}

	char sChild[MAX_PATH];
	strcpy_s(sChild, sizeof(sChild), sInChild);
	int iChildLen = (int)strlen(sChild);
	if(sChild[iChildLen - 1] == '\\')
	{
		sChild[iChildLen - 1] = '\0';
		iChildLen--;
	}

	if(iChildLen < sParentLen)
	{
		return 0;
	}

	int iRPos = 0;

	for(iRPos = 0; iRPos < sParentLen; iRPos++)
	{
		if(sInChild[iRPos] != sInParent[iRPos])
		{
			return -iRPos;
		}
	}

	return iRPos;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int FolderTree::GetPathTok(const char *sFullPath, char *sOutTok, int *iInOutRPos)
{
	int iLength = (int)strlen(sFullPath);
	int iWPos = 0;
	int iRPos = (int)*iInOutRPos;


	while(iRPos < iLength && (sFullPath[iRPos] == '\\' || sFullPath[iRPos] == '/'))
	{
		iRPos++;
	}

	while(iRPos < iLength)
	{
		if(sFullPath[iRPos] == '\\' || sFullPath[iRPos] == '/')
		{
			break;
		}
		sOutTok[iWPos++] = sFullPath[iRPos++];
	}

	sOutTok[iWPos] = '\0';
	*iInOutRPos = iRPos;

	return iWPos;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HTREEITEM FolderTree::FindImmediateChild(HTREEITEM hParent, const char *sText)
{
	HTREEITEM hItem = TreeView_GetNextItem(this->lhTreeHandle, hParent, TVGN_CHILD);
	if(hItem)
	{
		char sItemText[MAX_PATH];
		do
		{
			GetTreeItemText(hItem, sItemText);

			if(_strcmpi(sItemText, sText) == 0)
			{
				return hItem;
			}
		}
		while((hItem = TreeView_GetNextItem(this->lhTreeHandle, hItem, TVGN_NEXT)));
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderTree::SelectPath(const char *sINPath)
{
	if(DoesPathContainUnicode(sINPath))
	{
		return false;
	}

	char sPath[MAX_PATH];
	char sTok[MAX_PATH];
	int iRPos = 0;
	int iTokLen = 0;

	strcpy_s(sPath, sizeof(sPath), sINPath);
	CorrectReversePath(sPath, sizeof(sPath));

	HTREEITEM hItem = NULL;

	if((iRPos = ChildPathMatchLength(this->sDesktopPath, sINPath)) > 0)
	{
		hItem = this->hDesktopLeaf;
	}
	else if((iRPos = ChildPathMatchLength(this->sMyDocuments, sINPath)) > 0)
	{
		hItem = this->hMyDocumentsLeaf;
	}
	else {
		iRPos = 0;
		hItem = this->hMyComputerLeaf;
	}

	TreeView_Expand(this->lhTreeHandle, hItem, TVE_EXPAND);
	TreeView_SelectItem(this->lhTreeHandle, hItem);

	while((iTokLen = GetPathTok(sPath, sTok, &iRPos)))
	{
		if((hItem = FindImmediateChild(hItem, sTok)))
		{
			TreeView_Expand(this->lhTreeHandle, hItem, TVE_EXPAND);
			TreeView_SelectItem(this->lhTreeHandle, hItem);
		}
		else{
			return false;
		}
	}

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HTREEITEM FolderTree::AddTreeViewItem(HTREEITEM hParent, char *sINText, int iImage, int iSelImage)
{
    TV_INSERTSTRUCT tvIS;
    tvIS.item.hItem          = hParent;
    tvIS.item.pszText        = sINText;
    tvIS.item.iImage         = iImage;
    tvIS.item.iSelectedImage = iSelImage;
    tvIS.item.mask           = TVIF_TEXT|TVIF_SELECTEDIMAGE|TVIF_IMAGE;
    tvIS.hParent             = hParent;
    tvIS.item.cchTextMax     = (int)strlen(sINText);

    return TreeView_InsertItem(lhTreeHandle, &tvIS);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderTree::IsAlreadyLoaded(HTREEITEM hSubLeaf)
{
    int iPos = 0;

    while(iPos < iLoadedItems)
    {
        if(hSubLeaf == LoadedList[iPos])
        {
            return true;
        }
        iPos++;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderTree::InitFolderTree(HWND hTreeHandle, HWND hButtonHandle, HWND hSaveToLocation, HWND hSaveToFile)
{
	bool bResult = true;

	DWORD dwValue = 0;
	
	if(Get_DWORDRegistryValue(HKEY_CURRENT_USER,
		"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "Hidden", dwValue))
	{
		this->bShowHiddenFiles = (dwValue <= 1);
		this->bShowSuperHiddenFiles = Get_BOOLRegistryValue(HKEY_CURRENT_USER,
			"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", "ShowSuperHidden");
	}
	else {
		this->bShowHiddenFiles = false;
		this->bShowSuperHiddenFiles = false;
	}

	this->lhTreeHandle = hTreeHandle;
    this->lhButtonHandle = hButtonHandle;
    this->lhSaveToLocation = hSaveToLocation;
	this->lhSaveToFile = hSaveToFile;

	this->DeleteAllItems();

	Get_DesktopDirectory(this->sDesktopPath, sizeof(this->sDesktopPath));
	CorrectReversePath(this->sDesktopPath, sizeof(this->sDesktopPath));

	Get_DocumentsDirectory(this->sMyDocuments, sizeof(this->sMyDocuments));
	CorrectReversePath(this->sMyDocuments, sizeof(this->sMyDocuments));

	if(!this->CreateTreeImageList())
    {
        bResult = false;
    }

    if(!this->InitAlreadyLoaded())
    {
        bResult = false;
    }

    if(!this->PopBrowseTree())
    {
        bResult = false;
    }

    return bResult;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderTree::UnInitFolderTree(void)
{
	this->DeleteAllItems();

	if(!this->FreeAlreadyLoaded())
    {
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderTree::InitAlreadyLoaded(void)
{
    if((this->LoadedList = (HTREEITEM *) calloc(sizeof(HTREEITEM), LOAD_TREE_ITEMS_INCREMENT)) == NULL)
    {
        return false;
    }

    this->iMaxLoadedItems = LOAD_TREE_ITEMS_INCREMENT;
    this->iLoadedItems = 0;

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderTree::FreeAlreadyLoaded(void)
{
    this->iLoadedItems = 0;
    this->iMaxLoadedItems = 0;

    free(this->LoadedList);

    this->LoadedList = NULL;

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderTree::IndexDirectoryForSubItem(HTREEITEM hSubLeaf, const char *sINFolder,
										  bool bIndexTwoDeep, bool bIgnoreAlreadyLoaded)
{
    if(!bIgnoreAlreadyLoaded)
	{
		if(this->IsAlreadyLoaded(hSubLeaf))
		{
			return true;
		}
	}

	bool bLoadDocumentsIcons = false;
	char sMyPictures[MAX_PATH];
	char sMyVideos[MAX_PATH];
	char sMyMusic[MAX_PATH];

	if(this->GetTreeItemImage(hSubLeaf) == TREE_IMAGE_MYDOCUMENTS)
	{
		char sSpecialFolder[MAX_PATH];

		bLoadDocumentsIcons = true;

		if(!Get_PicturesDirectory(sSpecialFolder, sizeof(sSpecialFolder)))
		{
			strcpy_s(sSpecialFolder, sizeof(sSpecialFolder), "My Pictures");
		}
		GetFileName(sSpecialFolder, sMyPictures, sizeof(sMyPictures));

		if(!Get_VideosDirectory(sSpecialFolder, sizeof(sSpecialFolder)))
		{
			strcpy_s(sSpecialFolder, sizeof(sSpecialFolder), "My Videos");
		}
		GetFileName(sSpecialFolder, sMyVideos, sizeof(sMyVideos));

		if(!Get_MusicDirectory(sSpecialFolder, sizeof(sSpecialFolder)))
		{
			strcpy_s(sSpecialFolder, sizeof(sSpecialFolder), "My Music");
		}
		GetFileName(sSpecialFolder, sMyMusic, sizeof(sMyMusic));
	}
	
	WIN32_FIND_DATA FD;
	memset(&FD, 0, sizeof(WIN32_FIND_DATA));

	HANDLE fHandle = NULL;
    char sDirTemp[MAX_PATH + 1];
	char sExt[MAX_PATH];

    if(sINFolder[strlen(sINFolder) - 1] != '\\')
	{
        sprintf_s(sDirTemp, sizeof(sDirTemp), "%s\\*.*", sINFolder);
	}
    else sprintf_s(sDirTemp, sizeof(sDirTemp), "%s*.*", sINFolder);

    if((fHandle = FindFirstFile(sDirTemp, &FD)) == INVALID_HANDLE_VALUE)
    {
        return false;
    }

	if(!bIgnoreAlreadyLoaded)
	{
		if(iLoadedItems == iMaxLoadedItems)
		{
			this->iMaxLoadedItems += LOAD_TREE_ITEMS_INCREMENT;
			this->LoadedList = (HTREEITEM *) realloc(this->LoadedList, iMaxLoadedItems * sizeof(HTREEITEM));
		}

		this->LoadedList[this->iLoadedItems++] = hSubLeaf;
	}

    do{
        if( strcmp(FD.cFileName, ".") && strcmp(FD.cFileName, "..") )
        {
			if((FD.dwFileAttributes &FILE_ATTRIBUTE_SYSTEM))
			{
				if(!this->bShowSuperHiddenFiles || !this->bShowHiddenFiles)
				{
					continue; //Skip this directory.
				}
			}
			if((FD.dwFileAttributes &FILE_ATTRIBUTE_HIDDEN))
			{
				if(!this->bShowHiddenFiles)
				{
					continue; //Skip this directory.
				}
			}

			if(!DoesPathContainUnicode(FD.cFileName))
			{
				if(FD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					int iCloseImage = TREE_IMAGE_CLOSEDFOLDER;
					int iOpenImage = TREE_IMAGE_OPENFOLDER;

					if(bLoadDocumentsIcons)
					{
						if(_strcmpi(FD.cFileName, sMyPictures) == 0)
						{
							iCloseImage = TREE_IMAGE_MYPICTURES; 
							iOpenImage = TREE_IMAGE_MYPICTURES;
						}
						else if(_strcmpi(FD.cFileName, sMyVideos) == 0)
						{
							iCloseImage = TREE_IMAGE_MYMOVIES; 
							iOpenImage = TREE_IMAGE_MYMOVIES;
						}
						else if(_strcmpi(FD.cFileName, sMyMusic) == 0)
						{
							iCloseImage = TREE_IMAGE_MYMUSIC; 
							iOpenImage = TREE_IMAGE_MYMUSIC;
						}
					}

					HTREEITEM hRootLeaf = AddTreeViewItem(hSubLeaf, FD.cFileName, iCloseImage, iOpenImage);

					if(bIndexTwoDeep)
					{
						if(sINFolder[strlen(sINFolder) - 1] != '\\')
						{
							sprintf_s(sDirTemp, sizeof(sDirTemp), "%s\\%s", sINFolder, FD.cFileName);
						}
						else sprintf_s(sDirTemp, sizeof(sDirTemp), "%s%s", sINFolder, FD.cFileName);

						IndexDirectoryForSubItem(hRootLeaf, sDirTemp, false, false);
					}
				}
				else{
					if(this->lhSaveToFile)
					{
						if(GetFileExtension(FD.cFileName, sExt, sizeof(sExt)))
						{
							if(_strcmpi(sExt, DEFAULTEXT) == 0)
							{
								AddTreeViewItem(hSubLeaf, FD.cFileName, TREE_IMAGE_SECUREARCHIVE, TREE_IMAGE_SECUREARCHIVE);
							}
						}
					}
				}
			}
        }
    } while(FindNextFile(fHandle, &FD));

    FindClose(fHandle);

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

STORAGE_BUS_TYPE FolderTree::GetBusType(const char *sPhysicalPath)
{
	HANDLE hDevice = CreateFile(sPhysicalPath, 0,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if(hDevice != INVALID_HANDLE_VALUE)
	{
		DWORD dwOutBytes = 0;			// IOCTL output length.
		STORAGE_PROPERTY_QUERY Query;	// Input param for query.

		// Specify the query type.
		Query.PropertyId = StorageDeviceProperty;
		Query.QueryType = PropertyStandardQuery;

		char OutBuf[1024] = {0}; // Good enough, usually about 100 bytes.
		PSTORAGE_DEVICE_DESCRIPTOR pDevDesc = (PSTORAGE_DEVICE_DESCRIPTOR)OutBuf;
		pDevDesc->Size = sizeof(OutBuf);

		// Query using IOCTL_STORAGE_QUERY_PROPERTY
		BOOL bResult = DeviceIoControl(hDevice,			// Device handle.
			IOCTL_STORAGE_QUERY_PROPERTY,				// Info of device property.
			&Query, sizeof(STORAGE_PROPERTY_QUERY),		// Input data buffer.
			pDevDesc, pDevDesc->Size,					// Output data buffer.
			&dwOutBytes,								// Out's length.
			(LPOVERLAPPED)NULL);

		CloseHandle(hDevice);

		if(bResult)
		{
			return pDevDesc->BusType;
		}
	}

	return BusTypeUnknown;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderTree::PopBrowseTree(void)
{
	HTREEITEM hSubLeaf = NULL;

    char sDriveText[MAX_PATH];
    char sDisplayText[MAX_PATH * 2];
    char sPathTemp[MAX_PATH + 1];
	char sPhysicalPath[MAX_PATH];
	char sRootPath[MAX_PATH];

    int iDrive = 65;  //Drive A
    int iDriveType = 0;
    int iImage = 0;

    this->hDesktopLeaf = AddTreeViewItem(TVI_ROOT, "Desktop", TREE_IMAGE_DESKTOP, TREE_IMAGE_DESKTOP);
    this->hMyDocumentsLeaf = AddTreeViewItem(hDesktopLeaf, "My Documents", TREE_IMAGE_MYDOCUMENTS, TREE_IMAGE_MYDOCUMENTS);
    this->hMyComputerLeaf = AddTreeViewItem(hDesktopLeaf, "My Computer", TREE_IMAGE_MYCOMPUTER, TREE_IMAGE_MYCOMPUTER);

    while(iDrive < 90) //Drive Z
    {
        sprintf_s(sDriveText, sizeof(sDriveText), "%c:", iDrive);
        sprintf_s(sDisplayText, sizeof(sDisplayText), "%c:", iDrive);
		sprintf_s(sPhysicalPath, sizeof(sPhysicalPath), "\\\\.\\%s", sDriveText);
		sprintf_s(sRootPath, sizeof(sRootPath), "%c:\\", iDrive);

        if((iDriveType = GetDriveType(sDriveText)) != 1)
        {
            if(iDriveType == DRIVE_REMOVABLE)		iImage = TREE_IMAGE_REMOVABLE;	//Removable Disk
            else if(iDriveType == DRIVE_FIXED)		iImage = TREE_IMAGE_FIXED;		//Fixed Disk
            else if(iDriveType == DRIVE_REMOTE)		iImage = TREE_IMAGE_NETWORK;	//Remote Network Drive
            else if(iDriveType == DRIVE_CDROM)		iImage = TREE_IMAGE_CDROM;		//Cd-Rom Drive
            else if(iDriveType == DRIVE_RAMDISK)	iImage = TREE_IMAGE_RAMDRIVE;	//High Speed RAM Drive
			else									iImage = TREE_IMAGE_UNKNOWN;	//Unknown Type
 
			if(iDriveType != DRIVE_CDROM)
			{
				if(iDriveType == DRIVE_FIXED || iDriveType == DRIVE_REMOVABLE)
				{
					STORAGE_BUS_TYPE BusType = BusTypeUnknown;
					
					BusType = this->GetBusType(sPhysicalPath);

					if(iDriveType == DRIVE_FIXED || BusType == BusTypeUsb)
					{
						if(BusType == BusTypeUsb)
						{
							iImage = TREE_IMAGE_USB;
						}

						char sVolumeNameBuffer[MAX_PATH + 1];
						char sFileSystemNameBuffer[MAX_PATH + 1];
						DWORD dwVolumeSerialNumber = 0;
						DWORD dwMaximumComponentLength = 0;
						DWORD dwFileSystemFlags = 0;

						BOOL bVolInfoResult = GetVolumeInformation(
								sRootPath,
								sVolumeNameBuffer,
								sizeof(sVolumeNameBuffer),
								&dwVolumeSerialNumber,
								&dwMaximumComponentLength,
								&dwFileSystemFlags,
								sFileSystemNameBuffer,
								sizeof(sFileSystemNameBuffer)
							);

						if(bVolInfoResult)
						{
							if(Trim(sVolumeNameBuffer) > 0)
							{
								strcat_s(sDisplayText, sizeof(sDisplayText), " (");
								strcat_s(sDisplayText, sizeof(sDisplayText), sVolumeNameBuffer);
								strcat_s(sDisplayText, sizeof(sDisplayText), ")");
							}
						}
					}
				}

	            hSubLeaf = AddTreeViewItem(hMyComputerLeaf, sDisplayText, iImage, iImage);

				if(iDriveType == DRIVE_FIXED)
				{
	                IndexDirectoryForSubItem(hSubLeaf, sDriveText, false, false);
				}
			}
        }

        iDrive++;
    }

    if(Get_DesktopDirectory(sPathTemp, sizeof(sPathTemp)))
    {
        IndexDirectoryForSubItem(this->hDesktopLeaf, sPathTemp, true, true);
    }

    if(Get_DocumentsDirectory(sPathTemp, sizeof(sPathTemp)))
    {
        IndexDirectoryForSubItem(this->hMyDocumentsLeaf, sPathTemp, true, true);
    }

    TreeView_Expand(lhTreeHandle, hDesktopLeaf, TVE_EXPAND);
    TreeView_Expand(lhTreeHandle, hMyComputerLeaf, TVE_EXPAND);

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderTree::CreateTreeImageList(void)
{
    hImageList = ImageList_Create(16, 16, ILC_COLOR24, 10, 1);

    HBITMAP hBitMap = NULL;

    hBitMap = LoadBitmap(ghAppInstance, MAKEINTRESOURCE(IDB_TREE_UNKNOWN));
    ImageList_Add(hImageList, hBitMap, hBitMap);
    DeleteObject(hBitMap);

    hBitMap = LoadBitmap(ghAppInstance, MAKEINTRESOURCE(IDB_TREE_REMOVABLE));
    ImageList_Add(hImageList, hBitMap, hBitMap);
    DeleteObject(hBitMap);

    hBitMap = LoadBitmap(ghAppInstance, MAKEINTRESOURCE(IDB_TREE_FIXED));
    ImageList_Add(hImageList, hBitMap, hBitMap);
    DeleteObject(hBitMap);

    hBitMap = LoadBitmap(ghAppInstance, MAKEINTRESOURCE(IDB_TREE_NETWORK));
    ImageList_Add(hImageList, hBitMap, hBitMap);
    DeleteObject(hBitMap);

    hBitMap = LoadBitmap(ghAppInstance, MAKEINTRESOURCE(IDB_TREE_CDROM));
    ImageList_Add(hImageList, hBitMap, hBitMap);
    DeleteObject(hBitMap);

    hBitMap = LoadBitmap(ghAppInstance, MAKEINTRESOURCE(IDB_TREE_RAMDRIVE));
    ImageList_Add(hImageList, hBitMap, hBitMap);
    DeleteObject(hBitMap);

	hBitMap = LoadBitmap(ghAppInstance, MAKEINTRESOURCE(IDB_TREE_USBDRIVE));
    ImageList_Add(hImageList, hBitMap, hBitMap);
    DeleteObject(hBitMap);

    hBitMap = LoadBitmap(ghAppInstance, MAKEINTRESOURCE(IDB_TREE_DESKTOP));
    ImageList_Add(hImageList, hBitMap, hBitMap);
    DeleteObject(hBitMap);

    hBitMap = LoadBitmap(ghAppInstance, MAKEINTRESOURCE(IDB_TREE_MYDOCS));
    ImageList_Add(hImageList, hBitMap, hBitMap);
    DeleteObject(hBitMap);

    hBitMap = LoadBitmap(ghAppInstance, MAKEINTRESOURCE(IDB_TREE_CLOSEDFOLDER));
    ImageList_Add(hImageList, hBitMap, hBitMap);
    DeleteObject(hBitMap);

    hBitMap = LoadBitmap(ghAppInstance, MAKEINTRESOURCE(IDB_TREE_OPENFOLDER));
    ImageList_Add(hImageList, hBitMap, hBitMap);
    DeleteObject(hBitMap);

    hBitMap = LoadBitmap(ghAppInstance, MAKEINTRESOURCE(IDB_TREE_MYCOMPUTER));
    ImageList_Add(hImageList, hBitMap, hBitMap);
    DeleteObject(hBitMap);

    hBitMap = LoadBitmap(ghAppInstance, MAKEINTRESOURCE(IDB_TREE_MYDOCUMENTS));
    ImageList_Add(hImageList, hBitMap, hBitMap);
    DeleteObject(hBitMap);

    hBitMap = LoadBitmap(ghAppInstance, MAKEINTRESOURCE(IDB_TREE_MYPICTURES));
    ImageList_Add(hImageList, hBitMap, hBitMap);
    DeleteObject(hBitMap);

    hBitMap = LoadBitmap(ghAppInstance, MAKEINTRESOURCE(IDB_TREE_MYMOVIES));
    ImageList_Add(hImageList, hBitMap, hBitMap);
    DeleteObject(hBitMap);

    hBitMap = LoadBitmap(ghAppInstance, MAKEINTRESOURCE(IDB_TREE_MYMUSIC));
    ImageList_Add(hImageList, hBitMap, hBitMap);
    DeleteObject(hBitMap);

	hBitMap = LoadBitmap(ghAppInstance, MAKEINTRESOURCE(IDB_TREE_ARCHIVE));
    ImageList_Add(hImageList, hBitMap, hBitMap);
    DeleteObject(hBitMap);

    TreeView_SetImageList(lhTreeHandle, hImageList, TVSIL_NORMAL);

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void FolderTree::SetEditCursorToEnd(void)
{
	int iLength = (int)SendMessage(lhSaveToLocation, WM_GETTEXTLENGTH, 0, 0);

	SendMessage(lhSaveToLocation, EM_SETSEL, (WPARAM)iLength, (LPARAM)iLength);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL FolderTree::FolderTreeHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if(uMsg == WM_NOTIFY)
    {
        NMHDR *lpNMH = (LPNMHDR) lParam;

        if(lpNMH->code == TVN_ITEMEXPANDING || lpNMH->code == TVN_SELCHANGED)
        {
            HTREEITEM hItem = NULL;
            HTREEITEM hParent = NULL;

            char sFullSelectedPath[MAX_PATH + 1];
            char sSubIndexFullPath[MAX_PATH + 1];
            char sItemText[MAX_PATH + 1];
            char sSelectedPath[MAX_PATH + 1];
            char sParent[MAX_PATH + 1];

			int iImage = 0;
            int iRPos = 0;
            int iWPos = MAX_PATH;

            if(lpNMH->code == TVN_ITEMEXPANDING) // If the use is expanding an item.
            {
                LPNMTREEVIEW pnmtv = (LPNMTREEVIEW) lParam;
                hItem = pnmtv->itemNew.hItem;
                TreeView_SelectItem(lhTreeHandle, hItem);
            }
            else { // The user has selected a new tree item
                hItem = TreeView_GetSelection(lhTreeHandle);
            }

			iImage = this->GetTreeItemImage(hItem);
            this->GetTreeItemText(hItem, sItemText);

			if(iImage != TREE_IMAGE_FIXED
				&& iImage != TREE_IMAGE_REMOVABLE
				&& iImage != TREE_IMAGE_USB
				&& iImage != TREE_IMAGE_NETWORK
				&& iImage != TREE_IMAGE_RAMDRIVE
				&& iImage != TREE_IMAGE_DESKTOP
				&& iImage != TREE_IMAGE_MYDOCUMENTS
				&& iImage != TREE_IMAGE_MYPICTURES
				&& iImage != TREE_IMAGE_MYMOVIES
				&& iImage != TREE_IMAGE_MYMUSIC
				&& iImage != TREE_IMAGE_CLOSEDFOLDER
				&& iImage != TREE_IMAGE_OPENFOLDER
				&& iImage != TREE_IMAGE_SECUREARCHIVE)
            {
                EnableWindow(lhButtonHandle, FALSE);
                return TRUE;
            }

            hParent = hItem;

            // Is the desktop itself selected?
            if(iImage == TREE_IMAGE_DESKTOP)
            {
                if(Get_DesktopDirectory(sFullSelectedPath, sizeof(sFullSelectedPath)))
                {
                    if(sFullSelectedPath[strlen(sFullSelectedPath) - 1] != '\\')
                    {
                        strcat_s(sFullSelectedPath, sizeof(sFullSelectedPath), "\\");
                    }
                    
					Set_Text(lhSaveToLocation, sFullSelectedPath);
					SetEditCursorToEnd();

                    EnableWindow(lhButtonHandle, TRUE);
                }
                return TRUE;
            }
            // Is "MyDocuments" desktop itself selected?
            else if(iImage == TREE_IMAGE_MYDOCUMENTS)
            {
                if(Get_DocumentsDirectory(sFullSelectedPath, sizeof(sFullSelectedPath)))
                {
                    if(sFullSelectedPath[strlen(sFullSelectedPath) - 1] != '\\')
                    {
                        strcat_s(sFullSelectedPath, sizeof(sFullSelectedPath), "\\");
                    }
                    
					Set_Text(lhSaveToLocation, sFullSelectedPath);
					SetEditCursorToEnd();

                    EnableWindow(lhButtonHandle, TRUE);
                }
                return TRUE;
            }

            bool bAppendDesktop = false;
			bool bAppendDocuments = false;

            // Find out it the item that is selected is a child of the desktop.
            // If it is, set bAppendDesktop to true.
            while((hParent = TreeView_GetParent(lhTreeHandle, hParent)))
            {
                GetTreeItemText(hParent, sParent);

				if(GetTreeItemImage(hParent) == TREE_IMAGE_MYCOMPUTER)
                {
                    break;
                }
                else if(GetTreeItemImage(hParent) == TREE_IMAGE_DESKTOP)
                {
                    bAppendDesktop = true;
                    break;
                }
                else if(GetTreeItemImage(hParent) == TREE_IMAGE_MYDOCUMENTS)
                {
                    bAppendDocuments = true;
                    break;
                }

            }

			if(iImage != TREE_IMAGE_SECUREARCHIVE)
			{
				// Append the name of the selected item to "sSelectedPath" in reverse
				iRPos = (int)strlen(sItemText) - 1;
				while(iRPos != -1)
				{
					if(sItemText[iRPos] == '\0')
					{
						break;
					}

					sSelectedPath[iWPos--] = sItemText[iRPos--];
				}
			}
			else{
				if(lhSaveToFile)
				{
					Set_Text(this->lhSaveToFile, sItemText);
				}
			}

            //ErrorMessage(NULL, sSelectedPath);

            hParent = hItem;

            // Loop through parents of the selected item.
            // Appending there names to sSelectedPath in reverse.
            while( (hParent = TreeView_GetParent(lhTreeHandle, hParent)))
            {
                GetTreeItemText(hParent, sItemText);

				if(GetTreeItemImage(hParent) == TREE_IMAGE_MYCOMPUTER
					|| GetTreeItemImage(hParent) == TREE_IMAGE_DESKTOP
					|| GetTreeItemImage(hParent) == TREE_IMAGE_MYDOCUMENTS)
                {
                    break;
                }

                sSelectedPath[iWPos--] = '\\';

                iRPos = (int)strlen(sItemText) -1;
                while(iRPos != -1)
                {
                    if(sItemText[iRPos] == '\0')
					{
						break;
					}

                    sSelectedPath[iWPos--] = sItemText[iRPos--];
                }
            }


            // Mirror "sSelectedPath" to sFullSelectedPath (Reverse)
            // Data from the end of "sSelectedPath" to the beginning of "sFullSelectedPath" and backwards.
            iRPos = iWPos + 1;
            iWPos = 0;
            while(iRPos != (MAX_PATH + 1))
            {
                if(sSelectedPath[iRPos] == '\0')
				{
					break;
				}

                sFullSelectedPath[iWPos++] = sSelectedPath[iRPos++];
            }

            //SafeMsgBox(GetActiveWindow(), sFullSelectedPath, "sFullSelectedPath", 0);

            sFullSelectedPath[iWPos] = '\0';

            // Is the item that is selected a child of the desktop?
            // If so append its path to the end of the desktop path.
            if(bAppendDesktop)
            {
                if(Get_DesktopDirectory(sSelectedPath, sizeof(sSelectedPath)))
                {
                    if(sSelectedPath[strlen(sSelectedPath) - 1] != '\\')
                    {
                        strcat_s(sSelectedPath, sizeof(sSelectedPath), "\\");
                    }

                    strcat_s(sSelectedPath, sizeof(sSelectedPath), sFullSelectedPath); // Append "sSelectedPath" to the desktop path
                    strcpy_s(sSubIndexFullPath, sizeof(sSubIndexFullPath), sSelectedPath); // Save "sSubIndexFullPath" for sub indexing

					if(iImage != TREE_IMAGE_SECUREARCHIVE)
					{
						strcat_s(sSelectedPath, sizeof(sSelectedPath), "\\");
					}
                    Set_Text(lhSaveToLocation, sSelectedPath);
					SetEditCursorToEnd();

                    EnableWindow(lhButtonHandle, TRUE);
                }
            }
            else if(bAppendDocuments)
            {
                if(Get_DocumentsDirectory(sSelectedPath, sizeof(sSelectedPath)))
                {
                    if(sSelectedPath[strlen(sSelectedPath) - 1] != '\\')
                    {
                        strcat_s(sSelectedPath, sizeof(sSelectedPath), "\\");
                    }

                    strcat_s(sSelectedPath, sizeof(sSelectedPath), sFullSelectedPath); // Append "sSelectedPath" to the desktop path
                    strcpy_s(sSubIndexFullPath, sizeof(sSubIndexFullPath), sSelectedPath); // Save "sSubIndexFullPath" for sub indexing

					if(iImage != TREE_IMAGE_SECUREARCHIVE)
					{
						strcat_s(sSelectedPath, sizeof(sSelectedPath), "\\");
					}
                    Set_Text(lhSaveToLocation, sSelectedPath);
					SetEditCursorToEnd();

                    EnableWindow(lhButtonHandle, TRUE);
                }
            }
            else{
                strcpy_s(sSubIndexFullPath, sizeof(sSubIndexFullPath), sFullSelectedPath); // Save "sSubIndexFullPath" for sub indexing

				if(sFullSelectedPath[strlen(sFullSelectedPath) - 1] != '\\')
				{
					strcat_s(sFullSelectedPath, sizeof(sFullSelectedPath), "\\");
				}
                Set_Text(lhSaveToLocation, sFullSelectedPath);
				SetEditCursorToEnd();
            }

			if(iImage != TREE_IMAGE_SECUREARCHIVE)
			{
				IndexDirectoryForSubItem(hItem, sSubIndexFullPath, false, false);

				//---------------------------------------------------------------------------
				// Sub Indexing (Begin)
				// This code it designed to load the directory's into the tree "one level lower"
				// than the user currently has selected. So when (if) they do expand the path
				// its contents are already loaded.

				HTREEITEM hSibling = NULL;

				strcpy_s(sSelectedPath, sizeof(sSelectedPath), sSubIndexFullPath);

				if((hSibling = TreeView_GetChild(lhTreeHandle, hItem)) != NULL)
				{
					GetTreeItemText(hSibling, sItemText);

					strcat_s(sSubIndexFullPath, sizeof(sSubIndexFullPath), "\\");
					strcat_s(sSubIndexFullPath, sizeof(sSubIndexFullPath), sItemText);

					IndexDirectoryForSubItem(hSibling, sSubIndexFullPath, false, false);

					while((hSibling = TreeView_GetNextSibling(lhTreeHandle, hSibling)) != NULL)
					{
						GetTreeItemText(hSibling, sItemText);

						strcpy_s(sSubIndexFullPath, sizeof(sSubIndexFullPath), sSelectedPath);
						strcat_s(sSubIndexFullPath, sizeof(sSubIndexFullPath), "\\");
						strcat_s(sSubIndexFullPath, sizeof(sSubIndexFullPath), sItemText);

						IndexDirectoryForSubItem(hSibling, sSubIndexFullPath, false, false);
					}
				}

				// Sub Indexing (End)
				//---------------------------------------------------------------------------
			}

            EnableWindow(lhButtonHandle, TRUE);

            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
