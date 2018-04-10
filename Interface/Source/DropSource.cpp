///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Implementation of the IDropSource COM interface
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define STRICT

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <ShlObj.H>
#include <Stdio.H>
#include <Stdlib.H>

#include "Entry.H"
#include "Routines.H"
#include "Package.H"
#include "Workers.H"
#include "Init.H"
#include "DragDrop.H"

#ifndef _SELFEXTRACTOR_APP
#include "../Dialogs/ExtractDlg.H"
#include "../Dialogs/MainDlg.H"
#include "../Dialogs/ReadingDlg.H"
#include "../Dialogs/NewPassDlg.H"
#include "../Dialogs/HardKeyDlg.H"
#endif

#include "../Dialogs/OpenPassDlg.H"
#include "../Dialogs/ProgressDialog.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows; 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CDropSource : public IDropSource
{
public:
	//
    // IUnknown members
	//
    HRESULT __stdcall QueryInterface	(REFIID iid, void ** ppvObject);
    ULONG   __stdcall AddRef			(void);
    ULONG   __stdcall Release			(void);
		
    //
	// IDropSource members
	//
    HRESULT __stdcall QueryContinueDrag	(BOOL fEscapePressed, DWORD grfKeyState);
	HRESULT __stdcall GiveFeedback		(DWORD dwEffect);
	
	//
    // Constructor / Destructor
	//
    CDropSource();
    ~CDropSource();
	
private:

    //
	// private members and functions
	//
    LONG m_lRefCount;
	DWORD m_Effect;
	HWND m_hTaskBar;
};

//
//	Constructor
//
CDropSource::CDropSource() 
{
	this->m_lRefCount = 1;
	this->m_Effect = DROPEFFECT_NONE;
	this->m_hTaskBar = FindWindow("Shell_TrayWnd", NULL);
}

//
//	Destructor
//
CDropSource::~CDropSource()
{
}

//
//	IUnknown::AddRef
//
ULONG __stdcall CDropSource::AddRef(void)
{
    // increment object reference count
    return InterlockedIncrement(&m_lRefCount);
}

//
//	IUnknown::Release
//
ULONG __stdcall CDropSource::Release(void)
{
    // decrement object reference count
	LONG count = InterlockedDecrement(&m_lRefCount);
		
	if(count == 0)
	{
		delete this;
		return 0;
	}
	else
	{
		return count;
	}
}

//
//	IUnknown::QueryInterface
//
HRESULT __stdcall CDropSource::QueryInterface(REFIID iid, void **ppvObject)
{
    // check to see what interface has been requested
    if(iid == IID_IDropSource || iid == IID_IUnknown)
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }
}

//
//	CDropSource::QueryContinueDrag
//
//	Called by OLE whenever Escape/Control/Shift/Mouse buttons have changed
//
HRESULT __stdcall CDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
	// if the <Escape> key has been pressed since the last call, cancel the drop
	if(fEscapePressed == TRUE)
	{
		return DRAGDROP_S_CANCEL;
	}
	// if the <LeftMouse> button has been released, then do the drop!
	else if((grfKeyState & MK_LBUTTON) == 0)
	{
		if(m_Effect == DROPEFFECT_NONE || IsMouseOverHwnd(this->m_hTaskBar))
		{
			return DRAGDROP_S_CANCEL;
		}
		else{
			SetCapture(hMainDialog);

			if(DupsSelected())
			{
				SetForegroundWindow(hMainDialog);
				SetActiveWindow(hMainDialog);
				SetFocus(hMainDialog);

				MessageBox(hMainDialog,
					"You have selected multiple files that have the exact same name.\r\n"
					"Please check your selection and try again.",
					gsTitleCaption, MB_ICONINFORMATION);

				return DRAGDROP_S_CANCEL;
			}
			else{
				strcpy_s(gsExtractLocation, sizeof(gsExtractLocation), sTempDropPath);

				gdwCurrentJobType = JOB_TYPE_EXTRACT_SELECTED;
				gbOverwriteDlft = false;
				gbUseFolderNames = false;

				CreateWorker(ExtractEntireArchiveProc, (LPVOID) gsArchiveName);
				RunProgressDialog(hMainDialog);

				if(gdwCurrentJobType == JOB_TYPE_SUCCESS)
				{
					SetNormalDropAttributes();
					return DRAGDROP_S_DROP;
				}
				else{
					return DRAGDROP_S_CANCEL;
				}
			}
		}
	}

	// continue with the drag-drop
	return S_OK;
}

//
//	CDropSource::GiveFeedback
//
//	Return either S_OK, or DRAGDROP_S_USEDEFAULTCURSORS to instruct OLE to use the
//  default mouse cursor images
//
HRESULT __stdcall CDropSource::GiveFeedback(DWORD dwEffect)
{
	m_Effect = dwEffect;
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

//
//	Helper routine to create an IDropSource object
//	
HRESULT CreateDropSource(IDropSource **ppDropSource)
{
	if(ppDropSource == 0)
		return E_INVALIDARG;

	*ppDropSource = new CDropSource();

	return (*ppDropSource) ? S_OK : E_OUTOFMEMORY;

}