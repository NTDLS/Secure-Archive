#ifndef _INIT_CPP
#define _INIT_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <WindowsX.H>
#include <ShellAPI.H>
#include <Stdio.H>
#include <Stdlib.H>
#include <CommCtrl.H>

#include "../../SecureDeleteLibrary/SecureDelete.H"

#include "Init.H"
#include "Entry.H"
#include "Routines.H"
#include "Workers.H"
#include "PackHeader.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Registry;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void EnableDPI(void)
{
	typedef BOOL (*SetProcessDPIAwareFunc)();

	HMODULE hUser32 = LoadLibrary("user32.dll");
	if(hUser32)
	{
		SetProcessDPIAwareFunc setDPIAware
			= (SetProcessDPIAwareFunc)GetProcAddress(hUser32, "SetProcessDPIAware");

		if(setDPIAware)
		{
			setDPIAware();
		}

		FreeLibrary(hUser32);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool InitializeApp(void)
{
	LoadString(ghAppInstance, IDS_APPCOPYRIGHT, gsAppCopyRight, sizeof(gsAppCopyRight));
    LoadString(ghAppInstance, IDS_FILEVERSION, gsFileVersion, sizeof(gsFileVersion));
    LoadString(ghAppInstance, IDS_TITLECAPTION, gsTitleCaption, sizeof(gsTitleCaption));
    LoadString(ghAppInstance, IDS_REGISTRYKEY, gsRegistryKey, sizeof(gsRegistryKey));
    LoadString(ghAppInstance, IDS_PATHKEY, gsPathKey, sizeof(gsPathKey));
    LoadString(ghAppInstance, IDS_COMPATIBILITY, gsCompatibility, sizeof(gsCompatibility));
    LoadString(ghAppInstance, IDS_EMPTYPASSWORD, gsEmptyPassword, sizeof(gsEmptyPassword));

	//EnableDPI(); //For Vista DPI.

#ifndef _SELFEXTRACTOR_APP
	OleInitialize(0);
#endif

	memset(&gTrayIcon, 0, sizeof(CTrayIcon));
	memset(gsLastPath, 0, sizeof(gsLastPath));
	memset(gsWorkingOutput, 0, sizeof(gsWorkingOutput));
	memset(gsSecureDeletePopulate, 0, sizeof(gsSecureDeletePopulate));

	InitGlobalCode();

	INITCOMMONCONTROLSEX CCEX;
	CCEX.dwSize = sizeof(CCEX);
	CCEX.dwICC = ICC_TREEVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_BAR_CLASSES | ICC_ANIMATE_CLASS | ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&CCEX);

#ifndef _SELFEXTRACTOR_APP
	if(!GetRegistryValues())
	{
		SafeMsgBox(NULL, "Failed to get registry values, please reinstall Secure Archive.",
			gsTitleCaption, MB_ICONERROR);
	}
		
	//Set the default Affinity Mask if it is not set.
	if(gdwCPUAffinityMask == 0)
	{
		DWORD_PTR dwProcessAffinityMask = 0;
		DWORD_PTR dwSystemAffinityMask = 0;

		if(!GetProcessAffinityMask(GetCurrentProcess(), &dwProcessAffinityMask, &dwSystemAffinityMask))
		{
			gdwCPUAffinityMask = -1;
		}
		else{
			gdwCPUAffinityMask = dwSystemAffinityMask;
		}
	}

	SetProcessAffinityMask(GetCurrentProcess(), gdwCPUAffinityMask);
	SetProcessPriorityBoost(GetCurrentProcess(), !gbUseProcessBoost);
#endif

	if(!InitSecureDelete())
	{
		gdwSecureDeletePasses = 0; //Secure delete will not be available.
		//FIXFIX: Handle error!
	}

	gCRC32 = new CRC32();

	gCRC32->Initialize();

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
bool SetRegistryValues(void)
{
	Set_DWORDRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "CPUAffinityMask", gdwCPUAffinityMask);

	Set_DWORDRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "WeakEncryptionWarnLevel", gdwWeakEncryptionWarnLevel);
	Set_DWORDRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "SecureDeletePasses", gdwSecureDeletePasses);
	Set_DWORDRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltDefaultSortColumn", gdwDefaultSortColumn);
	Set_DWORDRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltDefaultSortDirection", gdwDefaultSortDirection);

	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "WarnAboutNoEncryption", gbWarnAboutNoEncryption);
	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "AlwaysRunInBackground", gbAlwaysRunInBackground);
	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "UseProcessBoost", gbUseProcessBoost);
	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "UseThreadBoost", gbUseThreadBoost);
	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltChkTimeStamps", gbDfltChkTimeStamps);
	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltChkAttributes", gbDfltChkAttributes);
	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltChkPathAttributes", gbDfltChkPathAttributes);
	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltChkCRCs", gbDfltChkCRCs);
	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltChkSavePaths", gbDfltChkSavePaths);

	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "IgnoreFreeSpaceWarning", gbIgnoreFreeSpaceWarning);
	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltShowGridLines", gbShowGridLines);
	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltShowGridIcons", gbShowGridIcons);
	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltEnableFullRowSelect", gbEnableFullRowSelect);
	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltEnableDragDrop", gbEnableDragDrop);
	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltAlwaysOnTop", gbAlwaysOnTop);
	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltChkIncludeHiddenFiles", gbIncludeHiddenFiles);
	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltDeleteSourceFiles", gbDeleteSourceFiles);
	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "KeepSelectionVisible", gbKeepSelectionVisible);
	Set_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "UsePreAllocation", gbUsePreAllocation);

	Set_StringRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltCompressLevel", gsDfltCompressLevel);
	Set_StringRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltEncryptAlgo", gsDfltEncryptAlgo);
	Set_StringRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltTempFilePath", gsTempFilePath);

	return true;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
bool CopyRegValue(HKEY hSourceRoot, HKEY hTargetRoot, const char *sKeyPath,
				  const char *sValueName, DWORD dwValueType)
{
	DWORD dwTestSize = 0;


	if(Get_BinaryRegistryValue(hTargetRoot, sKeyPath, sValueName, NULL, dwTestSize))
	{
		return true; //Value already exists.
	}

	if(dwValueType == REG_QWORD)
	{
		unsigned __int64 u64Buffer = 0;
		if(Get_DWORDRegistryValue(hSourceRoot, sKeyPath, sValueName, u64Buffer))
		{
			return Set_DWORDRegistryValue(hTargetRoot, sKeyPath, sValueName, u64Buffer);
		}
	}
	else if(dwValueType == REG_DWORD)
	{
		DWORD dwBuffer = 0;
		if(Get_DWORDRegistryValue(hSourceRoot, sKeyPath, sValueName, dwBuffer))
		{
			return Set_DWORDRegistryValue(hTargetRoot, sKeyPath, sValueName, dwBuffer);
		}
	}
	else if(dwValueType == REG_BINARY)
	{
		//Ignore the [Registration] value.
		return true;
		/*
		DWORD dwBufferSize = 0;
		if(Get_BinaryRegistryValue(hSourceRoot, sKeyPath, sValueName, NULL, dwBufferSize))
		{
			bool bResult = false;

			char *sBuffer = (char *)calloc(dwBufferSize + 1, sizeof(char));
			dwBufferSize++;
			if(Get_BinaryRegistryValue(hSourceRoot, sKeyPath, sValueName, sBuffer, dwBufferSize))
			{
				dwBufferSize--;
				bResult = Set_BinaryRegistryValue(hTargetRoot, sKeyPath, sValueName, sBuffer, dwBufferSize);
			}
			free(sBuffer);

			return bResult;
		}
		*/
	}
	else if(dwValueType == REG_SZ || dwValueType == REG_EXPAND_SZ)
	{
		DWORD dwBufferSize = 0;
		if(Get_StringRegistryValue(hSourceRoot, sKeyPath, sValueName, NULL, dwBufferSize))
		{
			bool bResult = false;

			char *sBuffer = (char *)calloc(dwBufferSize + 1, sizeof(char));
			dwBufferSize++;
			if(Get_StringRegistryValue(hSourceRoot, sKeyPath, sValueName, sBuffer, dwBufferSize))
			{
				dwBufferSize--;
				sBuffer[dwBufferSize] = '\0';
				bResult = Set_StringRegistryValue(hTargetRoot, sKeyPath, sValueName, sBuffer);
			}
			free(sBuffer);

			return bResult;
		}
	}
	else{
		char sText[512];
		sprintf_s(sText, sizeof(sText), "%d is an invalid registry value type.", dwValueType);
		Assert("dwValueType", sText);
		__debugbreak();
	}

	return false;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
bool CopyTemplateRegistrySettings(const char *sPath)
{
	HKEY hKLM = NULL;
	char sName[255];
	DWORD dwType = 0;
	DWORD dwNameSize = sizeof(sName);

	CreateRegKeyStructure(HKEY_CURRENT_USER, sPath);

	if(RegOpenKey(HKEY_LOCAL_MACHINE, sPath, &hKLM) != ERROR_SUCCESS)
	{
		return false;
	}

	for(DWORD dwIndex = 0;
		RegEnumValue(hKLM, dwIndex, sName, &dwNameSize, 0, &dwType, NULL, 0) == ERROR_SUCCESS;
		dwIndex++)
	{
		if(!CopyRegValue(HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER, sPath, sName, dwType))
		{
			char sText[2048];
			sprintf_s(sText, sizeof(sText), "Failed to copy registry value [%s] to local profile", sName);
			SafeMsgBox(GetActiveWindow(), sText, gsTitleCaption, MB_ICONERROR);
		}
		dwNameSize = sizeof(sName);
	}

	RegCloseKey(hKLM);

	return true;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SELFEXTRACTOR_APP
bool GetRegistryValues(void)
{
	if(!CopyTemplateRegistrySettings(gsRegistryKey))
	{
		SafeMsgBox(GetActiveWindow(),
			"Failed to copy template registry settings to local user account.",
			gsTitleCaption, MB_ICONERROR);		
		return false;
	}

	DWORD dwSize = 0;

	if(!Get_DWORDRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltDefaultSortColumn", gdwDefaultSortColumn))
	{
		gdwDefaultSortColumn = 0;
	}

	if(!Get_DWORDRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltDefaultSortDirection", gdwDefaultSortDirection))
	{
		gdwDefaultSortDirection = 0;
	}

	if(!Get_DWORDRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "WeakEncryptionWarnLevel", gdwWeakEncryptionWarnLevel))
	{
		gdwWeakEncryptionWarnLevel = KW_ACCEPTABLE;
	}

	if(!Get_DWORDRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "SecureDeletePasses", gdwSecureDeletePasses))
	{
		gdwSecureDeletePasses = 0;
	}

	if(!Get_DWORDRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "CPUAffinityMask", gdwCPUAffinityMask))
	{
		gdwSecureDeletePasses = 0;
	}

	gbWarnAboutNoEncryption = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "WarnAboutNoEncryption");
	gbAlwaysRunInBackground = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "AlwaysRunInBackground");
	gbUseProcessBoost = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "UseProcessBoost");
	gbUseThreadBoost = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "UseThreadBoost");
	gbDfltChkTimeStamps = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltChkTimeStamps");
	gbDfltChkAttributes = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltChkAttributes");
	gbDfltChkPathAttributes = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltChkPathAttributes");

	gbDfltChkCRCs = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltChkCRCs");
	gbDfltChkSavePaths = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltChkSavePaths");

	gbIgnoreFreeSpaceWarning = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "IgnoreFreeSpaceWarning");
	gbShowGridLines = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltShowGridLines");
	gbShowGridIcons = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltShowGridIcons");
	gbEnableFullRowSelect = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltEnableFullRowSelect");
	gbEnableDragDrop = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltEnableDragDrop");
	gbAlwaysOnTop = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltAlwaysOnTop");
	gbKeepSelectionVisible = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "KeepSelectionVisible");
	gbUsePreAllocation = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "UsePreAllocation");
	gbIncludeHiddenFiles = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltChkIncludeHiddenFiles");
	gbDeleteSourceFiles = Get_BOOLRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltDeleteSourceFiles");

	dwSize = sizeof(gsDfltCompressLevel);
	memset(gsDfltCompressLevel, 0, dwSize);
	Get_StringRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltCompressLevel", gsDfltCompressLevel, dwSize);

	dwSize = sizeof(gsDfltEncryptAlgo);
	memset(gsDfltEncryptAlgo, 0, dwSize);
	Get_StringRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltEncryptAlgo", gsDfltEncryptAlgo, dwSize);

	dwSize = sizeof(gsTempFilePath);
	memset(gsTempFilePath, 0, dwSize);
	Get_StringRegistryValue(HKEY_CURRENT_USER, gsRegistryKey, "DfltTempFilePath", gsTempFilePath, dwSize);

	dwSize = sizeof(gsPath);
	memset(gsPath, 0, dwSize);
	Get_StringRegistryValue(HKEY_LOCAL_MACHINE, gsPathKey, "Path", gsPath, dwSize);

	//-----------------------------------------------------------------------------------------

	ResetDefaultAttributes();
	ResetEncryptionOptions();

	//-----------------------------------------------------------------------------------------
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

	return true;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool InitGlobalCode(void)
{
	#define GCODESIZE 1024
	char sKeyHash[GCODESIZE + 1];
	HashKey(gsEmptyPassword, (int)strlen(gsEmptyPassword), sKeyHash,  GCODESIZE);
	return gCode.Initialize(sKeyHash, GCODESIZE, true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool UninitializeApp(void)
{
	gCode.Destroy();

#ifndef _SELFEXTRACTOR_APP
	SetRegistryValues();
#endif

	delete gCRC32;

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

