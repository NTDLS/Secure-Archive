#ifndef _LISTVIEW_H_
#define _LISTVIEW_H_
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define SORT_DIR_ASCENDING  1
#define SORT_DIR_DESCENDING 2

#define COL_STRING		1
#define COL_INTEGER		2
#define COL_DOUBLE		3
#define COL_DATE		4

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern WNDPROC gpOldListProc;
extern HWND ghFileList;
extern HMENU ghMenu;
extern HWND hHorizBar;
extern HWND hStatusBar;

extern HWND hTB_New[2];
extern HWND hTB_Open[2];
extern HWND hTB_AddFiles[2];
extern HWND hTB_Delete[2];
extern HWND hTB_Extract[2];
//extern HWND hTB_View[2];
extern HWND hTB_Info[2];
extern HWND hTB_Config[2];
extern HWND hTB_Find[2];
extern HWND hTB_Exit[2];

extern HIMAGELIST hImageList;
extern HIMAGELIST hImageListBig;

extern int giSortCol;
extern int giSortDirection;

extern const char *gsColumnNames[];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool Float(HWND hWnd);
inline bool Sink(HWND hWnd);
DWORD WINAPI MouseOverThread(LPVOID lpData);
HWND InitListView(HWND hDlg);
int CALLBACK CompList(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3);
int CALLBACK CompListEx(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3);
LRESULT CALLBACK NewListProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void AlignText(HWND hBut, HWND hTxt);
void CheckGridRows(HWND hGrid, bool bCheck);
inline void OnMouseOver(HWND hWnd);
void SelectedAllOfSelectedPaths(HWND hGrid);
void EnableButton(HWND *hButton, bool bEnable);
void UpdateButtons(void);
void RecalcSelectedFiles(void);
bool SortFileList(HWND hList, int iSortColumn, int iSortDirection);
bool SortFileList(HWND hList, const char *sColumnName, int iSortDirection);
int ColumnNameToPosition(const char *sColumnName);
void AutoSizeGridCols(void);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
