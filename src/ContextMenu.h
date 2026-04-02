#pragma once
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <string>
#include <vector>

namespace SmartExtract {

class ContextMenu : public IShellExtInit, public IContextMenu {
public:
    ContextMenu();
    ~ContextMenu();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IShellExtInit
    STDMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidlFolder,
                           IDataObject* pdtobj,
                           HKEY hkeyProgID) override;

    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu,
                                  UINT idCmdFirst, UINT idCmdLast,
                                  UINT uFlags) override;
    STDMETHODIMP InvokeCommand(CMINVOKECOMMANDINFO* pici) override;
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uFlags,
                                  UINT* pwReserved, LPSTR pszName,
                                  UINT cchMax) override;

private:
    LONG refCount_;
    std::vector<std::wstring> selectedFiles_; // 支持多选
};

class ContextMenuFactory : public IClassFactory {
public:
    ContextMenuFactory();
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
    STDMETHODIMP CreateInstance(IUnknown* pOuter, REFIID riid, void** ppv) override;
    STDMETHODIMP LockServer(BOOL fLock) override;
private:
    LONG refCount_;
};

} // namespace SmartExtract
