#ifndef _ReplaceFileDlg_CPP
#define _ReplaceFileDlg_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "../Source/Package.H"
#include "../Source/Entry.H"
#include "../Source/Routines.H"

#include "../Dialogs/MainDlg.H"
#include "../Dialogs/ReplaceFileDlg.H"
#include "../Dialogs/NetLogoDlg.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows;
using namespace NSWFL::String;
using namespace NSWFL::File;
using namespace NSWFL::Conversion;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------(Variable Declarations)
HWND hReplaceFile = NULL;

char sReplaceFile[MAX_PATH];
char sReplaceFileFull[MAX_PATH];

FILETIME ftGCreated;
FILETIME ftGModified;
__int64 i64GFileSz = 0;
int iGPkgAttribs = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK ReplaceFile(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//--------------------------------------------------------------------------

	static HICON hLargeIcon = NULL;

	//--------------------------------------------------------------------------

	if (uMsg == WM_INITDIALOG) // Received an Initialize Dialog Message
	{
		hReplaceFile = hWnd;

		HMENU hSystemMenu = GetSystemMenu(hWnd, FALSE);
		AppendMenu(hSystemMenu, MF_SEPARATOR, 0, 0);
		AppendMenu(hSystemMenu, MF_STRING, WM_ABOUT_DIALOG, "About");

		SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Replace File?");
		SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM)LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN)));

		SetStatusFileName(GetDlgItem(hWnd, IDC_REPLACEQUE), sReplaceFile);

		//---------------------------------------------------------------------------------------------------------------------------------|
		//--[BEGIN HANDLE ICON]------------------------------------------------------------------------------------------------------------|
		//---------------------------------------------------------------------------------------------------------------------------------|

		char sExt[MAX_PATH];
		char sIconFile[MAX_PATH];
		char sTemp[1024];
		char sFileSz[1024];
		char sCreated[128];
		char sModified[128];
		char sDescr[2048];
		int iIconIndex = 0;

		_splitpath_s(sReplaceFile, NULL, 0, NULL, 0, NULL, 0, sExt, sizeof(sExt));

		if (GetIconForExt(sExt, sIconFile, sizeof(sIconFile), &iIconIndex, sDescr, sizeof(sDescr)))
		{
			if (strlen(sIconFile) > 0)
			{
				if (strcmp(sIconFile, "%1") == 0)
				{
					strcpy_s(sIconFile, sizeof(sIconFile), sReplaceFileFull);
				}

				if (!ExtractIconEx(sIconFile, iIconIndex, &hLargeIcon, NULL, 1))
				{
					hLargeIcon = LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_UNKNOWN32));
				}
			}
			else {
				hLargeIcon = LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_UNKNOWN32));
			}
		}
		else {
			hLargeIcon = LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_UNKNOWN32));
		}

		SendMessage(GetDlgItem(hWnd, IDC_REPICO1), (UINT)STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)(HICON)hLargeIcon);
		SendMessage(GetDlgItem(hWnd, IDC_REPICO2), (UINT)STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)(HICON)hLargeIcon);

		//---------------------------------------------------------------------------------------------------------------------------------|
		//--[END HANDLE ICON]--------------------------------------------------------------------------------------------------------------|
		//---------------------------------------------------------------------------------------------------------------------------------|

		sprintf_s(sTemp, sizeof(sTemp), "%I64d", i64GFileSz);
		FormatiString(sFileSz, sizeof(sFileSz), sTemp);

		FileTimeToString(&ftGCreated, (bool)!(iGPkgAttribs & ATTRIB_DATETIME), sCreated, sizeof(sCreated));
		FileTimeToString(&ftGModified, (bool)!(iGPkgAttribs & ATTRIB_DATETIME), sModified, sizeof(sModified));

		sprintf_s(sDescr, sizeof(sDescr), "%s Bytes", sFileSz);
		Set_Text(GetDlgItem(hWnd, IDC_REPSOURCE), sDescr);
		sprintf_s(sDescr, sizeof(sDescr), "%s / %s", sCreated, sModified);
		Set_Text(GetDlgItem(hWnd, IDC_REPSOURCENFO), sDescr);

		//------------------------------------------------

		HANDLE hfSource = CreateFile(sReplaceFileFull, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hfSource == INVALID_HANDLE_VALUE)
		{
			Set_Text(GetDlgItem(hWnd, IDC_REPTARGET), "N/A");
			Set_Text(GetDlgItem(hWnd, IDC_REPTARGETNFO), "N/A");
		}
		else {
			__int64 i64FileSize = 0;

			if (FileSize(hfSource, &i64FileSize))
			{
				sprintf_s(sTemp, sizeof(sTemp), "%I64d", i64FileSize);
				FormatiString(sFileSz, sizeof(sFileSz), sTemp);
			}
			else {
				strcpy_s(sTemp, sizeof(sTemp), "<Unknown>");
			}

			GetFileTime(hfSource, &ftGCreated, NULL, &ftGModified);
			CloseHandle(hfSource);

			FileTimeToString(&ftGCreated, false, sCreated, sizeof(sCreated));
			FileTimeToString(&ftGModified, false, sModified, sizeof(sModified));

			sprintf_s(sDescr, sizeof(sDescr), "%s Bytes", sFileSz);
			Set_Text(GetDlgItem(hWnd, IDC_REPTARGET), sDescr);
			sprintf_s(sDescr, sizeof(sDescr), "%s / %s", sCreated, sModified);
			Set_Text(GetDlgItem(hWnd, IDC_REPTARGETNFO), sDescr);
		}

		CenterOverOwner(hWnd);

		if (!IsWindowVisible(GetParent(hWnd)))
		{
			ShowWindowInTaskbar(hWnd, true);
		}

		SetFocus(GetDlgItem(hWnd, IDC_OPNO));

		return FALSE; // Return TRUE to set the keyboard focus, Otherwise return FALSE
	}

	//--------------------------------------------------------------------------

	if (uMsg == WM_COMMAND)
	{
		if (wParam == IDC_OPYES || wParam == IDC_OPNO || wParam == IDC_OPYESTOALL || wParam == IDC_OPCANCEL)
		{
			DestroyIcon(hLargeIcon);
			DeleteObject(hLargeIcon);

			EndDialog(hWnd, wParam);
			return TRUE;
		}
		if (wParam == IDOK)
		{
			DestroyIcon(hLargeIcon);
			DeleteObject(hLargeIcon);

			EndDialog(hWnd, IDC_OPYES);
			return TRUE;
		}
		if (wParam == IDCANCEL)
		{
			DestroyIcon(hLargeIcon);
			DeleteObject(hLargeIcon);

			EndDialog(hWnd, IDC_OPNO);
			return TRUE;
		}

		return FALSE;
	}

	//--------------------------------------------------------------------------

	if (uMsg == WM_PAINT)
	{
		HDC ThisHDC;
		PAINTSTRUCT ThisPS;

		ThisHDC = BeginPaint(hWnd, &ThisPS);

		// Any painting should be done here

		EndPaint(hWnd, &ThisPS);
		return TRUE;
	}

	//--------------------------------------------------------------------------

	if (uMsg == WM_SYSCOMMAND) // Received A System Menu Message
	{
		if (LOWORD(wParam) == WM_ABOUT_DIALOG) //- About (SysMenu Command)
		{
			_AboutDialogInfo ADI;

			ADI.DisplayIcon = LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN));
			ADI.TitleCaption = gsTitleCaption;
			ADI.FileVersion = gsFileVersion;
			ADI.BuildDate = __DATE__;
			ADI.BuildTime = __TIME__;
			ADI.CopyRight = gsAppCopyRight;
			ADI.OwnerHandle = hWnd;

			NetLogo(&ADI);
			return TRUE;
		}

		return FALSE;
	}

	//--------------------------------------------------------------------------

	if (uMsg == WM_CLOSE)  // Received Close Message
	{
		DestroyIcon(hLargeIcon);
		DeleteObject(hLargeIcon);

		EndDialog(hWnd, IDC_OPCANCEL);
		return TRUE;
	}

	//--------------------------------------------------------------------------

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

