#include "ConflictHandler.h"
#include "ArchiveAnalyzer.h"
#include "Resource.h"
#include <windows.h>
#include <filesystem>
#include <sstream>

namespace SmartExtract {

extern HINSTANCE g_hInstance;

std::pair<std::wstring, bool> ConflictHandler::ComputeExtractDir(const std::wstring& archivePath) {
    std::filesystem::path archiveFile(archivePath);
    std::wstring parentDir = archiveFile.parent_path().wstring();

    // 需要分析压缩包内容来决定目标路径
    std::wstring sevenZipPath = ArchiveAnalyzer::FindSevenZipPath();
    if (sevenZipPath.empty()) {
        return {parentDir, false}; // 找不到7z，回退到当前目录，不检测冲突
    }

    ArchiveAnalysis analysis = ArchiveAnalyzer::Analyze(archivePath, sevenZipPath);

    if (analysis.fileCount < 0) {
        return {parentDir, false}; // 分析失败，不检测冲突
    } else if (analysis.isSingleFile()) {
        return {parentDir, false}; // 单文件：当前目录，不检测冲突
    } else if (analysis.isSingleFolder()) {
        return {parentDir, false}; // 单文件夹：当前目录，不检测冲突
    } else {
        // 多文件/多目录：同名子目录，需要检测冲突
        return {(std::filesystem::path(parentDir) / archiveFile.stem()).wstring(), true};
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
        auto [extractDir, needsCheck] = ComputeExtractDir(archive);
        if (needsCheck && TargetExists(extractDir)) {
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
