#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "ArchiveAnalyzer.h"

#include <shellapi.h>
#include <sstream>
#include <set>

namespace SmartExtract {

// 将UTF-8字符串转换为宽字符串
// 7z使用 -sccUTF-8 参数后输出UTF-8编码
static std::wstring ConvertUTF8ToWide(const std::string& str) {
    if (str.empty()) return L"";

    int wlen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (wlen > 0) {
        std::wstring wname(wlen, L'\0');
        if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wname.data(), wlen) > 0) {
            wname.pop_back(); // 移除末尾的null terminator
            return wname;
        }
    }

    return L"";
}

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

        // 移除末尾的 \r（7z输出可能是 \r\n 或 \r\r\n）
        while (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
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
            // 使用UTF-8转换（7z使用 -sccUTF-8 参数输出）
            std::wstring wname = ConvertUTF8ToWide(name);
            if (!wname.empty()) {
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

// 执行 7z l 命令并返回输出
static std::string Run7zList(const std::wstring& sevenZipPath, const std::wstring& archivePath) {
    std::wstring cmdLine = L"\"" + sevenZipPath + L"\" l -sccUTF-8 \"" + archivePath + L"\"";
    HANDLE hReadPipe = nullptr, hWritePipe = nullptr;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) return "";

    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = {sizeof(STARTUPINFOW)};
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    PROCESS_INFORMATION pi = {};
    std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back(L'\0');

    BOOL success = CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr,
                                  TRUE, CREATE_NO_WINDOW, nullptr, nullptr,
                                  &si, &pi);
    if (!success) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return "";
    }

    CloseHandle(hWritePipe);

    std::string output;
    char buffer[4096];
    DWORD bytesRead;
    while (ReadFile(hReadPipe, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0) {
        output.append(buffer, bytesRead);
    }

    CloseHandle(hReadPipe);
    WaitForSingleObject(pi.hProcess, 10000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return output;
}

ArchiveAnalysis ArchiveAnalyzer::Analyze(const std::wstring& archivePath,
                                          const std::wstring& sevenZipPath) {
    ArchiveAnalysis result;

    // 先列出压缩包内容
    std::string output = Run7zList(sevenZipPath, archivePath);
    if (output.empty()) {
        result.fileCount = -1;
        return result;
    }

    // 对 compound 格式（.tar.gz/.tgz 等），7z l 只显示外层（如单个 .tar）。
    // 不需要递归分析内部 tar——外层结果已经足够判断解压策略：
    //   - 外层通常显示一个 .tar 文件 → isSingleFile() → 解压到当前目录
    //   - 7zG x "file.tgz" 会自动递归解压（gzip→tar→文件），用户得到正确结果
    // 递归分析内部 tar 需要解压整个 gzip 流，对于大文件（几GB）会极慢甚至卡死。

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
