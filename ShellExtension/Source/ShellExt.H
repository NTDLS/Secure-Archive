#ifndef ShellExt_H
#define ShellExt_H
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define REGISTRYPATHKEY "Software\\NetWorkDLS\\Secure Archive\\Path"

// {8CC98F55-96FA-4bde-B01B-687EDFA6BD36}
DEFINE_GUID(CLSID_ShellExtension,
	0x8cc98f55, 0x96fa, 0x4bde, 0xb0, 0x1b, 0x68, 0x7e, 0xdf, 0xa6, 0xbd, 0x36);

/*
	Old GUIDs:
		A0BF07D5-B9CC-4864-91D0-6B30B979A324
		3dc8ff6a-9aea-42a5-8d23-9e294bac33e5
*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern UINT giRefThisDll;
extern HINSTANCE ghModThisDLL;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// this class factory object creates context menu handlers for Windows 95 shell
class CShellExtClassFactory : public IClassFactory
{
protected:
	ULONG	m_cRef;

public:
	CShellExtClassFactory();
	~CShellExtClassFactory();

	//IUnknown members
	STDMETHODIMP			QueryInterface(REFIID, LPVOID FAR*);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	//IClassFactory members
	STDMETHODIMP		CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR*);
	STDMETHODIMP		LockServer(BOOL);

};
typedef CShellExtClassFactory* LPCSHELLEXTCLASSFACTORY;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// this is the actual OLE Shell context menu handler
class CShellExt : public IShellExtInit, IContextMenu
{
public:
	char sFileUserClickedOn[MAX_PATH];
	HBITMAP hBitmap = NULL;

protected:
	ULONG		 m_cRef;
	LPDATAOBJECT m_pDataObj;

public:
	CShellExt();
	~CShellExt();

	//IUnknown members
	STDMETHODIMP			QueryInterface(REFIID, LPVOID FAR*);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	//IShell members
	STDMETHODIMP QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
	STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);

	HBITMAP CreateDIBFromIcon(char *lpName, int cxDesired, int cyDesired);

	//IShellExtInit methods
	STDMETHODIMP Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);
	HRESULT STDMETHODCALLTYPE GetCommandString(UINT_PTR idCmd, UINT uType, UINT* pwReserved, LPSTR pszName, UINT cchMax);

};
typedef CShellExt* LPCSHELLEXT;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

