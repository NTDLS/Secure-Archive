///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	MODULE:   ctxmenu.cpp
//
//	PURPOSE:   Implements the IContextMenu member functions necessary to support
//			   the context menu portioins of this shell extension.	Context menu
//			   shell extensions are called when the user right clicks on a file
//			   (of the type registered for the shell extension--see SHELLEXT.REG
//			   for details on the registry entries.  In this sample, the relevant
//			   files are of type .GAK) in the Explorer, or selects the File menu
//			   item.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.h>
#include <WindowsX.h>
#include <ShlObj.h>
#include <StdIO.H>
#include <VersionHelpers.H>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Priv.h"
#include "ShellExt.h"

#include "../../../@Libraries/NSWFL/NSWFL.H"
#include "../Resources/Resource.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::File;
using namespace NSWFL::Registry;
using namespace NSWFL::Graphics;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define DEFAULTEXT          ".SAef"
#define DEFAULTHARDKEYEXT   ".SAkf"

#define ACTION_ENCRYPT 1
#define ACTION_DECRYPT 2
#define ACTION_SELFEXE 3

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef unsigned long (WINAPI *PGetExeEnd)(void);

int iAllowedAction = -1;
int iEncryptTo = -1;
int iDecryptToSpecified = -1;
int iDecryptToCurrent = -1;
int iViewContents = -1;
int iCreateSelfExe = -1;
int iTestArchive = -1;
int iSecureDelete = -1;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int AppendMenuEx(HMENU hMenu, unsigned int iFlags,
				unsigned int *iCommand, unsigned int *iSlot, const char *sText)
{
	AppendMenu(hMenu, iFlags, (int)(*iCommand)++, sText);
	return (int)(*iSlot)++;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int InsertMenuEx(HMENU hMenu, unsigned int iFlags,
				unsigned int *iCommand, unsigned int *iSlot, const char *sText)
{
	InsertMenu(hMenu, (int)(*iSlot), iFlags, (int)(*iCommand)++, sText);
	return (int)(*iSlot)++;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned long GetExeEnd(const char *sFileName)
{
	unsigned long lExeEnd = 0;

	if(FileAccess(sFileName, FExist))
	{
		char sExtension[MAX_PATH];
		GetFileExtension(sFileName, sExtension, sizeof(sExtension));
		if(_strcmpi(sExtension, ".EXE") == 0)
		{
			HMODULE hExe = LoadLibrary(sFileName);
			if(hExe)
			{
				PGetExeEnd pGetExeEnd = NULL;
				if((pGetExeEnd = (PGetExeEnd) GetProcAddress(hExe, "ExGetExeEnd")))
				{
					lExeEnd = pGetExeEnd();
				}
				FreeLibrary(hExe);
			}
		}
	}

	return lExeEnd;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool DoesPathContainUnicode(const char *sPath, size_t iPathLen)
{
	for(size_t iPos = 0; iPos < iPathLen; iPos++)
	{
		if(sPath[iPos] == '?')
		{
			return true;
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool DoesPathContainUnicode(const char *sPath)
{
	return DoesPathContainUnicode(sPath, strlen(sPath));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	FUNCTION: CShellExt::QueryContextMenu(HMENU, UINT, UINT, UINT, UINT)
//
//	PURPOSE: Called by the shell just before the context menu is displayed.
//			 This is where you add your specific menu items.
//
//	PARAMETERS:
//	  hMenu 	 - Handle to the context menu
//	  indexMenu  - Index of where to begin inserting menu items
//	  idCmdFirst - Lowest value for new menu ID's
//	  idCmtLast  - Highest value for new menu ID's
//	  uFlags	 - Specifies the context of the menu event
//
//	RETURN VALUE:
//
//
//	COMMENTS:
//

STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
	UINT idCmd = idCmdFirst;
	UINT idIndex = indexMenu;

	if(((uFlags & 0x000F) == CMF_NORMAL) || (uFlags & CMF_EXPLORE))
	{
		char sTemp[MAX_PATH];
		char sFile[MAX_PATH];
		char sFolder[MAX_PATH];
		char sDrive[MAX_PATH];
		char sExt[MAX_PATH];

		unsigned int iSlot = 0;

		STGMEDIUM stgMedium;
		UINT uFileCount = 0;
		HRESULT hres   = 0;
		FORMATETC fmte = {CF_HDROP, (DVTARGETDEVICE FAR *)NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

		if(m_pDataObj)  //Paranoid check, m_pDataObj should have something by now...
		{
		   hres = m_pDataObj->GetData(&fmte, &stgMedium);
		}

		if(SUCCEEDED(hres))
		{
			if(stgMedium.hGlobal)
			{
				uFileCount = DragQueryFile((HDROP)stgMedium.hGlobal, (UINT)-1, 0, 0);
			}

			DragQueryFile((HDROP)stgMedium.hGlobal, 0, sFileUserClickedOn, sizeof(sFileUserClickedOn));

			if(uFileCount == 1 && !DoesPathContainUnicode(sFileUserClickedOn))
			{
				iAllowedAction = 0;

				_splitpath_s(sFileUserClickedOn,
					sDrive, sizeof(sDrive),
					sFolder, sizeof(sFolder),
					sFile, sizeof(sFile),
					sExt, sizeof(sExt));
				strcat_s(sDrive, sizeof(sDrive), sFolder);

				if(_strcmpi(sExt, DEFAULTEXT) == 0 && !IsDirectory(sFileUserClickedOn))
				{
					iAllowedAction = ACTION_DECRYPT;
				}
				else {
					if(_strcmpi(sExt, ".exe") == 0 && !IsDirectory(sFileUserClickedOn))
					{
						if(GetExeEnd(sFileUserClickedOn) > 0)
						{
							iAllowedAction = ACTION_SELFEXE;
						}
						else{
							iAllowedAction = ACTION_ENCRYPT;
						}
					}
					else{
						iAllowedAction = ACTION_ENCRYPT;
					}
				}

				HBITMAP hBitmap = NULL;
				
				if (IsWindowsVistaOrGreater())
				{
					hBitmap = (HBITMAP)LoadImage(ghModThisDLL, MAKEINTRESOURCE(IDB_24BITMENUITEM), IMAGE_BITMAP, 16, 16, 0);
				}
				else
				{
					hBitmap = LoadBitmap(ghModThisDLL, MAKEINTRESOURCE(IDB_16BITMENUITEM));
				}
			

				HMENU hCmdMenu = CreateMenu();

				if(iAllowedAction == ACTION_ENCRYPT)
				{
					iEncryptTo = InsertMenuEx(hCmdMenu, MF_BYCOMMAND|MF_STRING, &idCmd, &iSlot, "Encrypt to...");
					AppendMenuEx(hCmdMenu, MF_BYCOMMAND|MF_STRING|MF_SEPARATOR, &idCmd, &iSlot, "");
					iSecureDelete = InsertMenuEx(hCmdMenu, MF_BYCOMMAND|MF_STRING, &idCmd, &iSlot, "Secure Delete");
				}
				else if(iAllowedAction == ACTION_DECRYPT || iAllowedAction == ACTION_SELFEXE)
				{
					iDecryptToSpecified = AppendMenuEx(hCmdMenu, MF_BYCOMMAND|MF_STRING, &idCmd, &iSlot, "Decrypt to...");

					sprintf_s(sTemp, sizeof(sTemp), "Decrypt to %s", sDrive);
					iDecryptToCurrent = AppendMenuEx(hCmdMenu, MF_BYCOMMAND|MF_STRING, &idCmd, &iSlot, sTemp);

					iViewContents = AppendMenuEx(hCmdMenu, MF_BYCOMMAND|MF_STRING, &idCmd, &iSlot, "View Contents");

					if(iAllowedAction != ACTION_SELFEXE)
					{
						iCreateSelfExe = AppendMenuEx(hCmdMenu, MF_STRING, &idCmd, &iSlot, "Create Self-Extractor");
						AppendMenuEx(hCmdMenu, MF_BYCOMMAND|MF_STRING|MF_SEPARATOR, &idCmd, &iSlot, "");
					}

					iTestArchive = AppendMenuEx(hCmdMenu, MF_BYCOMMAND|MF_STRING, &idCmd, &iSlot, "Test Archive");
				}

				InsertMenu(hMenu, idIndex++, MF_POPUP|MF_BYPOSITION, (UINT_PTR)hCmdMenu, "Secure Archive");
				SetMenuItemBitmaps(hMenu, idIndex - 1, MF_BYPOSITION, hBitmap, hBitmap);
			}
		}

		return MAKE_HRESULT(SEVERITY_SUCCESS, 0, idCmd - idCmdFirst + 1);
		//return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (idIndex - 1) - indexMenu + 1);
		//return ResultFromShort(idCmd-idIndex);
	}
	/*
	if (uFlags & CMF_VERBSONLY)
	{
		idIndex++;
		InsertMenu(hMenu, idIndex++, MF_STRING|MF_BYPOSITION, idCmd++, "Encrypt Shortcut");

		HBITMAP BitMapResource = LoadBitmap(ghModThisDLL, MAKEINTRESOURCE(100) );
		SetMenuItemBitmaps(hMenu, idIndex, MF_BYPOSITION, BitMapResource, BitMapResource);

		return ResultFromShort(idCmd-idCmdFirst);
	}
	if (uFlags & CMF_EXPLORE)
	{
		idIndex++;
		InsertMenu(hMenu, idIndex++, MF_STRING|MF_BYPOSITION, idCmd++, "Open with Secure Archive");

		HBITMAP BitMapResource = LoadBitmap(ghModThisDLL, MAKEINTRESOURCE(IDB_SHELLMENU) );
		SetMenuItemBitmaps(hMenu, idIndex, MF_BYPOSITION, BitMapResource, BitMapResource);

		return ResultFromShort(idCmd-idCmdFirst);
	}
*/
	if (uFlags & CMF_DEFAULTONLY)
	{
		return NOERROR;
	}

   return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
	//This may be required for a 64bit context extension, discovery needed.
*/
/*
HRESULT CShellExt::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
	HRESULT hr = E_INVALIDARG;

	if(iAllowedAction == ACTION_ENCRYPT)
	{
		if(idCmd == iEncryptTo)
		{
		}
	}
	else if(iAllowedAction == ACTION_DECRYPT || iAllowedAction == ACTION_SELFEXE)
	{
		if(idCmd == iDecryptToSpecified)
		{
		}
		else if(idCmd == iDecryptToCurrent)
		{
		
		}
		else if(idCmd == iTestArchive)
		{

		}
		else if(idCmd == iCreateSelfExe)
		{

		}
		else if(idCmd == iViewContents)
		{

		}
	}

	return S_OK;


//hr = StringCchCopyNW((LPWSTR)pszName, cchMax, menuDescU_.c_str(), cchMax);

	return hr;
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	FUNCTION: CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO)
//
//	PURPOSE: Called by the shell after the user has selected on of the
//			 menu items that was added in QueryContextMenu().
//
//	PARAMETERS:
//	  lpcmi - Pointer to an CMINVOKECOMMANDINFO structure
//
//	RETURN VALUE:
//
//
//	COMMENTS:
//

STDMETHODIMP CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
	HRESULT hr = E_INVALIDARG;

	//If HIWORD(lpcmi->lpVerb) then we have been called programmatically
	//and lpVerb is a command that should be invoked.  Otherwise, the shell
	//has called us, and LOWORD(lpcmi->lpVerb) is the menu ID the user has
	//selected.  Actually, it's (menu ID - idCmdFirst) from QueryContextMenu().
	if(!HIWORD(lpcmi->lpVerb))
	{
		UINT idCmd = LOWORD(lpcmi->lpVerb);

		char sFile[MAX_PATH];
		char sFolder[MAX_PATH];
		char sDrive[MAX_PATH];
		char sParams[MAX_PATH * 2];
		char sExeName[MAX_PATH];
		unsigned long SizeOfsExeName = MAX_PATH;

		if(!Get_StringRegistryValue(HKEY_LOCAL_MACHINE, REGISTRYPATHKEY, "Secure Archive", sExeName, SizeOfsExeName))
		{
			MessageBox(NULL, "Secure Archive executable path key not found."
				"\r\nPlease reinstall Secure Archive." ,"Execute Error", MB_ICONHAND | MB_SYSTEMMODAL);
			return S_OK;
		}

		if(iAllowedAction == ACTION_ENCRYPT)
		{
			if(idCmd == iEncryptTo)
			{
				char sTarget[MAX_PATH];

				//Did the user Click on a disk?
				if(strlen(sFileUserClickedOn) <= 3 && sFileUserClickedOn[1] == ':')
				{
					sprintf_s(sTarget, sizeof(sTarget),
						"%s\\Drive %c", sFileUserClickedOn, sFileUserClickedOn[0]);
					CorrectReversePath(sFileUserClickedOn, sizeof(sFileUserClickedOn));
				}
				else{
					strcpy_s(sTarget, sizeof(sTarget), sFileUserClickedOn);
				}

				sprintf_s(sParams, sizeof(sParams),
					"/Source:\"%s\""
					" /Target:\"%s%s\""
					" /Encrypt"
					" /AutoClose",
					sFileUserClickedOn, sTarget, DEFAULTEXT);
			}
			else if(idCmd == iSecureDelete)
			{
				sprintf_s(sParams, sizeof(sParams),
					"/SecureDelete:\"%s\"",
					sFileUserClickedOn);
			}
		}
		else if(iAllowedAction == ACTION_DECRYPT || iAllowedAction == ACTION_SELFEXE)
		{
			if(idCmd == iDecryptToSpecified)
			{
				sprintf_s(sParams, sizeof(sParams),
					"/Source:\"%s\""
					" /Decrypt"
					" /AutoClose",
					sFileUserClickedOn);
			}
			else if(idCmd == iDecryptToCurrent)
			{
				char sNewFile[MAX_PATH];

				if(IsDirectory(sFileUserClickedOn))
				{
					strcat_s(sNewFile, sizeof(sNewFile), sFileUserClickedOn);
				}
				else{
					_splitpath_s(sFileUserClickedOn, sDrive, sizeof(sDrive), sFolder, sizeof(sFolder), sFile, sizeof(sFile), NULL, NULL);
					sprintf_s(sNewFile, sizeof(sNewFile), "%s%s", sDrive, sFolder);

					size_t iLen = strlen(sNewFile);
					if(sNewFile[iLen - 1] == '/' || sNewFile[iLen - 1] == '\\')
					{
						sNewFile[iLen - 1] = '\0';
					}
				}
				
				sprintf_s(sParams, sizeof(sParams),
					"/Source:\"%s\""
					" /Target:\"%s\""
					" /Decrypt"
					" /AutoClose",
					sFileUserClickedOn, sNewFile);
			}
			else if(idCmd == iTestArchive)
			{
				sprintf_s(sParams, sizeof(sParams),
					"/Test:\"%s\"",
					sFileUserClickedOn);
			}
			else if(idCmd == iCreateSelfExe)
			{
				char sNewFile[MAX_PATH];
				_splitpath_s(sFileUserClickedOn, sDrive, sizeof(sDrive),
					sFolder, sizeof(sFolder), sFile, sizeof(sFile), NULL, NULL);
				_makepath_s(sNewFile, sizeof(sNewFile), sDrive, sFolder, sFile, ".exe");

				sprintf_s(sParams, sizeof(sParams),
					"/Source:\"%s\""
					" /Target:\"%s\""
					" /CreateSelfExtractor"
					" /AutoClose",
					sFileUserClickedOn, sNewFile);
			}
			else if(idCmd == iViewContents)
			{
				sprintf_s(sParams, sizeof(sParams),
					"/Source:\"%s\""
					" /View",
					sFileUserClickedOn);
			}
		}

		HANDLE Result = ShellExecute(lpcmi->hwnd, NULL, sExeName, sParams, lpcmi->lpDirectory, lpcmi->nShow);

		if(Result == (VOID *) 0)					  MessageBox(NULL, "The operating system is out of memory or resources."											   ,"Execute Error", MB_ICONHAND + MB_SYSTEMMODAL);
		if(Result == (VOID *) ERROR_FILE_NOT_FOUND)   MessageBox(NULL, "The specified file was not found."																   ,"Execute Error", MB_ICONHAND + MB_SYSTEMMODAL);
		if(Result == (VOID *) ERROR_PATH_NOT_FOUND)   MessageBox(NULL, "The specified path was not found."																   ,"Execute Error", MB_ICONHAND + MB_SYSTEMMODAL);
		if(Result == (VOID *) ERROR_BAD_FORMAT) 	  MessageBox(NULL, "The .EXE file is invalid (non-Win32 .EXE or error in .EXE image)."								   ,"Execute Error", MB_ICONHAND + MB_SYSTEMMODAL);
		if(Result == (VOID *) SE_ERR_ACCESSDENIED)	  MessageBox(NULL, "The operating system denied access to the specified file."										   ,"Execute Error", MB_ICONHAND + MB_SYSTEMMODAL);
		if(Result == (VOID *) SE_ERR_ASSOCINCOMPLETE) MessageBox(NULL, "The filename association is incomplete or invalid." 											   ,"Execute Error", MB_ICONHAND + MB_SYSTEMMODAL);
		if(Result == (VOID *) SE_ERR_DDEBUSY)		  MessageBox(NULL, "The DDE transaction could not be completed because other DDE transactions were being processed."   ,"Execute Error", MB_ICONHAND + MB_SYSTEMMODAL);
		if(Result == (VOID *) SE_ERR_DDEFAIL)		  MessageBox(NULL, "The DDE transaction failed."																	   ,"Execute Error", MB_ICONHAND + MB_SYSTEMMODAL);
		if(Result == (VOID *) SE_ERR_DDETIMEOUT)	  MessageBox(NULL, "The DDE transaction could not be completed because the request timed out."						   ,"Execute Error", MB_ICONHAND + MB_SYSTEMMODAL);
		if(Result == (VOID *) SE_ERR_DLLNOTFOUND)	  MessageBox(NULL, "The specified dynamic-link library was not found."												   ,"Execute Error", MB_ICONHAND + MB_SYSTEMMODAL);
		if(Result == (VOID *) SE_ERR_NOASSOC)		  MessageBox(NULL, "There is no application associated with the given filename extension."							   ,"Execute Error", MB_ICONHAND + MB_SYSTEMMODAL);
		if(Result == (VOID *) SE_ERR_OOM)			  MessageBox(NULL, "There was not enough memory to complete the operation." 										   ,"Execute Error", MB_ICONHAND + MB_SYSTEMMODAL);
		if(Result == (VOID *) SE_ERR_PNF)			  MessageBox(NULL, "The specified path was not found."																   ,"Execute Error", MB_ICONHAND + MB_SYSTEMMODAL);
		if(Result == (VOID *) SE_ERR_SHARE) 		  MessageBox(NULL, "A sharing violation occurred."																	   ,"Execute Error", MB_ICONHAND + MB_SYSTEMMODAL);

		return NOERROR;
	}

	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//
//	FUNCTION: CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO)
//
//	PURPOSE: Called by the shell after the user has selected on of the
//			 menu items that was added in QueryContextMenu().
//
//	PARAMETERS:
//	  lpcmi - Pointer to an CMINVOKECOMMANDINFO structure
//
//	RETURN VALUE:
//
//
//	COMMENTS:
//

HRESULT STDMETHODCALLTYPE CShellExt::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pReserved, LPSTR pszName, UINT cchMax)
{
	switch (idCmd)
	{
		case 0:
			if(uType & GCS_VERBA)
			{
				lstrcpyn(pszName, "Open this file with Secure Archive.", cchMax);
			}
			else{
				lstrcpynW((LPWSTR)pszName, L"Open this file with Secure Archive.", cchMax);
			}

			break;
	}

	return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


