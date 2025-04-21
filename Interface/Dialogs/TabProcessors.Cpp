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

void PopulateProcessors(HWND hListView);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK TabProcessorsDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND hCPUs = NULL;

	if (uMsg == WM_INITDIALOG)
	{
		char sRegBuf[255];
		char sTemp[255];

		DWORD dwRegBufSz = 0;

		LV_COLUMN lvColumn;
		LV_ITEM lvItem;
		HIMAGELIST hImageList;

		memset(&lvColumn, 0, sizeof(lvColumn));
		memset(&lvItem, 0, sizeof(lvItem));
		memset(&hImageList, 0, sizeof(hImageList));

		hCPUs = GetDlgItem(hWnd, IDC_CPUS);

		if (gbUseProcessBoost) CheckDlgButton(hWnd, IDC_PROCESSBOOST, BST_CHECKED);
		if (gbUseThreadBoost) CheckDlgButton(hWnd, IDC_THREADBOOST, BST_CHECKED);

		int iLVStyle = LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES;
		SendMessage(hCPUs, (UINT)LVM_SETEXTENDEDLISTVIEWSTYLE, (WPARAM)0, (LPARAM)iLVStyle);

		// Create ImageList begin
		hImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_MASK | ILC_COLOR32, 1, 1);

		HICON hIcon = NULL;

		hIcon = LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_SMCPU));
		ImageList_AddIcon(hImageList, (HICON)hIcon);
		hIcon = LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_EFFICIENCYCORE));
		ImageList_AddIcon(hImageList, (HICON)hIcon);
		DeleteObject((HICON)hIcon);

		ListView_SetImageList(hCPUs, hImageList, LVSIL_SMALL);

		// Create Listview columns
		lvColumn.cx = 150;
		lvColumn.pszText = "CPU";
		lvColumn.iSubItem = 0;
		lvColumn.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
		lvColumn.fmt = LVCFMT_LEFT;
		ListView_InsertColumn(hCPUs, 0, &lvColumn);

		lvColumn.cx = 150;
		lvColumn.pszText = "Type";
		lvColumn.iSubItem = 1;
		lvColumn.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
		lvColumn.fmt = LVCFMT_LEFT;
		ListView_InsertColumn(hCPUs, 2, &lvColumn);

		PopulateProcessors(hCPUs);

		SYSTEM_INFO SI;
		GetSystemInfo(&SI);
		float fMaxReportedCPUSpeed = 0;

		for (DWORD dwIndex = 0; dwIndex < SI.dwNumberOfProcessors; dwIndex++)
		{
			char sKey[1024];
			sprintf_s(sKey, sizeof(sKey), "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\%d", dwIndex);
			if (!Get_DWORDRegistryValue(HKEY_LOCAL_MACHINE, sKey, "~MHz", dwRegBufSz))
			{
				dwRegBufSz = 0;
			}
			if (dwRegBufSz > fMaxReportedCPUSpeed)
			{
				fMaxReportedCPUSpeed = (float)dwRegBufSz;
			}

			dwIndex++;
		}

		dwRegBufSz = sizeof(sRegBuf);
		memset(sRegBuf, 0, dwRegBufSz);
		if (Get_StringRegistryValue(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "ProcessorNameString", sRegBuf, dwRegBufSz))
		{
			Trim(sRegBuf);

			if (fMaxReportedCPUSpeed > 0)
			{
				if (fMaxReportedCPUSpeed > 1000)
				{
					sprintf_s(sTemp, sizeof(sTemp), " (%.1f0 GHz)", fMaxReportedCPUSpeed / 1000.0f);
					strcat_s(sRegBuf, sizeof(sRegBuf), sTemp);
				}
				else {
					sprintf_s(sTemp, sizeof(sTemp), "(%.2f MHz)", fMaxReportedCPUSpeed);
					strcat_s(sRegBuf, sizeof(sRegBuf), sTemp);
				}
			}

			Set_Text(GetDlgItem(hWnd, IDC_CPUDESCR), sRegBuf);
		}


		return TRUE;
	}
	else if (uMsg == WM_COMMAND)
	{
		if (wParam == IDC_HELPBUTTON)
		{
			OpenHelpFile("Configuration/Processor Settings.Html");
			return TRUE;
		}
	}
	else if (uMsg == WM_NOTIFY)
	{
		LPNMHDR pNMH = (LPNMHDR)lParam;
		if (pNMH->code == PSN_KILLACTIVE)
		{
			return FALSE;
		}
		if (pNMH->code == PSN_APPLY) // Ok
		{
			DWORD_PTR dwCPUAffinityMask = 0;
			DWORD iItems = ListView_GetItemCount(hCPUs);
			DWORD dwItem = 0;

			while (dwItem < iItems)
			{
				if (ListView_GetCheckState(hCPUs, dwItem))
				{
					SetBit(dwCPUAffinityMask, dwItem);
				}

				dwItem++;
			}

			if (dwCPUAffinityMask == 0)
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
		if (pNMH->code == PSN_RESET) // Cancel
		{
			return FALSE;
		}
	}

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void PopulateProcessors(HWND hListView)
{
	DWORD len = 0;
	GetLogicalProcessorInformationEx(RelationProcessorCore, NULL, &len);
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)malloc(len);
	GetLogicalProcessorInformationEx(RelationProcessorCore, buffer, &len);

	int coreIndex = 0;
	BYTE* ptr = (BYTE*)buffer;
	LV_ITEM lvItem;

	bool supportsEfficiencyCores = false;

	while (ptr < ((BYTE*)buffer + len))
	{
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;

		if (info->Relationship == RelationProcessorCore && info->Processor.EfficiencyClass > 0)
		{
			supportsEfficiencyCores = true;
			break;

		}
		ptr += info->Size;
	}

	ptr = (BYTE*)buffer;
	while (ptr < ((BYTE*)buffer + len))
	{
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;

		if (info->Relationship == RelationProcessorCore)
		{
			DWORD speedMHz = 0;
			char regPath[128];
			sprintf_s(regPath, sizeof(regPath),
				"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\%d", coreIndex);

			char sTemp[255];
			memset(sTemp, 0, sizeof(sTemp));
			sprintf_s(sTemp, sizeof(sTemp), "Core %2d", coreIndex);

			lvItem.pszText = sTemp;
			lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
			lvItem.iItem = coreIndex;
			if (supportsEfficiencyCores)
			{
				lvItem.iImage = (info->Processor.EfficiencyClass > 0) ? 0 : 1; // 0 = Performance, 1 = Efficiency
			}
			else
			{
				lvItem.iImage = 0;
			}
			lvItem.iSubItem = 0;
			ListView_InsertItem(hListView, &lvItem);

			if (supportsEfficiencyCores)
			{
				lvItem.pszText = (info->Processor.EfficiencyClass > 0) ? "Performance" : "Efficiency";
			}
			else
			{
				lvItem.pszText = "Standard CPU";
			}
			lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
			lvItem.iImage = 0;
			lvItem.iItem = coreIndex;
			lvItem.iSubItem = 1;
			ListView_SetItem(hListView, &lvItem);

			if (IsBitSet(gdwCPUAffinityMask, coreIndex))
			{
				ListView_SetCheckState(hListView, coreIndex, TRUE);
			}
			else {
				ListView_SetCheckState(hListView, coreIndex, FALSE);
			}

			coreIndex++;
		}

		ptr += info->Size;
	}

	free(buffer);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
