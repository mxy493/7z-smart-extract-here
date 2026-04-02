#pragma once
#define NOMINMAX
#include <windows.h>
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
