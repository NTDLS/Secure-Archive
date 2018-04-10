#ifndef _StatusDlg_H
#define _StatusDlg_H
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CStatusDlg{
	public:
		HWND Load(HWND hOwner, char *sTitle, int iIcon, bool bAllowCancel);
		bool Unload(void);
		bool IsDlg(void);
		bool IsDlgVisible(void);
		bool SetIcon(int iIcon);
		bool SetCaption(char *sCaption);
		bool Show(void);
		bool Hide(void);
		bool SetText(char *sText);
		bool EnableButton(void);
		bool DisableButton(void);
		bool SetButtonText(char *sText);
		bool SetProgressPos(long lPos);
		bool SetProgressRange(int iMin, int iMax);

		CStatusDlg();
		~CStatusDlg();

	private:
		friend DWORD WINAPI CStatusDlg_Thread(LPVOID lpData);
		HANDLE CStatusDlg_Thread_Handle;
		DWORD CStatusDlg_Thread_ID;

		HWND hDialogHandle;
		HWND hOwnerHandle;
		HWND hText;
		HWND hProgress;
		HWND hCmdCancel;

		HANDLE hEvent;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK CStatusDlgProc(HWND xHandle, UINT xMessage, WPARAM wParam, LPARAM lParam);
DWORD WINAPI CStatusDlg_Thread(LPVOID lpData);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

