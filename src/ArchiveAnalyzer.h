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
