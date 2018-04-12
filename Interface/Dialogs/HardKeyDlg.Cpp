#ifndef _HardKeyDlg_CPP
#define _HardKeyDlg_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <ShlObj.H>
#include <Stdio.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "../Source/Entry.H"
#include "../Source/Routines.H"
#include "../Source/HardKey.H"

#include "../Dialogs/HardKeyDlg.H"
#include "../Dialogs/NetLogoDlg.H"
#include "../Dialogs/HardKeyStatDlg.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 
using namespace NSWFL::String; 
using namespace NSWFL::File; 
using namespace NSWFL::Conversion; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define HARDKEYTOPWDRATIO 10240

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HANDLE ghThread = NULL;
HWND hHardKeyDlg = NULL;

char *gsHardKeyPwd = NULL;
int giHardKeyGenLen = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI WorkerThread(LPVOID lpVoid)
{
	Sleep(1000);

	WaitOnWindow(hHardKeyStatDialog);

	int iResult = CreateKeyFile(hHardKeyStatusText, hHardKeyStatusBar, (char *)lpVoid,
		gsHardKeyPwd, (int)strlen(gsHardKeyPwd), giHardKeyGenLen / 8);
	if(!iResult)
	{
		DeleteFile((char *)lpVoid);
	}

	EndDialog(hHardKeyStatDialog, iResult);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK HardKeyDlg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //--------------------------------------------------------------------------

    static HWND hPassword1 = NULL;
    static HWND hPassword2 = NULL;
    static HWND hPasswordBits = NULL;
    static HWND hKeyGenSlider = NULL;
    static HWND hKeyGenSliderText = NULL;

	static int iLastPWLength = 0;

    //--------------------------------------------------------------------------

    if(uMsg == WM_INITDIALOG) // Received A Initilize Dialog Message
    {
		hHardKeyDlg = hWnd;

        HMENU hSysMenu_hMenu = GetSystemMenu(hWnd, FALSE);
        AppendMenu(hSysMenu_hMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hSysMenu_hMenu, MF_STRING, WM_ABOUT_DIALOG, "About");

        SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"HardKey Generator");
        SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN)));

        hPassword1 = GetDlgItem(hWnd, IDC_PASSWORD1);
        hPassword2 = GetDlgItem(hWnd, IDC_PASSWORD2);
		hPasswordBits = GetDlgItem(hWnd, IDC_PASSWORDBITS2);
		hKeyGenSlider = GetDlgItem(hWnd, IDC_KEYGENSZ);
		hKeyGenSliderText = GetDlgItem(hWnd, IDC_CERTPWDBITS);

		SendMessage(hPassword1, EM_LIMITTEXT, 1024*10, 0);
		SendMessage(hPassword2, EM_LIMITTEXT, 1024*10, 0);

		SendMessage(hKeyGenSlider, TBM_SETRANGEMIN, TRUE, 0);
		SendMessage(hKeyGenSlider, TBM_SETRANGEMAX, TRUE, 0);
		SendMessage(hWnd, 276, 0, 0);

		CenterOverOwner(hWnd);

        return TRUE; // Return TRUE to set the keyboard focus, Otherwise return FALSE.
    }

	if(uMsg == 276)
	{
		int iPDWDLen = (int)SendMessage(hKeyGenSlider, TBM_GETPOS, 0, 0);

		char sTemp[64];
		char sMaxBits[64];
		char sCurrentBits[64];

		FormatInteger(sCurrentBits, sizeof(sCurrentBits), (int)SendMessage(hKeyGenSlider, TBM_GETPOS, 0, 0));
		FormatInteger(sMaxBits, sizeof(sMaxBits), iLastPWLength * HARDKEYTOPWDRATIO);

		sprintf_s(sTemp, sizeof(sTemp), " (%s of %s Bits)", sCurrentBits, sMaxBits);
		Set_Text(hKeyGenSliderText, sTemp);
	}

	//--------------------------------------------------------------------------

    if(uMsg == 307) // Key Down
    {
		HWND hCurrentFocus = GetFocus();

		if(hCurrentFocus == hPassword1 || hCurrentFocus == hPassword2)
		{
			char lsTemp0[64];
			char lsTemp1[64];
			char sMaxBits[64];
			char sCurrentBits[64];

			iLastPWLength = (int)SendMessage(hCurrentFocus, WM_GETTEXTLENGTH, 0, 0);

			sprintf_s(lsTemp0, sizeof(lsTemp0), " (%s Bits)",
				FormatInteger(lsTemp1, sizeof(lsTemp1), (iLastPWLength * 8)));
			Set_Text(hPasswordBits, lsTemp0);

			int iMaxKeySz = iLastPWLength * HARDKEYTOPWDRATIO;
			int iPos = (int)SendMessage(hKeyGenSlider, TBM_GETPOS, 0, 0);

			SendMessage(hKeyGenSlider, TBM_SETRANGEMAX, TRUE, iMaxKeySz);

			if(iPos > iMaxKeySz || iPos == (iMaxKeySz - HARDKEYTOPWDRATIO) || iPos == 0)
			{
				iPos = iMaxKeySz;
				SendMessage(hKeyGenSlider, TBM_SETPOS, TRUE, iMaxKeySz);
			}

			FormatInteger(sCurrentBits, sizeof(sCurrentBits), iPos);
			FormatInteger(sMaxBits, sizeof(sMaxBits), iMaxKeySz);
			sprintf_s(lsTemp0, sizeof(lsTemp0), " (%s of %s Bits)", sCurrentBits, sMaxBits);
			Set_Text(hKeyGenSliderText, lsTemp0);
		}
    }

	//--------------------------------------------------------------------------

    if(uMsg == WM_COMMAND)
    {
        if(wParam == IDC_OKBUTTON || wParam == IDOK)
        {
            int iLength1 = Get_TextLength(hPassword1);
            int iLength2 = Get_TextLength(hPassword2);

			giHardKeyGenLen = (int)SendMessage(hKeyGenSlider, TBM_GETPOS, 0, 0);

			if(iLength1 <= 0 || iLength2 <= 0)
			{
				SafeMsgBox(hWnd, "You must enter a HardKey password.", gsTitleCaption, MB_ICONINFORMATION);
				return TRUE;
			}

			int giKeyWeight = 0;

			if(!ComparePasswordTextBoxes(hPassword1, iLength1, hPassword2, iLength2, &giKeyWeight))
			{
				SafeMsgBox(hWnd, "The passwords you entered do not match.", gsTitleCaption, MB_ICONINFORMATION);
				return TRUE;
			}

			if(giHardKeyGenLen <= 0)
			{
				int iResult = SafeMsgBox(hWnd,
						"You have not set a HardKey size.\r\nWould you like to use the maximum available?",
						gsTitleCaption, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);

				if(iResult != IDYES)
				{
					return TRUE;
				}

				giHardKeyGenLen = (iLength2 * HARDKEYTOPWDRATIO);
			}

			gsHardKeyPwd = (char *) calloc(sizeof(char), (giHardKeyGenLen + 1));
			Get_Text(hPassword1, gsHardKeyPwd, giHardKeyGenLen + 1);

			char sFileName[MAX_PATH];
			char sDlftDirectory[MAX_PATH];

			strcpy_s(sFileName, sizeof(sFileName), "New HardKey");
			Get_DesktopDirectory(sDlftDirectory, sizeof(sDlftDirectory));

			if(!OSFileDialog(hWnd, sFileName, sizeof(sFileName), DEFAULTHARDKEYEXT,
				"Save HardKey as", "HardKey File\0*" DEFAULTHARDKEYEXT "\0All-Files\0*.*\0\0", sDlftDirectory, SFD, 0))
			{
				return FALSE;
			}

			if(FileAccess(sFileName, FExist))
			{
				if(SafeMsgBox(hWnd, "The file name you specified already exits. Overwrite it?",
					gsTitleCaption, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) != IDYES)
				{
					free(gsHardKeyPwd);
					return TRUE;
				}
			}

			//Check for free space
			char sDrive[MAX_PATH];
			_splitpath_s(sFileName, sDrive, sizeof(sDrive), NULL, 0, NULL, 0, NULL, 0);

			ULARGE_INTEGER uiFreeBytesAvailableToCaller;
			ULARGE_INTEGER uiTotalNumberOfBytes;
			ULARGE_INTEGER uiTotalNumberOfFreeBytes;

			if(!GetDiskFreeSpaceEx(sDrive, &uiFreeBytesAvailableToCaller, &uiTotalNumberOfBytes, &uiTotalNumberOfFreeBytes))
			{
				SafeMsgBox(hWnd, "Failed to get free disk space information.", gsTitleCaption, MB_ICONERROR);
				free(gsHardKeyPwd);
				return TRUE;
			}

			unsigned __int64 i64RequiredSpace = giHardKeyGenLen + (100 * 1024);

			if(uiFreeBytesAvailableToCaller.QuadPart < i64RequiredSpace)
			{
				SafeMsgBox(hWnd, "There is not enough free space on the disk you specified.\r\n"
					"Reduce the key size, free some disk space or choose another location.", gsTitleCaption, MB_ICONINFORMATION);
				free(gsHardKeyPwd);
				return TRUE;
			}

			if(ghThread != NULL)
			{
				CloseHandle(ghThread);
			}

			ghThread = CreateThread(NULL, 0, WorkerThread, (LPVOID)sFileName, 0, NULL);

			if(DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_HARDKEYSTATUS), hWnd, HardKeyStatDialog) == 1)
			{
				SafeMsgBox(hHardKeyDlg, "Operation Complete!", gsTitleCaption, MB_ICONINFORMATION);
				EndDialog(hWnd, 0);
			}

			free(gsHardKeyPwd);

			return TRUE;
        }

        else if(wParam == IDC_CANCEL || wParam == IDCANCEL)
        {
            EndDialog(hWnd, 0);
            DestroyWindow(hWnd);

            return TRUE;
        }

        else if(wParam == IDC_HELPBUTTON)
        {
			OpenHelpFile("/HardKey/Overview.Html");
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

        //- Any painting should be done here.

        EndPaint(hWnd, &ThisPS);
        return TRUE;
    }

    //--------------------------------------------------------------------------

    if(uMsg == WM_SYSCOMMAND) //- Received a system menu message.
    {
        if(LOWORD(wParam) == WM_ABOUT_DIALOG) //- About.
        {
            _AboutDialogInfo ADI;
    
            ADI.DisplayIcon  = LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN));
            ADI.TitleCaption = gsTitleCaption;
            ADI.FileVersion  = gsFileVersion;
            ADI.BuildDate    = __DATE__;
            ADI.BuildTime    = __TIME__;
            ADI.CopyRight    = gsAppCopyRight;
            ADI.OwnerHandle  = hWnd;

            NetLogo(&ADI);
            return TRUE;
        }

        return FALSE;
    }

    //--------------------------------------------------------------------------

    if(uMsg == WM_CLOSE) //- Received close message.
    {
        EndDialog(hWnd,0);
        DestroyWindow(hWnd);

        return TRUE;
    }

    //--------------------------------------------------------------------------

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

