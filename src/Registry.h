#pragma once
#include <string>
#include <vector>

namespace SmartExtract {

// 支持的压缩格式
inline const std::vector<std::wstring> SUPPORTED_EXTENSIONS = {
    L".7z", L".zip", L".rar", L".tar", L".gz", L".bz2",
    L".xz", L".lzma", L".cab", L".arj", L".iso",
    L".wim", L".dmg", L".rpm", L".deb", L".cpio",
    L".tgz", L".tbz2", L".txz", L".tar.gz", L".tar.bz2", L".tar.xz"
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
