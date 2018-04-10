#ifndef _NEWPASSDLG_CPP
#define _NEWPASSDLG_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <ShlObj.H>
#include <Stdio.H>
#include <ShlOBJ.H>
#include <Stdlib.H>

#include "../Source/Package.H"
#include "../Source/Entry.H"
#include "../Source/Routines.H"
#include "../Source/FolderTree.H"
#include "../Source/HardKey.H"

#include "../Dialogs/MainDlg.H"
#include "../Dialogs/NewPassDlg.H"
#include "../Dialogs/OpenPassDlg.H"
#include "../Dialogs/NewFolderDlg.H"
#include "../Dialogs/NetLogoDlg.H"
#include "../Dialogs/NoSecurityDlg.H"
#include "../Dialogs/WeakPwdDialog.H"
#include "../Dialogs/WeakPwdNotificationDlg.H"
#include "../Dialogs/OverwriteOrAppend.H"
#include "../Dialogs/AdvancedOptDlg.H"

#include "../../../@Libraries/CNASCCL (Stream)/CNASCCL.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 
using namespace NSWFL::File; 
using namespace NSWFL::String; 
using namespace NSWFL::Conversion; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

WNDPROC gpOldSaveToLocationProc;
HWND hNewPassDialog = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK NewSaveToLocationProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if(uMsg == WM_GETDLGCODE)
	{
		return (DLGC_WANTALLKEYS | CallWindowProc(gpOldSaveToLocationProc, hWnd, uMsg, wParam, lParam));
	}
	else if(uMsg == WM_CHAR)
	{
		if(wParam == VK_RETURN)
		{
			return 0;
		}
		else if(wParam == VK_TAB)
		{
			return 0;
		}
	}
	else if(uMsg == WM_KEYDOWN || uMsg == WM_KILLFOCUS)
	{
		if((wParam == VK_RETURN || wParam == VK_TAB) || uMsg == WM_KILLFOCUS)
		{
		    int iLength = GetWindowTextLength(hWnd);
			if(iLength > 0)
			{
				char *sText = (char*) calloc(sizeof(char), iLength + 1);
				iLength = GetWindowText(hWnd, sText, iLength + 1);
				if(iLength > 2 && strncmp(sText, "\\\\", 2) == 0)
				{
					EnableWindow(GetDlgItem(hNewPassDialog, IDC_ENCRYPT), TRUE);
				}
				else {
					SendMessage(hNewPassDialog, WM_COMMAND, IDC_GOTO, 0);
				}
				free(sText);
			}
			else {
				SendMessage(hNewPassDialog, WM_COMMAND, IDC_GOTO, 0);
			}

			if(uMsg == WM_KEYDOWN)
			{
				if(wParam == VK_RETURN)
				{
					SetFocus(GetDlgItem(hNewPassDialog, IDC_SAVETOFILE));
					SendMessage(GetDlgItem(hNewPassDialog, IDC_SAVETOFILE), EM_SETSEL, 0, -1);
				}
				else if(wParam == VK_TAB) {
					PostMessage(hNewPassDialog, WM_NEXTDLGCTL, 0, 0L);
				}
			}
		}
	}

	return CallWindowProc(gpOldSaveToLocationProc, hWnd, uMsg, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK NewPassDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //--------------------------------------------------------------------------

    static FolderTree FolderTree;

    static HWND hBrowseTree = NULL;
    static HWND hPassword1 = NULL;
    static HWND hPassword2 = NULL;

    static HWND hPasswordLbl1 = NULL;
    static HWND hPasswordLbl2 = NULL;

	static HWND hPasswordBits = NULL;
    static HWND hPasswordBitsBar = NULL;
    static HWND hSaveToLocation = NULL;
    static HWND hSaveToFile = NULL;
    static HWND hCompressLevel = NULL;
    static HWND hEncryptAlgo = NULL;

	static char sHardKeyFile[MAX_PATH];

    //--------------------------------------------------------------------------

    if(uMsg == WM_INITDIALOG) // Received A Initilize Dialog Message
    {
		hNewPassDialog = hWnd;

		HMENU hSystemMenu = GetSystemMenu(hWnd, FALSE);
        AppendMenu(hSystemMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hSystemMenu, MF_STRING, WM_ABOUT_DIALOG, "About");

        SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)DISPLAYACRONYM"Encrypt");
        SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN)) );

        hPassword1 = GetDlgItem(hWnd, IDC_PASSWORD1);
        hPassword2 = GetDlgItem(hWnd, IDC_PASSWORD2);
		hPasswordLbl1 = GetDlgItem(hWnd, IDC_PASSWORDLBL1);
        hPasswordLbl2 = GetDlgItem(hWnd, IDC_PASSWORDLBL2);

        hPasswordBitsBar = GetDlgItem(hWnd, IDC_PASSWORDBAR2);
        hPasswordBits = GetDlgItem(hWnd, IDC_PASSWORDBITS2);
        hSaveToLocation = GetDlgItem(hWnd, IDC_SAVETOLOCATION);
        hSaveToFile = GetDlgItem(hWnd, IDC_SAVETOFILE);
        hCompressLevel = GetDlgItem(hWnd, IDC_COMPRESSLEVEL);
        hEncryptAlgo = GetDlgItem(hWnd, IDC_ENCRYPTALGO);
        hBrowseTree = GetDlgItem(hWnd, IDC_BROWSETREE);

		gpOldSaveToLocationProc = (WNDPROC)
			SetWindowLongPtr(hSaveToLocation, GWLP_WNDPROC, (LRESULT) &NewSaveToLocationProc);

		SendMessage(hPassword1, EM_LIMITTEXT, 0, 0);
		SendMessage(hPassword2, EM_LIMITTEXT, 0, 0);

		//giPkgAttribs = 0; //Don't reset the global encryption flags.
		//giPkgOptions = 0; //Don't Reset the global encryption flags.

		ResetDefaultAttributes();

        int iIndex = 0;

		//-----------------------------------------------------------------------------------------
        SendMessage(hCompressLevel, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"None");
        SendMessage(hCompressLevel, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"Very Low");
        SendMessage(hCompressLevel, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"Low");
        SendMessage(hCompressLevel, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"Moderate");
        SendMessage(hCompressLevel, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"High");
        SendMessage(hCompressLevel, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"Very High");
		SelectComboItem(hCompressLevel, gsDfltCompressLevel, 9);

		//-----------------------------------------------------------------------------------------
        SendMessage(hEncryptAlgo, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"AES");
        SendMessage(hEncryptAlgo, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"NASCCL");
        SendMessage(hEncryptAlgo, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"BlowFish");
		SelectComboItem(hEncryptAlgo, gsDfltEncryptAlgo, 3);

		//-----------------------------------------------------------------------------------------

        if(gPassword.iLength > 0)
		{
			Set_Text(hPassword1, gPassword.sPassword);
			Set_Text(hPassword2, gPassword.sPassword);
		}

		FolderTree.InitFolderTree(hBrowseTree, GetDlgItem(hWnd, IDC_ENCRYPT), hSaveToLocation, hSaveToFile);

		char FullPath[MAX_PATH + 1];
        char sDrive[MAX_PATH + 1];
        char sDir[MAX_PATH + 1];
        char sFile[MAX_PATH + 1];
        char sExt[MAX_PATH + 1];

		if(strlen(gsWorkingOutput) > 0)
		{
			strcpy_s(gsArchiveName, sizeof(gsArchiveName), gsWorkingOutput);
		}

		if(strlen(gsArchiveName) == 0)
		{
			if(Get_DesktopDirectory(gsArchiveName, sizeof(gsArchiveName)))
			{
				FolderTree.SelectPath(gsArchiveName);
			}

			strcat_s(gsArchiveName, sizeof(gsArchiveName), "\\New Archive" DEFAULTEXT);
		}

		if(IsDirectory(gsArchiveName))
        {
            int iLength = (int)strlen(gsArchiveName);

            if(gsArchiveName[iLength - 1] == '\\')
            {
                gsArchiveName[iLength] = '\0';
            }

            _splitpath_s(gsArchiveName,
				sDrive, sizeof(sDrive),
				sDir, sizeof(sDir),
				sFile, sizeof(sFile),
				sExt, sizeof(sExt));

			//Append the file extension to the file name.
			//	Just incase the directory has a period (.) in its name.
			strcat_s(sFile, sizeof(sFile), sExt);

			_makepath_s(FullPath, sizeof(FullPath), sDrive, sDir, NULL, NULL);
            FolderTree.SelectPath(FullPath);

			if(strlen(sFile) == 0) //No target file name?
			{
				strcpy_s(sFile, sizeof(sFile), "New Archive"); //Default a target file name.
			}

            _makepath_s(FullPath, sizeof(FullPath), NULL, NULL, sFile, DEFAULTEXT);
            Set_Text(hSaveToFile, FullPath);
        }
        else {
            // Parse File name and location
            _splitpath_s(gsArchiveName,
				sDrive, sizeof(sDrive),
				sDir, sizeof(sDir),
				sFile, sizeof(sFile),
				NULL, 0);

            _makepath_s(FullPath, sizeof(FullPath), sDrive, sDir, NULL, NULL);
			Set_Text(hSaveToLocation, FullPath);

			if(strncmp(FullPath, "\\\\", 2) == 0)
			{
				EnableWindow(GetDlgItem(hWnd, IDC_ENCRYPT), TRUE);
			}
			else{
				FolderTree.SelectPath(FullPath);
			}

			_makepath_s(FullPath, sizeof(FullPath), NULL, NULL, sFile, DEFAULTEXT);
            Set_Text(hSaveToFile, FullPath);
        }

        CenterOverOwner(hWnd);

		if(!IsWindowVisible(GetParent(hWnd)))
		{
			ShowWindowInTaskbar(hWnd, true);
		}

		return TRUE; // Return TRUE to set the keyboard focus, Otherwise return FALSE
    }

    //--------------------------------------------------------------------------

    else if(FolderTree.FolderTreeHandler(hWnd, uMsg, wParam, lParam))
    {
        return TRUE;
    }

    //--------------------------------------------------------------------------

    else if(uMsg == 307) // Key Down
    {
		float fPasswordLen = 0;
		float fEffeciencyBits = 0;

        char lsPassword[1024 + 1];
		char lsTemp0[64];
		char lsTemp1[64];

		HWND hFocus = GetFocus();

		if(hFocus == hPassword1 || hFocus == hPassword2)
		{
			fPasswordLen = (float) Get_TextLength(hFocus);
			if(fPasswordLen == 0)
			{
				Set_Text(hPasswordBits, "(0 Bits, No security)");
			}
			else{
				fEffeciencyBits = (fPasswordLen / 16.0f) * 100.0f;

				const char *sPasswordWeight;

				if(fPasswordLen >= sizeof(lsPassword))
				{
					sPasswordWeight = "";
				}
				else{
					CNASCCL Crypt;
					Get_Text(hFocus, lsPassword, sizeof(lsPassword));
					sPasswordWeight = Crypt.KeyWeightString(lsPassword, ((int)fPasswordLen));
				}

				sprintf_s(lsTemp0, sizeof(lsTemp0), " (%s Bits%s%s)",
					FormatInteger(lsTemp1, sizeof(lsTemp1), ((long)fPasswordLen * 8)),
					(sPasswordWeight == "" ? "" : ", "),
					sPasswordWeight);

				Set_Text(hPasswordBits, lsTemp0);
			}
		}
    }

    //--------------------------------------------------------------------------

    else if(uMsg == WM_COMMAND)
    {
		if(wParam == IDC_ADVANCEDOPT)
        {
			DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_ADVANCEDOPT), hWnd, AdvancedOptDlg);
		}
		else if(wParam == IDC_USEGENERATEDKEY)
        {
			bool bEnable = true;
			//char sTemp0[64];
			//char sTemp1[64];

			if((bEnable = (IsDlgButtonChecked(hWnd, IDC_USEGENERATEDKEY) == BST_CHECKED)))
			{
				char *Filters = "HardKey-Files\0*" DEFAULTHARDKEYEXT "\0All-Files\0*.*\0\0";
				char sInitialDirectory[MAX_PATH];

				GetLastPath(sInitialDirectory, sizeof(sInitialDirectory));

				memset(sHardKeyFile, 0, sizeof(sHardKeyFile));

				if(OSFileDialog(hWnd, sHardKeyFile, sizeof(sHardKeyFile), DEFAULTHARDKEYEXT,
					"Select a previously generated HardKey file.",
					Filters, sInitialDirectory, OFD, OFN_EXPLORER + OFN_LONGNAMES + OFN_PATHMUSTEXIST))
				{
					SetLastPath(sHardKeyFile);
					/*
					if((dwKeySz = Get_FileSize(sHardKeyFile)) <= 0)
					{
						SafeMsgBox(hWnd, "Failed to open the key file that you selected.", gsTitleCaption, MB_ICONINFORMATION);
						CheckDlgButton(hWnd, IDC_USEGENERATEDKEY, BST_UNCHECKED);
					}
					*/
				}
				else{
					CheckDlgButton(hWnd, IDC_USEGENERATEDKEY, BST_UNCHECKED);
				}
			}

			return TRUE;
		}
		else if(wParam == IDC_NEWFOLDER)
        {
            int iLength = Get_Text(hSaveToLocation, gsExtractLocation, 255);
			if(iLength > 0 || iLength <= 255)
			{
				if(DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_NEWFOLDER), hWnd, NewFolderDialog) == 1)
				{
					char sNewPath[MAX_PATH + 1];
					_makepath_s(sNewPath, sizeof(sNewPath), NULL, gsExtractLocation, sNewFolderName, NULL);
					strcat_s(sNewPath, sizeof(sNewPath), "\\");
					CustomCreateFolderStructure(sNewPath);

					//I know, I know... This is a bad way to refresh the treeview.
					FolderTree.UnInitFolderTree();
					FolderTree.InitFolderTree(hBrowseTree, GetDlgItem(hWnd, IDC_ENCRYPT), hSaveToLocation, hSaveToFile);
					FolderTree.SelectPath(sNewPath);
				}
			}
			return TRUE;
		}
		else if(wParam == IDC_GOTO)
        {
            int iLength = Get_Text(hSaveToLocation, gsExtractLocation, 255);
			if(iLength > 0 || iLength <= 255)
			{
				FolderTree.SelectPath(gsExtractLocation);
			}
			return TRUE;
		}
		else if(wParam == IDC_ENCRYPT || wParam == IDOK)
        {
            char sSaveToLocation[MAX_PATH + 1];
            int iSaveToLocation = Get_Text(hSaveToLocation, sSaveToLocation, sizeof(sSaveToLocation));
            if(iSaveToLocation <= 0 || iSaveToLocation >= sizeof(sSaveToLocation))
			{
				MessageBox(hWnd, "You must specify a Location to save the encrypted files to.",
					gsTitleCaption, MB_ICONINFORMATION);
				return TRUE;
			}

			strcat_s(sSaveToLocation, sizeof(sSaveToLocation), "\\");

            char sSaveToFile[MAX_PATH + 1];
            int iSaveToFile = Get_Text(hSaveToFile, sSaveToFile, sizeof(sSaveToFile));
			if(iSaveToFile <= 0 || iSaveToFile >= sizeof(sSaveToFile))
			{
				MessageBox(hWnd, "You must specify a File Name to save the encrypted files to.",
					gsTitleCaption, MB_ICONINFORMATION);
				return TRUE;
			}

			//Add the default file extension if its not already present.
			if(_strcmpi(sSaveToFile + (iSaveToFile - strlen(DEFAULTEXT)), DEFAULTEXT) != 0)
			{
				strcat_s(sSaveToFile, sizeof(sSaveToFile), DEFAULTEXT);
			}

			strcat_s(sSaveToLocation, sizeof(sSaveToLocation), sSaveToFile);
			CorrectReversePath(sSaveToLocation, sizeof(sSaveToLocation), true);

			if(DoesPathContainUnicode(sSaveToLocation))
			{
				SetPassword(&gPassword, NULL, 0); //Free RAM.

				SafeMsgBox(hWnd,
					"The name of the file you are trying to create contains\r\n"
					"UNICODE characters, which are not supported.",
					gsTitleCaption, MB_ICONASTERISK);
				return FALSE;
			}

			int iLength1 = Get_TextLength(hPassword1);
            int iLength2 = Get_TextLength(hPassword2);
			if((iLength1 + iLength2) > 0)
            {
				if(!ComparePasswordTextBoxes(hPassword1, iLength1, hPassword2, iLength2, &giKeyWeight))
				{
					SafeMsgBox(hWnd,
						"The passwords you entered do not match.", gsTitleCaption, MB_ICONINFORMATION);
					return TRUE;
				}

				if(((DWORD)giKeyWeight) < gdwWeakEncryptionWarnLevel)
				{
					INT_PTR iRetVal = DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_WEAKPASSWORD), hWnd, WeakPwdDialog);
					if(iRetVal == IDC_CMD_YES)
					{
						//Just continue...
					}
					else if(iRetVal == IDC_CMD_NO){
						return TRUE;
					}
					else if(iRetVal == IDC_CMD_CHANGE){
						DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_WEAKPWDNOTIFY), hWnd, WeakPwdNotificationDialog);
						return TRUE;
					}
				}

				if(IsDlgButtonChecked(hWnd, IDC_USEGENERATEDKEY) == BST_CHECKED)
				{
					PASSWORD Pwd;
					memset(&Pwd, 0, sizeof(Pwd));

					SetPassword(&Pwd, hPassword1); //Get the password from the Textbox, or free RAM if empty.

					SetPassword(&gPassword, NULL, 0); //Free RAM.
					bool bResult = ReadHardKey(hWnd, sHardKeyFile, Pwd.sPassword, Pwd.iLength, &gPassword);
					SetPassword(&Pwd, NULL, 0); //Free RAM.
					if(!bResult)
					{
						//All messages are handled internally by the ReadHardKey function.
						return TRUE;
					}
				}
				else{
					SetPassword(&gPassword, hPassword1); //Get the password from the Textbox, or free RAM if empty.
				}			

				giPkgAttribs &= ~ATTRIB_NOPASSWORD;
			}
			else{
				if(IsDlgButtonChecked(hWnd, IDC_USEGENERATEDKEY) == BST_CHECKED)
				{
					SafeMsgBox(hWnd,
						"You must specify the password for the selected HardKey.", gsTitleCaption, MB_ICONINFORMATION);
					return FALSE;
				}

				if(gbWarnAboutNoEncryption)
				{
					//Warn user about not using encryption.
					if(!DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_NOSECURITYWARN), hWnd, NoSecurityDialog))
					{
						return FALSE;
					}
				}
				SetPassword(&gPassword, gsEmptyPassword);

				giPkgAttribs |= ATTRIB_NOPASSWORD;
			}

			giPkgOptions &= ~OPTION_USEALTFILEPACK;

			if(FileAccess(sSaveToLocation, FExist))
			{
				if(!gbOverwrite)
				{
					INT_PTR iResult = DialogBox(ghAppInstance, MAKEINTRESOURCE(IDD_OVERWRITEORAPPEND), hWnd, OverwriteOrAppendDialog);
					if(iResult == RESULT_OVERWRITE)
					{
						giPkgOptions |= OPTION_OVERWRITE;
						giPkgOptions &= ~OPTION_ADDFILES;
					}
					else if(iResult == RESULT_APPEND)
					{
						giPkgOptions |= OPTION_ADDFILES;
						giPkgOptions &= ~OPTION_OVERWRITE;
					}
					else{
						SetPassword(&gPassword, NULL, 0); //Free RAM.
						return FALSE;
					}
				}
			}

            strcpy_s(gsWorkingOutput, sizeof(gsWorkingOutput), sSaveToLocation);

			//-----------------------------------------------------------------------------------------
            Get_Text(hCompressLevel, gsDfltCompressLevel, sizeof(gsDfltCompressLevel));
            if(_strcmpi(gsDfltCompressLevel, "None") != 0)
            {
                giPkgAttribs |= ATTRIB_COMPRESSION;

                if(_strcmpi(gsDfltCompressLevel, "Very Low") == 0)
                    giCompressLevel = 1;
                else if(_strcmpi(gsDfltCompressLevel, "Low") == 0)
                    giCompressLevel = 3;
                else if(_strcmpi(gsDfltCompressLevel, "Moderate") == 0)
                    giCompressLevel = 5;
                else if(_strcmpi(gsDfltCompressLevel, "High") == 0)
                    giCompressLevel = 7;
                else if(_strcmpi(gsDfltCompressLevel, "Very High") == 0)
                    giCompressLevel = 9;
            }
			else{
				giCompressLevel = 0;
                giPkgAttribs &= ~ATTRIB_COMPRESSION;
			}

			//-----------------------------------------------------------------------------------------
            Get_Text(hEncryptAlgo, gsDfltEncryptAlgo, sizeof(gsDfltEncryptAlgo));
			if(_strcmpi(gsDfltEncryptAlgo, "None") != 0)
            {
				giCryptAlgo &= ~CRYPT_ALGO_NASCCL;
				giCryptAlgo &= ~CRYPT_ALGO_RIJNDAEL;
				giCryptAlgo &= ~CRYPT_ALGO_BLOWFISH;

                if(_strcmpi(gsDfltEncryptAlgo, "NASCCL") == 0)
				{
                    giCryptAlgo |= CRYPT_ALGO_NASCCL;
				}
				else if(_strcmpi(gsDfltEncryptAlgo, "AES Algorithm (Rijndael)") == 0){
                    giCryptAlgo |= CRYPT_ALGO_RIJNDAEL;
				}
				else if(_strcmpi(gsDfltEncryptAlgo, "BlowFish") == 0){
                    giCryptAlgo |= CRYPT_ALGO_BLOWFISH;
				}
            }

			//-----------------------------------------------------------------------------------------

			FolderTree.UnInitFolderTree();
            EndDialog(hWnd, 1);

			return TRUE;
        }
        else if(wParam == IDC_CANCEL || wParam == IDCANCEL)
        {
            FolderTree.UnInitFolderTree();
        	EndDialog(hWnd, 0);
            return TRUE;
        }
        else if(wParam == IDC_HELPBUTTON)
        {
			OpenHelpFile("Encrypting/WalkThrough.html");
            return TRUE;
        }
		
		return FALSE;
    }

    //--------------------------------------------------------------------------

	else if(uMsg == WM_PAINT)
    {
        HDC ThisHDC;
        PAINTSTRUCT ThisPS;

        ThisHDC = BeginPaint(hWnd, &ThisPS);

        // Any painting should be done here

        EndPaint(hWnd, &ThisPS);
        return TRUE;
    }

    //--------------------------------------------------------------------------

    else if(uMsg == WM_SYSCOMMAND) // Received A System Menu Message
    {
        if(LOWORD(wParam) == WM_ABOUT_DIALOG) //- About (SysMenu Command)
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

    else if(uMsg == WM_CLOSE)  // Received Close Message
    {
        EndDialog(hWnd, 0);
        FolderTree.UnInitFolderTree();
        return TRUE;
    }

    //--------------------------------------------------------------------------

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

