///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright © NetworkDLS 2002, All rights reserved
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _CTRAYICON_H
#define _CTRAYICON_H
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define WM_SHELLNOTIFY			WM_USER + 545
#define WM_CTRAYICON_INITIALIZE WM_USER + 656

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CTrayIcon{

	typedef void (* UserCallback)(CTrayIcon *lpTI, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	public:
		HINSTANCE hAppInstance;
		UserCallback pUserCallback;

		LPVOID UserData;

		NOTIFYICONDATA OriginalTrayNotify;

		unsigned int WM_TASKBARCREATED;

		HWND hTrayHandle;
		HMENU hPopupMenu;

		bool Destroy(void);
		#ifdef UNICODE
		bool Create(int iIcon, const WCHAR *sTitle, HMENU hInPopupMenu, UserCallback lpUserCallback);
		bool Update(int iIcon, const WCHAR *sTitle);
		bool Update(const WCHAR *sTitle);
		#else
		bool Create(int iIcon, const char *sTitle, HMENU hInPopupMenu, UserCallback lpUserCallback);
		bool Update(int iIcon, const char *sTitle);
		bool Update(const char *sTitle);
		#endif

		bool Update(int iIcon);
		bool DoesIconExist(void);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
