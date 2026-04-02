#include "Registry.h"
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <filesystem>

namespace SmartExtract {

extern HINSTANCE g_hInstance;

bool Registry::Register(const std::wstring& dllPath) {
    // 1. 注册CLSID\InprocServer32
    std::wstring clsidKey = std::wstring(L"CLSID\\") + CLSID_STRING;
    HKEY hKey = nullptr;

    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, clsidKey.c_str(),
                        0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    const wchar_t* desc = L"Smart Extract Here";
    RegSetValueExW(hKey, nullptr, 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(desc),
                   (DWORD)((wcslen(desc) + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);

    std::wstring inprocKey = clsidKey + L"\\InprocServer32";
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, inprocKey.c_str(),
                        0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    RegSetValueExW(hKey, nullptr, 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(dllPath.c_str()),
                   (DWORD)((dllPath.length() + 1) * sizeof(wchar_t)));
    const wchar_t* model = L"Apartment";
    RegSetValueExW(hKey, L"ThreadingModel", 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(model),
                   (DWORD)((wcslen(model) + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);

    // 2. 注册到 HKCR\*\shellex\ContextMenuHandlers（所有文件生效）
    std::wstring handlerKey = L"*\\shellex\\ContextMenuHandlers\\SmartExtract";
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, handlerKey.c_str(),
                        0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    RegSetValueExW(hKey, nullptr, 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(CLSID_STRING),
                   (DWORD)((wcslen(CLSID_STRING) + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);

    // 3. 刷新Shell
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return true;
}

bool Registry::Unregister() {
    // 1. 删除 * 下的注册
    std::wstring handlerKey = L"*\\shellex\\ContextMenuHandlers\\SmartExtract";
    RegDeleteTreeW(HKEY_CLASSES_ROOT, handlerKey.c_str());

    // 2. 删除CLSID注册
    std::wstring clsidKey = std::wstring(L"CLSID\\") + CLSID_STRING;
    RegDeleteTreeW(HKEY_CLASSES_ROOT, clsidKey.c_str());

    // 3. 刷新Shell
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return true;
}

bool Registry::IsRegistered() {
    std::wstring keyPath = std::wstring(L"CLSID\\") + CLSID_STRING + L"\\InprocServer32";
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, keyPath.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

} // namespace SmartExtract
