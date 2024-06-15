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

        BOOL ret = GetUserName(username, &username_len);
        if (!ret)
            return false;

        int nArgs;
        LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
        if (!szArglist || nArgs == 0)
            return false;

        const std::wstring progName = std::filesystem::path(szArglist[0]).filename().wstring();

        const std::wstring linkPath = ::GetStartUpLinkPath(username, progName);
        return std::filesystem::exists(linkPath);
    }

    void CleanUpStartOnBoot()
    {
        WCHAR username[UNLEN + 1];
        DWORD username_len = UNLEN + 1;

        BOOL ret = GetUserName(username, &username_len);
        if (!ret)
            return;

        const std::wstring progName = std::filesystem::path(__argv[0]).filename().wstring();
        const std::wstring linkPath = GetStartUpLinkPath(username, progName);
        if (std::filesystem::exists(linkPath))
            std::filesystem::remove(linkPath);
    }

    HRESULT SetupStartOnBoot(const std::string& args)
    {
        WCHAR username[UNLEN + 1];
        DWORD username_len = UNLEN + 1;

        bool ret = GetUserName(username, &username_len);
        if (!ret)
            return E_FAIL;

        const std::wstring progName = std::filesystem::path(__argv[0]).filename().wstring();
        const std::wstring target = std::filesystem::temp_directory_path().wstring() + progName;

        ret = std::filesystem::copy_file(__argv[0], target, std::filesystem::copy_options::overwrite_existing);
        if (!ret)
            return E_FAIL;

        const std::wstring linkPath = GetStartUpLinkPath(username, progName);

        HRESULT initRet = CoInitialize(NULL);
        if (initRet == S_OK) {
            initRet = CreateLink(target.c_str(), linkPath.c_str(), L"", std::wstring(args.begin(), args.end()).c_str());
            CoUninitialize();
        }

        return initRet;
    }
}