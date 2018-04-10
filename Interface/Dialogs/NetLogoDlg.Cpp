#ifndef _NETLOGODLG_CPP
#define _NETLOGODLG_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <StdIO.H>

#include "../Source/Entry.H"
#include "../Source/Routines.H"

#include "NetLogoDlg.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HWND hNetLogoDialog = NULL;
HINSTANCE hModuleHandle = NULL;
LPABOUTDLGINFO glpADI;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool NetLogo(LPABOUTDLGINFO lpADI)
{
	HWND OwnerHandle = NULL;

    glpADI = lpADI;

	if(glpADI->OwnerHandle == NULL)
        OwnerHandle = GetActiveWindow();
	else OwnerHandle = glpADI->OwnerHandle;

	hModuleHandle = GetModuleHandle(NULL);

	DialogBox(hModuleHandle, MAKEINTRESOURCE(IDD_NETLOGO), OwnerHandle, (DLGPROC)NetLogoDialog);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HWND NetLogoHandle(void)
{
	return hNetLogoDialog;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool IsNetLogoActive(void)
{
	return (hNetLogoDialog != NULL);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void EndNetLogo(void)
{
	EndDialog(hNetLogoDialog, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK NetLogoDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //--------------------------------------------------------------------------

    static HWND hAppName       = NULL;
    static HWND hAppVersion    = NULL;
    static HWND hAppCopyRight  = NULL;
    static HWND hDisplayIcon   = NULL;
    static HWND hBuildTimeDate = NULL;
	static HWND hNetLink       = NULL;

    static HCURSOR HandCursor      = NULL;

    static DWORD BGColorRef        = 0;

    //--------------------------------------------------------------------------

    if(uMsg == WM_INITDIALOG) // Received A Initilize Dialog Message
    {
        char sTempText[255];

        hNetLogoDialog = hWnd;

		HandCursor = LoadCursor(hModuleHandle, MAKEINTRESOURCE(IDC_HANDCURSOR));
        BGColorRef = GetSysColor(COLOR_3DFACE);

        sprintf_s(sTempText, sizeof(sTempText), "NetworkDLS :: %s", glpADI->TitleCaption);
        SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)sTempText);

        hAppName       = GetDlgItem(hWnd, IDC_APPNAME);
        hAppVersion    = GetDlgItem(hWnd, IDC_APPVERSION);
        hBuildTimeDate = GetDlgItem(hWnd, IDC_BUILDTIMEDATE);
        hAppCopyRight  = GetDlgItem(hWnd, IDC_APPCOPYRIGHT);
        hDisplayIcon   = GetDlgItem(hWnd, IDC_ABOUTICON);
        hNetLink       = GetDlgItem(hWnd, IDC_NETLINK);

        SendMessage(hDisplayIcon, (UINT)STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)glpADI->DisplayIcon);

        Set_Text(hAppName, glpADI->TitleCaption);

        Set_Text(hAppVersion, glpADI->FileVersion);

        sprintf_s(sTempText, sizeof(sTempText), "%s - %s", glpADI->BuildDate, glpADI->BuildTime);
        Set_Text(hBuildTimeDate, sTempText);

        Set_Text(hAppCopyRight, glpADI->CopyRight);

        CenterOverOwner(hWnd);

        return TRUE; // Return TRUE to set the keyboard focus, Otherwise return FALSE
    }

    //--------------------------------------------------------------------------

	if(IsMouseOverHwnd(hNetLink))
	{
		SetCursor(HandCursor);

		if(uMsg == WM_LBUTTONUP)
        {
			ShellExecute(0, "Open", "Http://www.NetworkDLS.com/", NULL, NULL, SW_SHOWNORMAL);
        }
	}

    //--------------------------------------------------------------------------

    if(uMsg == WM_CTLCOLORSTATIC)
	{
		if((HANDLE)lParam == hNetLink)
			return Set_Color(RGB(0, 0, 255), BGColorRef, wParam);

        return FALSE;
	}

    //--------------------------------------------------------------------------

    if(uMsg == WM_COMMAND)
    {
        if(wParam == IDC_OKBUTTON || wParam == IDOK || wParam == IDCANCEL)
        {
            EndDialog(hWnd, 0);
            DestroyWindow(hWnd);
            return TRUE;
        }
        return FALSE;
    }

    //--------------------------------------------------------------------------

	if(uMsg == WM_PAINT)
    {
        HDC ThisHDC;
        PAINTSTRUCT ThisPS;

        ThisHDC = BeginPaint(hWnd, &ThisPS);

        // Any painting should be done here

        EndPaint(hWnd, &ThisPS);
        return TRUE;
    }

    //--------------------------------------------------------------------------

	if(uMsg == WM_DESTROY)
    {
		FreeLibrary(hModuleHandle);
		hModuleHandle = NULL;
		hNetLogoDialog = NULL;
        return TRUE;
    }

    //--------------------------------------------------------------------------

    if(uMsg == WM_CLOSE)  // Received Close Message
    {
        EndDialog(hWnd,0);
        DestroyWindow(hWnd);
        return TRUE;
    }

    //--------------------------------------------------------------------------

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
