#ifndef _DIALOGS_TABPROCESSORS_CPP
#define _DIALOGS_TABPROCESSORS_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <WindowsX.H>
#include <Windows.H>
#include <Stdio.H>
#include <Stdlib.H>
#include <ShlOBJ.H>
#include <CommCtrl.h>

#include "../Source/Entry.H"
#include "../Source/Routines.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 
using namespace NSWFL::Registry; 
using namespace NSWFL::String; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK TabProcessorsDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND hCPUs = NULL;

	if(uMsg == WM_INITDIALOG)
    {
		char sRegBuf[255];
		char sTemp[255];

		DWORD dwRegBufSz = 0;
		float fCPUSpeed = 0;

		LV_COLUMN lvColumn;
		LV_ITEM lvItem;
		HIMAGELIST hImageList;

		memset(&lvColumn, 0, sizeof(lvColumn));
		memset(&lvItem, 0, sizeof(lvItem));
		memset(&hImageList, 0, sizeof(hImageList));

		hCPUs = GetDlgItem(hWnd, IDC_CPUS);

		if(gbUseProcessBoost) CheckDlgButton(hWnd, IDC_PROCESSBOOST, BST_CHECKED);
		if(gbUseThreadBoost) CheckDlgButton(hWnd, IDC_THREADBOOST, BST_CHECKED);

		SYSTEM_INFO SI;
        GetSystemInfo(&SI);
		
		int iLVStyle = LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES;
        SendMessage(hCPUs, (UINT)LVM_SETEXTENDEDLISTVIEWSTYLE, (WPARAM)0, (LPARAM)iLVStyle);

        // Create ImageList begin
        hImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_MASK|ILC_COLOR32, 1, 1);

        HICON hIcon = NULL;

        hIcon = LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_SMCPU));
        ImageList_AddIcon(hImageList, (HICON)hIcon);
        DeleteObject((HICON)hIcon);

        ListView_SetImageList(hCPUs, hImageList, LVSIL_SMALL);

        // Create Listview columns
        lvColumn.cx        = 180;
        lvColumn.pszText   = "CPU";
        lvColumn.iSubItem  = 0;
        lvColumn.mask      = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
        lvColumn.fmt       = LVCFMT_LEFT;
        ListView_InsertColumn(hCPUs, 0, &lvColumn);

        lvColumn.cx        = 45;
        lvColumn.pszText   = "Model";
        lvColumn.iSubItem  = 1;
        lvColumn.mask      = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
        lvColumn.fmt       = LVCFMT_CENTER;
        ListView_InsertColumn(hCPUs, 1, &lvColumn);

        lvColumn.cx        = 55;
        lvColumn.pszText   = "Rev.";
        lvColumn.iSubItem  = 2;
        lvColumn.mask      = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
        lvColumn.fmt       = LVCFMT_CENTER;
        ListView_InsertColumn(hCPUs, 2, &lvColumn);

        lvColumn.cx        = 65;
        lvColumn.pszText   = "Granularity";
        lvColumn.iSubItem  = 3;
        lvColumn.mask      = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
        lvColumn.fmt       = LVCFMT_CENTER;
        ListView_InsertColumn(hCPUs, 3, &lvColumn);

		dwRegBufSz = sizeof(sRegBuf);
		memset(sRegBuf, 0, dwRegBufSz);
		if(Get_StringRegistryValue(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "ProcessorNameString", sRegBuf, dwRegBufSz))
		{
			Trim(sRegBuf);
			Set_Text(GetDlgItem(hWnd, IDC_CPUDESCR), sRegBuf);
		}

		DWORD dwItem = 0;
		while(dwItem < SI.dwNumberOfProcessors)
        {
			char sKey[1024];
			sprintf_s(sKey, sizeof(sKey), "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\%d", dwItem);

			if(!Get_DWORDRegistryValue(HKEY_LOCAL_MACHINE, sKey, "~MHz", dwRegBufSz))
			{
				dwRegBufSz = 0;
			}
			fCPUSpeed = (float)dwRegBufSz;

			dwRegBufSz = sizeof(sRegBuf);
			memset(sRegBuf, 0, dwRegBufSz);
			if(!Get_StringRegistryValue(HKEY_LOCAL_MACHINE, sKey, "VendorIdentifier", sRegBuf, dwRegBufSz))
			{
				strcpy_s(sRegBuf, sizeof(sRegBuf), "");
			}

			Trim(sRegBuf);

			if(fCPUSpeed > 1000)
			{
				sprintf_s(sTemp, sizeof(sTemp), "%s %.1f0 GHz", sRegBuf, fCPUSpeed / 1000.0f);
			}
			else {
				if(fCPUSpeed > 0)
				{
					sprintf_s(sTemp, sizeof(sTemp), "%s %.2f MHz", sRegBuf, fCPUSpeed);
				}
				else{
					strcpy_s(sTemp, sizeof(sTemp), "");
				}
			}
			
			lvItem.pszText = sTemp;
            lvItem.mask = LVIF_TEXT|LVIF_IMAGE;
            lvItem.iImage = 0;
            lvItem.iItem  = dwItem;
            lvItem.iSubItem = 0;
            ListView_InsertItem(hCPUs, &lvItem);

			sprintf_s(sTemp, sizeof(sTemp), "%d", HIWORD(SI.wProcessorRevision));
			lvItem.pszText = sTemp;
            lvItem.mask = LVIF_TEXT|LVIF_IMAGE;
            lvItem.iImage = 0;
            lvItem.iItem  = dwItem;
            lvItem.iSubItem = 1;
            ListView_SetItem(hCPUs, &lvItem);

			sprintf_s(sTemp, sizeof(sTemp), "%d", LOWORD(SI.wProcessorRevision));
            lvItem.pszText = sTemp;
            lvItem.mask = LVIF_TEXT|LVIF_IMAGE;
            lvItem.iImage = 0;
            lvItem.iItem  = dwItem;
            lvItem.iSubItem = 2;
            ListView_SetItem(hCPUs, &lvItem);

			sprintf_s(sTemp, sizeof(sTemp), "%0.2f K", ((float)SI.dwAllocationGranularity / 1024.0f));
            lvItem.pszText = sTemp;
            lvItem.mask = LVIF_TEXT|LVIF_IMAGE;
            lvItem.iImage = 0;
            lvItem.iItem  = dwItem;
            lvItem.iSubItem = 3;
            ListView_SetItem(hCPUs, &lvItem);

			if(IsBitSet(gdwCPUAffinityMask, dwItem))
			{
				ListView_SetCheckState(hCPUs, dwItem, TRUE);
			}
			else {
				ListView_SetCheckState(hCPUs, dwItem, FALSE);
			}

            dwItem++;
        }
		return TRUE;
    }
    else if(uMsg == WM_COMMAND)
    {
        if(wParam == IDC_HELPBUTTON)
        {
			OpenHelpFile("Configuration/Processor Settings.Html");
            return TRUE;
        }
	}
	else if(uMsg == WM_NOTIFY)
    {
        LPNMHDR pNMH = (LPNMHDR)lParam;
        if(pNMH->code == PSN_KILLACTIVE)
        {
            return FALSE;
        }
        if(pNMH->code == PSN_APPLY) // Ok
        {
			DWORD_PTR dwCPUAffinityMask = 0;
			DWORD iItems = ListView_GetItemCount(hCPUs);
			DWORD dwItem = 0;

			while(dwItem < iItems)
			{
				if(ListView_GetCheckState(hCPUs, dwItem))
				{
					SetBit(dwCPUAffinityMask, dwItem);
				}

				dwItem++;
			}

			if(dwCPUAffinityMask == 0)
			{
                SafeMsgBox(hWnd, "You must select at least one processor.", gsTitleCaption, MB_ICONASTERISK);
                SetWindowLongPtr(hWnd, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE); // Prevent window from closing
                return TRUE;
			}

			gbUseProcessBoost = (IsDlgButtonChecked(hWnd, IDC_PROCESSBOOST) == BST_CHECKED);
			gbUseThreadBoost = (IsDlgButtonChecked(hWnd, IDC_THREADBOOST) == BST_CHECKED);

			//Save the mask.
			gdwCPUAffinityMask = dwCPUAffinityMask;

			//Apply settings.
			SetProcessAffinityMask(GetCurrentProcess(), gdwCPUAffinityMask);
			SetProcessPriorityBoost(GetCurrentProcess(), !gbUseProcessBoost);

			return TRUE;
        }
        if(pNMH->code == PSN_RESET) // Cancel
        {
            return FALSE;
        }
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
