#include "start_on_boot.h"
#include "debug.h"

#include <string>
#include <sstream>
#include <filesystem>
#include <shlobj.h>
#include <windows.h>
#include <Lmcons.h>

namespace {
    HRESULT CreateLink(LPCWSTR lpszPathObj1, LPCWSTR lpszPathLink, LPCWSTR lpszDesc, LPCWSTR lpszarg)
    {
        HRESULT hres;
        IShellLinkW* psl;

        // Get a pointer to the IShellLink interface. It is assumed that CoInitialize
        // has already been called.
        hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));
        if (SUCCEEDED(hres))
        {
            IPersistFile* ppf;

            // Set the path to the shortcut target and add the description.
            psl->SetPath(lpszPathObj1);
            psl->SetArguments(lpszarg);
            psl->SetDescription(lpszDesc);

            // Query IShellLink for the IPersistFile interface, used for saving the
            // shortcut in persistent storage.
            hres = psl->QueryInterface(IID_PPV_ARGS(&ppf));
            if (SUCCEEDED(hres))
            {
                // Save the link by calling IPersistFile::Save.
                hres = ppf->Save(lpszPathLink, TRUE);
                ppf->Release();
            }
            psl->Release();
        }
        return hres;
    }

    std::wstring GetStartUpLinkPath(const std::wstring& username, const std::wstring& progName)
    {
        std::wstringstream path;
        path << L"C:\\Users\\" << username << L"\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\" << progName << ".lnk";
        return path.str();
    }
}

namespace StartOnBoot {
    bool StartUpLinkExits()
    {
        WCHAR username[UNLEN + 1];
        DWORD username_len = UNLEN + 1;

        // Retuns true if OK so we need to flip it for HRESULT
        HRESULT ret = !GetUserName(username, &username_len);
        if (ret != S_OK)
            return false;

        std::wstring progName = std::filesystem::path(__argv[0]).filename().wstring();

        std::wstring linkPath = ::GetStartUpLinkPath(username, progName);
        return std::filesystem::exists(linkPath);
    }

    void CleanUpStartOnBoot()
    {
        WCHAR username[UNLEN + 1];
        DWORD username_len = UNLEN + 1;

        // Retuns true if OK so we need to flip it for HRESULT
        HRESULT ret = !GetUserName(username, &username_len);
        if (ret != S_OK)
            return;

        std::wstring progName = std::filesystem::path(__argv[0]).filename().wstring();
        std::wstring linkPath = GetStartUpLinkPath(username, progName);
        if (std::filesystem::exists(linkPath))
            std::filesystem::remove(linkPath);
    }

    HRESULT SetupStartOnBoot(const std::string& args)
    {
        WCHAR username[UNLEN + 1];
        DWORD username_len = UNLEN + 1;

        // Retuns true if OK so we need to flip it for HRESULT
        HRESULT ret = !GetUserName(username, &username_len);
        if (ret != S_OK)
            return E_FAIL;

        std::wstring progName = std::filesystem::path(__argv[0]).filename().wstring();

        std::wstring target = std::filesystem::temp_directory_path().wstring() + progName;

        ret = !std::filesystem::copy_file(__argv[0], target, std::filesystem::copy_options::overwrite_existing);
        if (ret != S_OK)
            return E_FAIL;

        std::wstring linkPath = GetStartUpLinkPath(username, progName);

        if ((ret = CoInitialize(NULL)) == S_OK) {
            ret = CreateLink(target.c_str(), linkPath.c_str(), L"", std::wstring(args.begin(), args.end()).c_str());
            CoUninitialize();
        }

        return ret;
    }
}