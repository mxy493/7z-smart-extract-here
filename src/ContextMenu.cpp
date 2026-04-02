#include "ContextMenu.h"
#include "ArchiveAnalyzer.h"
#include "ConflictHandler.h"
#include "Registry.h"
#include "Resource.h"
#include <shellapi.h>
#include <strsafe.h>
#include <algorithm>
#include <filesystem>

namespace SmartExtract {

extern HINSTANCE g_hInstance;
extern LONG g_dllRefCount;

// ==================== ContextMenu ====================

ContextMenu::ContextMenu() : refCount_(1) {}
ContextMenu::~ContextMenu() {}

STDMETHODIMP ContextMenu::QueryInterface(REFIID riid, void** ppv) {
    static const QITAB qit[] = {
        QITABENT(ContextMenu, IShellExtInit),
        QITABENT(ContextMenu, IContextMenu),
        { 0 }
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) ContextMenu::AddRef() { return InterlockedIncrement(&refCount_); }
STDMETHODIMP_(ULONG) ContextMenu::Release() {
    LONG ref = InterlockedDecrement(&refCount_);
    if (ref == 0) delete this;
    return ref;
}

// IShellExtInit: 接收用户选中的文件
STDMETHODIMP ContextMenu::Initialize(PCIDLIST_ABSOLUTE, IDataObject* pdtobj, HKEY) {
    if (!pdtobj) return E_INVALIDARG;

    selectedFiles_.clear();

    FORMATETC fmt = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stg = {};
    if (FAILED(pdtobj->GetData(&fmt, &stg))) return E_INVALIDARG;

    HDROP hDrop = static_cast<HDROP>(GlobalLock(stg.hGlobal));
    if (!hDrop) { ReleaseStgMedium(&stg); return E_INVALIDARG; }

    UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
    for (UINT i = 0; i < count; i++) {
        wchar_t buf[MAX_PATH] = {};
        if (DragQueryFileW(hDrop, i, buf, MAX_PATH) > 0) {
            // 检查扩展名是否支持（含复合扩展名如 .tar.gz）
            std::wstring filename = std::filesystem::path(buf).filename().wstring();
            std::transform(filename.begin(), filename.end(), filename.begin(), ::towlower);
            std::wstring ext = std::filesystem::path(buf).extension().wstring();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
            bool supported = false;
            for (const auto& se : SUPPORTED_EXTENSIONS) {
                if (ext == se || (filename.size() > se.size() &&
                    filename.compare(filename.size() - se.size(), se.size(), se) == 0)) {
                    supported = true;
                    break;
                }
            }
            if (supported) {
                selectedFiles_.push_back(buf);
            }
        }
    }

    GlobalUnlock(stg.hGlobal);
    ReleaseStgMedium(&stg);

    return selectedFiles_.empty() ? E_INVALIDARG : S_OK;
}

// IContextMenu: 添加菜单项
STDMETHODIMP ContextMenu::QueryContextMenu(HMENU hmenu, UINT index,
                                            UINT idFirst, UINT, UINT uFlags) {
    if (uFlags & CMF_DEFAULTONLY) return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);

    // 至少选中一个支持的压缩文件时显示
    if (selectedFiles_.empty()) return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);

    wchar_t text[64] = {};
    LoadStringW(g_hInstance, IDS_MENU_TEXT, text, ARRAYSIZE(text));

    MENUITEMINFOW mii = {};
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
    mii.wID = idFirst;
    mii.fState = MFS_ENABLED;
    mii.dwTypeData = text;
    InsertMenuItemW(hmenu, index, TRUE, &mii);

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1); // 1个命令
}

// IContextMenu: 执行命令（支持多选批量解压）
STDMETHODIMP ContextMenu::InvokeCommand(CMINVOKECOMMANDINFO* pici) {
    if (!pici || !IS_INTRESOURCE(pici->lpVerb)) return E_INVALIDARG;
    if (LOWORD(pici->lpVerb) != 0) return E_INVALIDARG;

    if (selectedFiles_.empty()) return E_INVALIDARG;

    // 1. 查找7z.exe
    std::wstring sevenZipPath = ArchiveAnalyzer::FindSevenZipPath();
    if (sevenZipPath.empty()) {
        wchar_t title[64] = {}, msg[256] = {};
        LoadStringW(g_hInstance, IDS_ERROR_TITLE, title, ARRAYSIZE(title));
        LoadStringW(g_hInstance, IDS_ERROR_7Z_NOT_FOUND, msg, ARRAYSIZE(msg));
        MessageBoxW(pici->hwnd, msg, title, MB_ICONERROR);
        return S_OK;
    }

    // 2. 查找7zG.exe
    std::wstring sevenZipGUI = ArchiveAnalyzer::FindSevenZipGUIPath(sevenZipPath);
    if (sevenZipGUI.empty()) {
        sevenZipGUI = sevenZipPath; // 回退到7z.exe
    }

    // 3. 冲突检测：检查所有压缩包的解压目标是否存在同名文件/文件夹
    auto action = ConflictHandler::CheckConflicts(selectedFiles_, pici->hwnd);
    if (action == ConflictAction::Skip) {
        return S_OK; // 用户选择跳过
    }

    // 4. 对每个选中的压缩包执行智能解压
    for (const auto& archivePath : selectedFiles_) {
        std::filesystem::path archiveFile(archivePath);
        std::wstring destDir = archiveFile.parent_path().wstring();

        // 分析压缩包内容
        ArchiveAnalysis analysis = ArchiveAnalyzer::Analyze(archivePath, sevenZipPath);

        // 确定解压目标路径
        std::wstring extractDir;
        if (analysis.fileCount < 0) {
            extractDir = destDir; // 分析失败，解压到当前目录
        } else if (analysis.isSingleFile()) {
            extractDir = destDir; // 单文件：当前目录
        } else if (analysis.isSingleFolder()) {
            extractDir = destDir; // 单文件夹：当前目录
        } else {
            // 多文件/多目录：解压到同名子目录
            extractDir = (std::filesystem::path(destDir) / archiveFile.stem()).wstring();
        }

        // 构建7zG命令行: 7zG.exe x "archive.zip" -o"destDir" -y
        std::wstring cmdLine = L"\"" + sevenZipGUI +
                               L"\" x \"" + archivePath +
                               L"\" -o\"" + extractDir +
                               L"\" -y";

        std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
        cmdBuf.push_back(L'\0');

        STARTUPINFOW si = {sizeof(STARTUPINFOW)};
        PROCESS_INFORMATION pi = {};

        BOOL ok = CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr,
                                 FALSE, 0, nullptr, nullptr, &si, &pi);
        if (ok) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            // 不等待，让7zG自己显示进度窗口
        } else {
            wchar_t title[64] = {};
            LoadStringW(g_hInstance, IDS_ERROR_TITLE, title, ARRAYSIZE(title));
            std::wstring errMsg = L"启动7-Zip失败: " + archiveFile.filename().wstring();
            MessageBoxW(pici->hwnd, errMsg.c_str(), title, MB_ICONERROR);
        }
    }

    return S_OK;
}

// IContextMenu: 状态栏/提示文本
STDMETHODIMP ContextMenu::GetCommandString(UINT_PTR idCmd, UINT uFlags,
                                            UINT*, LPSTR pszName, UINT cchMax) {
    if (idCmd != 0) return E_INVALIDARG;

    if (uFlags == GCS_HELPTEXTW) {
        LoadStringW(g_hInstance, IDS_MENU_HELP_TEXT,
                    reinterpret_cast<LPWSTR>(pszName), cchMax);
        return S_OK;
    }
    if (uFlags == GCS_HELPTEXTA) {
        char buf[256] = {};
        LoadStringA(g_hInstance, IDS_MENU_HELP_TEXT, buf, ARRAYSIZE(buf));
        StringCchCopyNA(pszName, cchMax, buf, cchMax);
        return S_OK;
    }
    return E_INVALIDARG;
}

// ==================== ContextMenuFactory ====================

ContextMenuFactory::ContextMenuFactory() : refCount_(1) {}

STDMETHODIMP ContextMenuFactory::QueryInterface(REFIID riid, void** ppv) {
    static const QITAB qit[] = {
        QITABENT(ContextMenuFactory, IClassFactory),
        { 0 }
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) ContextMenuFactory::AddRef() { return InterlockedIncrement(&refCount_); }
STDMETHODIMP_(ULONG) ContextMenuFactory::Release() {
    LONG ref = InterlockedDecrement(&refCount_);
    if (ref == 0) delete this;
    return ref;
}

STDMETHODIMP ContextMenuFactory::CreateInstance(IUnknown* pOuter, REFIID riid, void** ppv) {
    if (pOuter) return CLASS_E_NOAGGREGATION;
    auto* obj = new (std::nothrow) ContextMenu();
    if (!obj) return E_OUTOFMEMORY;
    HRESULT hr = obj->QueryInterface(riid, ppv);
    obj->Release();
    return hr;
}

STDMETHODIMP ContextMenuFactory::LockServer(BOOL fLock) {
    if (fLock) InterlockedIncrement(&g_dllRefCount);
    else InterlockedDecrement(&g_dllRefCount);
    return S_OK;
}

} // namespace SmartExtract
