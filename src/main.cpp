#define NOMINMAX
#include <windows.h>
#include <objbase.h>
#include <olectl.h>
#include "ContextMenu.h"
#include "Registry.h"

namespace SmartExtract {
HINSTANCE g_hInstance = nullptr;
LONG g_dllRefCount = 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        SmartExtract::g_hInstance = hModule;
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}

// CLSID定义（必须与Registry.h中的CLSID_STRING一致）
static const CLSID CLSID_SmartExtract =
    { 0xa8e8a8a8, 0xe8a8, 0x4a8a,
      { 0xa8, 0xe8, 0xa8, 0xe8, 0xa8, 0xa8, 0xa8, 0xa8 } };

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv) {
    if (!IsEqualCLSID(rclsid, CLSID_SmartExtract)) return CLASS_E_CLASSNOTAVAILABLE;
    auto* factory = new (std::nothrow) SmartExtract::ContextMenuFactory();
    if (!factory) return E_OUTOFMEMORY;
    HRESULT hr = factory->QueryInterface(riid, ppv);
    factory->Release();
    return hr;
}

STDAPI DllCanUnloadNow() {
    return (SmartExtract::g_dllRefCount == 0) ? S_OK : S_FALSE;
}

STDAPI DllRegisterServer() {
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(SmartExtract::g_hInstance, path, MAX_PATH);
    return SmartExtract::Registry::Register(path) ? S_OK : SELFREG_E_CLASS;
}

STDAPI DllUnregisterServer() {
    return SmartExtract::Registry::Unregister() ? S_OK : SELFREG_E_CLASS;
}
