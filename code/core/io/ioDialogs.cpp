#include "ioDialogs.h"

#include "utils.h"

#include "..\system\faultHandler.h"
#include "..\system\video\video3d.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

void IO::Dialogs::setup_for_xml(Params& _params)
{
	_params.defaultExtension = TXT("xml");
	_params.extensions.push_back();
	{
		auto& e = _params.extensions.get_last();
		e.extension = TXT("*.xml");
		e.info = TXT("XML file");
	}
}

#ifdef AN_WINDOWS
#include <windows.h>
#include <shlobj.h>
#include <objbase.h>      // For COM headers
#include <shobjidl_core.h>
#include <shlwapi.h>
#include <knownfolders.h> // for KnownFolder APIs/datatypes/function headers
#include <propvarutil.h>  // for PROPVAR-related functions
#include <propkey.h>      // for the Property key APIs/datatypes
#include <propidl.h>      // for the Property System APIs
#include <strsafe.h>      // for StringCchPrintfW
#include <shtypes.h>      // for COMDLG_FILTERSPEC

#pragma comment(lib, "Shlwapi.lib")

// code is based on samples provided by Microsoft:
// https ://learn.microsoft.com/en-us/windows/win32/shell/samples-commonfiledialog

class CDialogEventHandler
: public IFileDialogEvents
, public IFileDialogControlEvents
{
public:
    // IUnknown methods
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        static const QITAB qit[] = {
            QITABENT(CDialogEventHandler, IFileDialogEvents),
            QITABENT(CDialogEventHandler, IFileDialogControlEvents),
            { 0 },
#pragma warning(suppress:4838)
        };
        return QISearch(this, qit, riid, ppv);
    }

    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&_cRef);
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        long cRef = InterlockedDecrement(&_cRef);
        if (!cRef)
            delete this;
        return cRef;
    }

    // IFileDialogEvents methods
	IFACEMETHODIMP OnFileOk(IFileDialog*) { return S_OK; }
    IFACEMETHODIMP OnFolderChange(IFileDialog *) { return S_OK; }
	IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem*) { return S_OK; }
	IFACEMETHODIMP OnHelp(IFileDialog*) { return S_OK; }
	IFACEMETHODIMP OnSelectionChange(IFileDialog*) { return S_OK; }
	IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) { return S_OK; }
    IFACEMETHODIMP OnTypeChange(IFileDialog *pfd) { return S_OK; }
	IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) { return S_OK; }

    // IFileDialogControlEvents methods
	IFACEMETHODIMP OnItemSelected(IFileDialogCustomize* pfdc, DWORD dwIDCtl, DWORD dwIDItem) { return S_OK; }
	IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize*, DWORD) { return S_OK; }
	IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize*, DWORD, BOOL) { return S_OK; }
	IFACEMETHODIMP OnControlActivating(IFileDialogCustomize*, DWORD) { return S_OK; }

    CDialogEventHandler() : _cRef(1) { };
private:
    ~CDialogEventHandler() { };
    long _cRef;
};

HRESULT CDialogEventHandler_CreateInstance(REFIID riid, void **ppv)
{
    *ppv = NULL;
    CDialogEventHandler *pDialogEventHandler = new (std::nothrow) CDialogEventHandler();
    HRESULT hr = pDialogEventHandler ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        hr = pDialogEventHandler->QueryInterface(riid, ppv);
        pDialogEventHandler->Release();
    }
    return hr;
}

struct WindowsFileDialog
{
	enum Type
	{
		Save,
		Open
	};

	static void into_windows_directory_inline(REF_ String & directory)
	{
		for_every(c, directory.access_data())
		{
			*c = *c == '/' ? '\\' : *c;
		}
	}

	WindowsFileDialog(Type _type, IO::Dialogs::Params& _params)
	:type(_type), params(_params)
	{}

	Type type = Open;
	IO::Dialogs::Params& params;

	Optional<String> do_dialog()
	{
		Optional<String> result;

		System::FaultHandler::set_ignore_faults(true);

		// CoCreate the File Open Dialog object.
		IFileDialog* pfd = NULL;
		HRESULT hr = CoCreateInstance(type == Save? CLSID_FileSaveDialog : CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
		if (SUCCEEDED(hr))
		{
			// Create an event handling object, and hook it up to the dialog.
			IFileDialogEvents* pfde = NULL;
			hr = CDialogEventHandler_CreateInstance(IID_PPV_ARGS(&pfde));
			if (SUCCEEDED(hr))
			{
				// Hook up the event handler.
				DWORD dwCookie;
				hr = pfd->Advise(pfde, &dwCookie);
				if (SUCCEEDED(hr))
				{
					// options
					{
						DWORD dwFlags;

						// Before setting, always get the options first in order not to override existing options.
						hr = pfd->GetOptions(&dwFlags);
						if (SUCCEEDED(hr))
						{
							dwFlags |= FOS_FORCEFILESYSTEM;
							if (type == Open)
							{
								dwFlags |= FOS_FILEMUSTEXIST;
							}
							hr = pfd->SetOptions(dwFlags);
						}
					}

					// save types
					{
						Array<COMDLG_FILTERSPEC> saveTypes;
						for_every(ext, params.extensions)
						{
							saveTypes.push_back();
							saveTypes.get_last().pszName = ext->info.to_char();
							saveTypes.get_last().pszSpec = ext->extension.to_char();
						}
						// Set the file types to display only. Notice that, this is a 1-based array.
						hr = pfd->SetFileTypes(saveTypes.get_size(), saveTypes.get_data());
						if (SUCCEEDED(hr))
						{
							// Set the selected file type index to Word Docs for this example.
							hr = pfd->SetFileTypeIndex(1);
							if (SUCCEEDED(hr))
							{
								// Set the default extension to be ".doc" file.
								hr = pfd->SetDefaultExtension(params.defaultExtension.to_char());
							}
						}
					}

					// directories
					{
						String openDir = params.startInDirectory;

						// existing file
						if (!params.existingFile.is_empty())
						{
							String existingFileName = IO::Utils::get_file(params.existingFile);
							if (params.startInDirectory.is_empty())
							{
								openDir = IO::Utils::get_directory(params.existingFile);
							}
							pfd->SetFileName(existingFileName.to_char());
						}

						// get folder
						if (!openDir.is_empty())
						{
							IShellItem* pFolder = NULL;
							into_windows_directory_inline(REF_ openDir);
							hr = SHCreateItemFromParsingName(openDir.to_char(), NULL, IID_PPV_ARGS(&pFolder));
							if (!SUCCEEDED(hr))
							{
								openDir = IO::Utils::make_absolute(openDir);
								into_windows_directory_inline(REF_ openDir);
								hr = SHCreateItemFromParsingName(openDir.to_char(), NULL, IID_PPV_ARGS(&pFolder));
							}
							if (SUCCEEDED(hr))
							{
								pfd->SetDefaultFolder(pFolder);
								pfd->SetFolder(pFolder);
								pFolder->Release();
							}
						}
					}

					// open and get result
					{
						// Show the dialog
						hr = pfd->Show(NULL);
					}

					if (SUCCEEDED(hr))
					{
						// Obtain the result, once the user clicks the 'Open' button.
						// The result is an IShellItem object.
						IShellItem* psiResult;
						hr = pfd->GetResult(&psiResult);
						if (SUCCEEDED(hr))
						{
							// We are just going to print out the name of the file for sample sake.
							PWSTR pszFilePath = NULL;
							hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
							if (SUCCEEDED(hr))
							{
								result = String(pszFilePath);
							}
							psiResult->Release();
						}
					}

					// Unhook the event handler.
					pfd->Unadvise(dwCookie);
				}
				pfde->Release();
			}
			pfd->Release();
		}

		System::FaultHandler::set_ignore_faults(false);

		return result;
	}
};

#endif

Optional<String> IO::Dialogs::get_file_to_save(Params& _params)
{
#ifdef AN_WINDOWS
	WindowsFileDialog fileDialog(WindowsFileDialog::Save, _params);
	return fileDialog.do_dialog();
#endif

	todo_important(TXT("implement"));

	return NP;
}

Optional<String> IO::Dialogs::get_file_to_open(Params& _params)
{
#ifdef AN_WINDOWS
	WindowsFileDialog fileDialog(WindowsFileDialog::Open, _params);
	return fileDialog.do_dialog();
#endif

	todo_important(TXT("implement"));

	return NP;
}
