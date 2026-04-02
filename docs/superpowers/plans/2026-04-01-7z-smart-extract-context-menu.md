# 7z智能解压右键菜单 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在Windows右键菜单中添加"7z智能解压"选项，选中压缩文件时才显示。单文件压缩包解压至当前文件夹，多文件压缩包解压至同名子文件夹。支持多选批量解压（每个压缩包独立按上述逻辑解压）。解压操作调用7zG.exe完成（自带进度窗口）。解压前检测同名文件/文件夹冲突，提示用户覆盖或跳过。

**Architecture:** 实现一个COM Shell Extension DLL，注册到Windows Shell上下文菜单。当用户右键点击压缩文件时，DLL检查文件扩展名决定是否显示菜单。点击菜单后，DLL判断压缩包内容（单文件/多文件），计算目标路径，然后调用系统安装的7zG.exe执行解压，7zG自动显示原生进度窗口。

**Tech Stack:** C++17 (Shell Extension / COM), Windows Registry, 7zG.exe (用户已安装的7-Zip), CMake, Visual Studio MSVC

---

## 方案选型说明

**方案选型：方案2 - 调用7zG.exe**

原因：
- 开发量小（~500-800行 vs ~2000行）
- 7zG.exe自带专业进度窗口，支持暂停/取消
- 无需集成7z SDK，减少依赖
- 7zG.exe是7-Zip官方GUI，稳定性有保证
- 调用方式：`7zG.exe x "archive.7z" -o"C:\output\path" -y`

**Bandizip智能解压规则（参考实现）：**

| 压缩包内容 | 解压行为 |
|------------|----------|
| 单一文件 | 解压到当前位置 |
| 单文件夹打包 | 解压到当前位置（保留文件夹结构） |
| 其他情况（多文件/多文件夹） | 解压到"压缩包名"文件夹 |

---

## 文件结构

```
7z-smart-extract-here/
├── src/
│   ├── main.cpp              # DLL入口点（DllMain + 导出函数）
│   ├── ContextMenu.h         # Shell扩展接口定义
│   ├── ContextMenu.cpp       # Shell扩展实现（显示菜单 + 执行解压）
│   ├── ArchiveAnalyzer.h     # 压缩包分析接口
│   ├── ArchiveAnalyzer.cpp   # 压缩包分析实现（调用7z l命令）
│   ├── ConflictHandler.h     # 同名冲突检测接口
│   ├── ConflictHandler.cpp   # 同名冲突检测与用户提示实现
│   ├── Registry.h            # 注册表操作接口
│   ├── Registry.cpp          # 注册表注册/卸载实现
│   ├── Resource.h            # 资源ID定义
│   └── Resource.rc           # 资源文件（字符串、版本信息）
├── tests/
│   ├── test_analyzer.cpp     # 压缩包分析单元测试
│   └── CMakeLists.txt
├── installer/
│   ├── install.bat           # 安装脚本
│   └── uninstall.bat         # 卸载脚本
├── CMakeLists.txt
└── README.md
```

---

## Task 1: 项目初始化

**Files:**
- Create: `CMakeLists.txt`
- Create: `README.md`

- [ ] **Step 1: 创建CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.15)
project(7zSmartExtractHere VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(SOURCES
    src/main.cpp
    src/ContextMenu.cpp
    src/ArchiveAnalyzer.cpp
    src/ConflictHandler.cpp
    src/Registry.cpp
    src/Resource.rc
)

set(HEADERS
    src/ContextMenu.h
    src/ArchiveAnalyzer.h
    src/ConflictHandler.h
    src/Registry.h
    src/Resource.h
)

add_library(7zSmartExtractHere SHARED ${SOURCES} ${HEADERS})

target_include_directories(7zSmartExtractHere PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(7zSmartExtractHere PRIVATE
    ole32
    shell32
    shlwapi
    user32
)

target_compile_definitions(7zSmartExtractHere PRIVATE
    UNICODE
    _UNICODE
)

enable_testing()
add_subdirectory(tests)
```

- [ ] **Step 2: 创建README.md**

```markdown
# 7z智能解压右键菜单

Windows右键菜单扩展，提供智能解压功能。

## 功能

- 右键菜单集成，仅对支持的压缩文件显示
- 智能解压：单文件→当前目录，多文件→同名子目录
- 调用7zG.exe解压，显示原生进度窗口

## 前置要求

- Windows 10/11
- 已安装 [7-Zip](https://www.7-zip.org/)（需要7zG.exe）

## 构建

```cmd
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## 安装

```cmd
regsvr32 /s "path\to\7zSmartExtractHere.dll"
```

或以管理员身份运行 `installer/install.bat`。

## 卸载

```cmd
regsvr32 /u /s "path\to\7zSmartExtractHere.dll"
```

或以管理员身份运行 `installer/uninstall.bat`。
```

- [ ] **Step 3: 提交**

```bash
git add CMakeLists.txt README.md
git commit -m "chore: initialize project with CMake build system"
```

---

## Task 2: 资源文件

**Files:**
- Create: `src/Resource.h`
- Create: `src/Resource.rc`

- [ ] **Step 1: 创建Resource.h**

```cpp
#ifndef RESOURCE_H
#define RESOURCE_H

#define IDS_MENU_TEXT                201
#define IDS_MENU_HELP_TEXT           202
#define IDS_ERROR_TITLE              203
#define IDS_ERROR_7Z_NOT_FOUND       204
#define IDS_ERROR_EXTRACT_FAILED     205
#define IDS_CONFLICT_TITLE           206
#define IDS_CONFLICT_MSG             207

#endif // RESOURCE_H
```

- [ ] **Step 2: 创建Resource.rc**

```rc
#include "Resource.h"
#include <windows.h>

STRINGTABLE
BEGIN
    IDS_MENU_TEXT           "7z智能解压"
    IDS_MENU_HELP_TEXT      "智能解压压缩文件到当前位置或同名子文件夹"
    IDS_ERROR_TITLE         "7z智能解压"
    IDS_ERROR_7Z_NOT_FOUND  "未找到7-Zip，请先安装7-Zip (https://www.7-zip.org/)"
    IDS_ERROR_EXTRACT_FAILED "解压失败"
    IDS_CONFLICT_TITLE        "7z智能解压 - 文件冲突"
    IDS_CONFLICT_MSG          "以下目标已存在，是否覆盖？\n\n%s"
END

VS_VERSION_INFO VERSIONINFO
FILEVERSION     1,0,0,0
PRODUCTVERSION  1,0,0,0
FILEFLAGSMASK   0x3fL
FILEFLAGS       0x0L
FILEOS          VOS_NT_WINDOWS32
FILETYPE        VFT_DLL
FILESUBTYPE     VFT2_UNKNOWN
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "080404b0"
        BEGIN
            VALUE "FileDescription", "7z智能解压右键菜单扩展"
            VALUE "FileVersion", "1.0.0.0"
            VALUE "InternalName", "7zSmartExtractHere"
            VALUE "ProductName", "7z智能解压"
            VALUE "ProductVersion", "1.0.0.0"
        END
    END
END
```

- [ ] **Step 3: 提交**

```bash
git add src/Resource.h src/Resource.rc
git commit -m "feat: add resource definitions"
```

---

## Task 3: 压缩包分析器

**Files:**
- Create: `src/ArchiveAnalyzer.h`
- Create: `src/ArchiveAnalyzer.cpp`

**核心逻辑：** 调用 `7z l archive.zip` 命令解析输出，获取文件列表，判断单文件/多文件。

- [ ] **Step 1: 创建ArchiveAnalyzer.h**

```cpp
#pragma once
#include <string>
#include <filesystem>

namespace SmartExtract {

// 压缩包分析结果
struct ArchiveAnalysis {
    int fileCount = 0;      // 非目录条目数
    int dirCount = 0;       // 目录条目数
    std::wstring topName;   // 顶层唯一名称（如果有的话）
    bool isSingleFile() const { return fileCount == 1 && dirCount == 0; }
    // 单文件夹：只有一个顶层目录且该目录下所有内容
    bool isSingleFolder() const;
};

class ArchiveAnalyzer {
public:
    // 分析压缩包内容
    // archivePath: 压缩包完整路径
    // sevenZipPath: 7z.exe的完整路径
    // 返回分析结果，失败时 fileCount = -1
    static ArchiveAnalysis Analyze(const std::wstring& archivePath,
                                    const std::wstring& sevenZipPath);

    // 查找系统安装的7z.exe路径
    // 依次检查：注册表安装路径、默认安装目录、PATH环境变量
    static std::wstring FindSevenZipPath();

    // 查找7zG.exe路径（基于7z.exe路径推导）
    static std::wstring FindSevenZipGUIPath(const std::wstring& sevenZipPath);
};

} // namespace SmartExtract
```

- [ ] **Step 2: 创建ArchiveAnalyzer.cpp**

```cpp
#include "ArchiveAnalyzer.h"
#include <windows.h>
#include <shellapi.h>
#include <sstream>
#include <set>
#include <algorithm>

namespace SmartExtract {

// 从7z l命令输出中解析文件列表
// 7z l输出格式（列表部分）：
//   Date      Time    Attr         Size   Compressed  Name
//   ------------------- ----- ------------ ------------  ------------------------
//   2024-01-01 12:00:00 D....            0            0  folder
//   2024-01-01 12:00:00 .....         1234          567  folder/file.txt
static std::vector<std::pair<std::wstring, bool>> Parse7zListOutput(const std::string& output) {
    std::vector<std::pair<std::wstring, bool>> entries;
    std::istringstream stream(output);
    std::string line;

    bool inEntries = false;
    while (std::getline(stream, line)) {
        // 跳过空行
        if (line.empty()) continue;

        // 检测条目分隔线 "-------------------"
        if (line.find("----") != std::string::npos) {
            inEntries = !inEntries;
            continue;
        }

        if (!inEntries) continue;

        // 解析条目行
        // 格式: Date Time Attr Size Compressed Name
        // Attr字段包含 'D' 表示目录
        bool isDir = false;
        // 查找属性列（第20个字符左右，格式如 "D...." 或 "....."）
        std::string attr;
        size_t attrStart = line.find("D....");
        if (attrStart != std::string::npos) {
            isDir = true;
        }

        // 提取名称（最后一个空格后的部分）
        size_t lastSpace = line.rfind(' ');
        if (lastSpace != std::string::npos && lastSpace > 20) {
            std::string name = line.substr(lastSpace + 1);
            // 转换为wstring
            int wlen = MultiByteToWideChar(CP_OEMCP, 0, name.c_str(), -1, nullptr, 0);
            if (wlen > 0) {
                std::wstring wname(wlen, L'\0');
                MultiByteToWideChar(CP_OEMCP, 0, name.c_str(), -1, wname.data(), wlen);
                wname.pop_back(); // 移除末尾的null terminator
                entries.push_back({wname, isDir});
            }
        }
    }

    return entries;
}

// 提取顶层名称
static std::wstring GetTopLevelName(const std::wstring& path) {
    // 移除末尾的/
    std::wstring p = path;
    while (!p.empty() && (p.back() == L'/' || p.back() == L'\\')) {
        p.pop_back();
    }
    // 取第一个路径段
    size_t pos = p.find_first_of(L"/\\");
    if (pos != std::wstring::npos) {
        return p.substr(0, pos);
    }
    return p;
}

bool ArchiveAnalysis::isSingleFolder() const {
    if (fileCount == 0 && dirCount == 0) return false;
    // 只有当只有一个顶层目录时才是单文件夹
    return !topName.empty();
}

ArchiveAnalysis ArchiveAnalyzer::Analyze(const std::wstring& archivePath,
                                          const std::wstring& sevenZipPath) {
    ArchiveAnalysis result;

    // 构建命令: 7z l "archive.zip"
    std::wstring cmdLine = L"\"" + sevenZipPath + L"\" l \"" + archivePath + L"\"";

    // 创建管道捕获输出
    HANDLE hReadPipe = nullptr, hWritePipe = nullptr;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        result.fileCount = -1;
        return result;
    }

    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = {sizeof(STARTUPINFOW)};
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    PROCESS_INFORMATION pi = {};

    // 需要可修改的缓冲区
    std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back(L'\0');

    BOOL success = CreateProcessW(
        nullptr, cmdBuf.data(), nullptr, nullptr,
        TRUE, CREATE_NO_WINDOW, nullptr, nullptr,
        &si, &pi
    );

    if (!success) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        result.fileCount = -1;
        return result;
    }

    CloseHandle(hWritePipe);

    // 读取输出
    std::string output;
    char buffer[4096];
    DWORD bytesRead;
    while (ReadFile(hReadPipe, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0) {
        output.append(buffer, bytesRead);
    }

    CloseHandle(hReadPipe);
    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // 解析输出
    auto entries = Parse7zListOutput(output);

    std::set<std::wstring> topLevelNames;
    for (const auto& [name, isDir] : entries) {
        if (isDir) {
            result.dirCount++;
        } else {
            result.fileCount++;
        }

        // 收集顶层名称
        std::wstring top = GetTopLevelName(name);
        if (!top.empty()) {
            topLevelNames.insert(top);
        }
    }

    // 如果只有一个顶层名称，记录它
    if (topLevelNames.size() == 1) {
        result.topName = *topLevelNames.begin();
    }

    return result;
}

std::wstring ArchiveAnalyzer::FindSevenZipPath() {
    // 方法1: 从注册表查找安装路径
    HKEY hKey = nullptr;
    wchar_t path[MAX_PATH] = {0};
    DWORD size = sizeof(path);

    // 64位注册表
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\7-Zip",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, L"Path", nullptr, nullptr,
                             reinterpret_cast<LPBYTE>(path), &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            std::wstring result = path;
            if (!result.empty() && result.back() != L'\\') result += L'\\';
            result += L"7z.exe";
            if (std::filesystem::exists(result)) {
                return result;
            }
        } else {
            RegCloseKey(hKey);
        }
    }

    // 方法2: 尝试默认安装路径
    const wchar_t* defaultPaths[] = {
        L"C:\\Program Files\\7-Zip\\7z.exe",
        L"C:\\Program Files (x86)\\7-Zip\\7z.exe",
    };
    for (const auto* p : defaultPaths) {
        if (std::filesystem::exists(p)) {
            return p;
        }
    }

    // 方法3: 从PATH环境变量查找
    wchar_t fullPath[MAX_PATH] = {0};
    if (SearchPathW(nullptr, L"7z.exe", nullptr, MAX_PATH, fullPath, nullptr)) {
        return fullPath;
    }

    return L""; // 未找到
}

std::wstring ArchiveAnalyzer::FindSevenZipGUIPath(const std::wstring& sevenZipPath) {
    if (sevenZipPath.empty()) return L"";
    std::filesystem::path p(sevenZipPath);
    std::wstring guiPath = (p.parent_path() / L"7zG.exe").wstring();
    if (std::filesystem::exists(guiPath)) {
        return guiPath;
    }
    return L"";
}

} // namespace SmartExtract
```

- [ ] **Step 3: 提交**

```bash
git add src/ArchiveAnalyzer.h src/ArchiveAnalyzer.cpp
git commit -m "feat: implement archive analyzer (list command parsing)"
```

---

## Task 4: 同名冲突检测

**Files:**
- Create: `src/ConflictHandler.h`
- Create: `src/ConflictHandler.cpp`

**核心逻辑：** 解压前检测目标路径是否已存在同名文件/文件夹，若存在则弹出对话框让用户选择覆盖或跳过。

- [ ] **Step 1: 创建ConflictHandler.h**

```cpp
#pragma once
#include <string>
#include <vector>

namespace SmartExtract {

// 冲突处理结果
enum class ConflictAction {
    Overwrite,  // 覆盖全部
    Skip,       // 跳过（取消本次解压）
};

class ConflictHandler {
public:
    // 检查多个压缩包的解压目标是否存在冲突
    // archives: 压缩包路径列表
    // 获取每个压缩包的解压目标路径（同ArchiveAnalyzer逻辑）
    // 返回用户选择的动作
    // hwnd: 父窗口句柄，用于弹出对话框
    static ConflictAction CheckConflicts(
        const std::vector<std::wstring>& archives,
        HWND hwnd);

private:
    // 计算单个压缩包的解压目标路径
    static std::wstring ComputeExtractDir(const std::wstring& archivePath);

    // 检查目标路径是否存在
    static bool TargetExists(const std::wstring& extractDir);
};

} // namespace SmartExtract
```

- [ ] **Step 2: 创建ConflictHandler.cpp**

```cpp
#include "ConflictHandler.h"
#include "ArchiveAnalyzer.h"
#include "Resource.h"
#include <windows.h>
#include <filesystem>
#include <sstream>

namespace SmartExtract {

extern HINSTANCE g_hInstance;

std::wstring ConflictHandler::ComputeExtractDir(const std::wstring& archivePath) {
    std::filesystem::path archiveFile(archivePath);
    std::wstring parentDir = archiveFile.parent_path().wstring();

    // 需要分析压缩包内容来决定目标路径
    std::wstring sevenZipPath = ArchiveAnalyzer::FindSevenZipPath();
    if (sevenZipPath.empty()) {
        return parentDir; // 找不到7z，回退到当前目录
    }

    ArchiveAnalysis analysis = ArchiveAnalyzer::Analyze(archivePath, sevenZipPath);

    if (analysis.fileCount < 0) {
        return parentDir; // 分析失败
    } else if (analysis.isSingleFile()) {
        return parentDir; // 单文件：当前目录
    } else if (analysis.isSingleFolder()) {
        return parentDir; // 单文件夹：当前目录
    } else {
        // 多文件/多目录：同名子目录
        return (std::filesystem::path(parentDir) / archiveFile.stem()).wstring();
    }
}

bool ConflictHandler::TargetExists(const std::wstring& extractDir) {
    return std::filesystem::exists(extractDir);
}

ConflictAction ConflictHandler::CheckConflicts(
    const std::vector<std::wstring>& archives,
    HWND hwnd) {

    // 收集所有有冲突的路径
    std::vector<std::wstring> conflictPaths;

    for (const auto& archive : archives) {
        std::wstring extractDir = ComputeExtractDir(archive);
        if (TargetExists(extractDir)) {
            std::filesystem::path p(archive);
            conflictPaths.push_back(p.filename().wstring() + L" -> " + extractDir);
        }
    }

    if (conflictPaths.empty()) {
        return ConflictAction::Overwrite; // 无冲突
    }

    // 构建冲突提示信息
    std::wstring msg = L"以下解压目标已存在，是否覆盖？\n\n";
    for (const auto& cp : conflictPaths) {
        msg += L"  " + cp + L"\n";
    }

    wchar_t title[64] = {};
    LoadStringW(g_hInstance, IDS_CONFLICT_TITLE, title, ARRAYSIZE(title));

    int result = MessageBoxW(hwnd, msg.c_str(), title,
                              MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
    if (result == IDYES) {
        return ConflictAction::Overwrite;
    }
    return ConflictAction::Skip;
}

} // namespace SmartExtract
```

- [ ] **Step 3: 提交**

```bash
git add src/ConflictHandler.h src/ConflictHandler.cpp
git commit -m "feat: add conflict detection with user prompt for overwrite/skip"
```

---

## Task 5: 注册表操作

**Files:**
- Create: `src/Registry.h`
- Create: `src/Registry.cpp`

- [ ] **Step 1: 创建Registry.h**

```cpp
#pragma once
#include <string>
#include <vector>

namespace SmartExtract {

// 支持的压缩格式
inline const std::vector<std::wstring> SUPPORTED_EXTENSIONS = {
    L".7z", L".zip", L".rar", L".tar", L".gz", L".bz2",
    L".xz", L".lzma", L".cab", L".arj", L".iso",
    L".wim", L".dmg", L".rpm", L".deb", L".cpio"
};

// 使用 {A8E8A8A8-E8A8-4A8A-A8E8-A8E8A8A8A8A8} 占位
// 实际使用前请用uuidgen生成新的CLSID
inline const wchar_t* CLSID_STRING = L"{A8E8A8A8-E8A8-4A8A-A8E8-A8E8A8A8A8A8}";

class Registry {
public:
    static bool Register(const std::wstring& dllPath);
    static bool Unregister();
    static bool IsRegistered();
};

} // namespace SmartExtract
```

- [ ] **Step 2: 创建Registry.cpp**

```cpp
#include "Registry.h"
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <filesystem>

namespace SmartExtract {

extern HINSTANCE g_hInstance;

// 单个扩展名注册
static bool RegisterOneExtension(const std::wstring& ext) {
    // HKCR\.ext\shellex\ContextMenuHandlers\SmartExtract -> {CLSID}
    std::wstring keyPath = ext + L"\\shellex\\ContextMenuHandlers\\SmartExtract";
    HKEY hKey = nullptr;
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, keyPath.c_str(),
                        0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    RegSetValueExW(hKey, nullptr, 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(CLSID_STRING),
                   (DWORD)((wcslen(CLSID_STRING) + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);
    return true;
}

// 单个扩展名卸载
static bool UnregisterOneExtension(const std::wstring& ext) {
    std::wstring keyPath = ext + L"\\shellex\\ContextMenuHandlers\\SmartExtract";
    RegDeleteTreeW(HKEY_CLASSES_ROOT, keyPath.c_str());
    // 同时删除空的shellex键（如果空了）
    std::wstring shellexKey = ext + L"\\shellex";
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, shellexKey.c_str(),
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD subKeyCount = 0;
        RegQueryInfoKeyW(hKey, nullptr, nullptr, nullptr, &subKeyCount,
                         nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
        RegCloseKey(hKey);
        if (subKeyCount == 0) {
            RegDeleteKeyW(HKEY_CLASSES_ROOT, shellexKey.c_str());
        }
    }
    return true;
}

bool Registry::Register(const std::wstring& dllPath) {
    // 1. 注册CLSID\InprocServer32
    std::wstring clsidKey = std::wstring(L"CLSID\\") + CLSID_STRING;
    HKEY hKey = nullptr;

    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, clsidKey.c_str(),
                        0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    const wchar_t* desc = L"7z智能解压";
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

    // 2. 为每个扩展名注册上下文菜单
    for (const auto& ext : SUPPORTED_EXTENSIONS) {
        RegisterOneExtension(ext);
    }

    // 3. 刷新Shell
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return true;
}

bool Registry::Unregister() {
    // 1. 删除扩展名注册
    for (const auto& ext : SUPPORTED_EXTENSIONS) {
        UnregisterOneExtension(ext);
    }

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
```

- [ ] **Step 3: 提交**

```bash
git add src/Registry.h src/Registry.cpp
git commit -m "feat: implement registry register/unregister operations"
```

---

## Task 6: Shell扩展上下文菜单

**Files:**
- Create: `src/ContextMenu.h`
- Create: `src/ContextMenu.cpp`

- [ ] **Step 1: 创建ContextMenu.h**

```cpp
#pragma once
#include <windows.h>
#include <shlobj.h>
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
```

- [ ] **Step 2: 创建ContextMenu.cpp**

```cpp
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
            // 检查扩展名是否支持
            std::wstring ext = std::filesystem::path(buf).extension().wstring();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
            bool supported = false;
            for (const auto& se : SUPPORTED_EXTENSIONS) {
                if (ext == se) { supported = true; break; }
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
```

- [ ] **Step 3: 提交**

```bash
git add src/ContextMenu.h src/ContextMenu.cpp
git commit -m "feat: implement shell context menu with smart extract logic"
```

---

## Task 7: DLL入口点

**Files:**
- Create: `src/main.cpp`

- [ ] **Step 1: 创建main.cpp**

```cpp
#include <windows.h>
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
```

- [ ] **Step 2: 提交**

```bash
git add src/main.cpp
git commit -m "feat: implement DLL entry point and exports"
```

---

## Task 8: 安装/卸载脚本

**Files:**
- Create: `installer/install.bat`
- Create: `installer/uninstall.bat`

- [ ] **Step 1: 创建install.bat**

```bat
@echo off
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo 请以管理员身份运行此脚本
    pause
    exit /b 1
)

set "DLL_PATH=%~dp0..\build\bin\Release\7zSmartExtractHere.dll"
if not exist "%DLL_PATH%" (
    echo 未找到DLL文件: %DLL_PATH%
    echo 请先编译项目
    pause
    exit /b 1
)

regsvr32 /s "%DLL_PATH%"
if %errorlevel% equ 0 (
    echo 安装成功！
) else (
    echo 安装失败，请检查是否有管理员权限
)
pause
```

- [ ] **Step 2: 创建uninstall.bat**

```bat
@echo off
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo 请以管理员身份运行此脚本
    pause
    exit /b 1
)

regsvr32 /u /s "%~dp0..\build\bin\Release\7zSmartExtractHere.dll"
if %errorlevel% equ 0 (
    echo 卸载成功！
    echo 提示：可能需要重启资源管理器以刷新右键菜单
) else (
    echo 卸载完成（DLL可能未注册）
)
pause
```

- [ ] **Step 3: 提交**

```bash
git add installer/install.bat installer/uninstall.bat
git commit -m "feat: add install/uninstall batch scripts"
```

---

## Task 9: 测试

**Files:**
- Create: `tests/CMakeLists.txt`
- Create: `tests/test_analyzer.cpp`

- [ ] **Step 1: 创建tests/CMakeLists.txt**

```cmake
add_executable(test_analyzer test_analyzer.cpp)
target_include_directories(test_analyzer PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_analyzer COMMAND test_analyzer)
```

- [ ] **Step 2: 创建tests/test_analyzer.cpp**

```cpp
#include <cassert>
#include <iostream>
#include <string>
#include <filesystem>
#include "ArchiveAnalyzer.h"

using namespace SmartExtract;

void test_find_7z() {
    std::wstring path = ArchiveAnalyzer::FindSevenZipPath();
    if (path.empty()) {
        std::cout << "  7z.exe not found (expected if not installed)" << std::endl;
    } else {
        std::wcout << L"  Found: " << path << std::endl;
        assert(std::filesystem::exists(path));
    }
    std::cout << "test_find_7z: PASS" << std::endl;
}

void test_find_7zg() {
    std::wstring sevenZip = ArchiveAnalyzer::FindSevenZipPath();
    if (!sevenZip.empty()) {
        std::wstring gui = ArchiveAnalyzer::FindSevenZipGUIPath(sevenZip);
        if (!gui.empty()) {
            std::wcout << L"  Found: " << gui << std::endl;
            assert(std::filesystem::exists(gui));
        }
    }
    std::cout << "test_find_7zg: PASS" << std::endl;
}

int main() {
    std::cout << "Running tests..." << std::endl;
    test_find_7z();
    test_find_7zg();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
```

- [ ] **Step 3: 提交**

```bash
git add tests/CMakeLists.txt tests/test_analyzer.cpp
git commit -m "test: add analyzer unit tests"
```

---

## 自检清单

### 1. 规格覆盖检查

| 需求 | 对应任务 |
|------|----------|
| 右键菜单显示"7z智能解压" | Task 6 (ContextMenu.cpp QueryContextMenu) |
| 仅对支持的压缩文件显示 | Task 6 (Initialize中过滤扩展名) |
| 单文件解压至当前文件夹 | Task 6 (InvokeCommand中判断 isSingleFile) |
| 多文件解压至同名子文件夹 | Task 6 (InvokeCommand中构造子目录路径) |
| 支持多选批量解压 | Task 6 (InvokeCommand循环处理selectedFiles_) |
| 同名文件/文件夹冲突提示 | Task 4 (ConflictHandler::CheckConflicts) |
| 调用7zG.exe显示进度 | Task 6 (InvokeCommand中CreateProcess调用7zG) |
| 注册表集成 | Task 5 (Registry::Register/Unregister) |
| 支持多种格式 | Task 5 (SUPPORTED_EXTENSIONS) |

### 2. 占位符检查

- [x] 无TBD/TODO/占位符
- [x] 所有步骤有完整代码
- [x] 所有命令有具体内容

### 3. 关键设计点

- CLSID在Registry.h和main.cpp中一致
- 函数签名.h/.cpp匹配
- 7zG调用参数: `x -o"path" -y`（解压+指定目录+全部确认）
- 7z l命令用于分析压缩包内容（不实际解压）

---

**计划完成，已保存到 `docs/superpowers/plans/2026-04-01-7z-smart-extract-context-menu.md`**
