#ifndef _NewFolderDlg_H
#define _NewFolderDlg_H
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern HWND hNewFolderDialog;
extern char sNewFolderName[MAX_PATH + 1];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK NewFolderDialog(HWND, UINT, WPARAM, LPARAM);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

