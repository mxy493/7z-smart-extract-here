# Smart Extract Here

一个 Windows Shell 扩展 DLL，为压缩文件添加"Smart Extract Here"右键菜单选项。

## 功能

- 右键菜单集成，支持 `.zip`、`.rar`、`.7z`、`.tar.gz`、`.tgz` 等[多种压缩格式](src/Registry.h)
- 智能解压：单文件/单文件夹 → 直接解压到当前目录；多个文件 → 解压到以压缩包命名的子文件夹
- 调用 7zG.exe 执行解压，带原生进度对话框
- 文件冲突检测与处理（覆盖 / 跳过）

## 前置条件

- Windows 10 / 11
- 已安装 [7-Zip](https://www.7-zip.org/)
- Visual Studio 2022（需要 MSVC C++ 桌面开发工作负载）

## 构建

运行 `build.bat`（自动配置 MSVC + Ninja 环境）：

```cmd
build.bat
```

或手动使用 CMake：

```cmd
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

DLL 输出到 `dist\7zSmartExtractHere.dll`。

### 编译器标志

| 标志 | 用途 |
|------|------|
| `/utf-8` | 源文件使用 UTF-8 编码 |
| `/DNOMINMAX` | 避免 Windows `min/max` 宏与 `<algorithm>` 冲突 |
| `/DUNICODE /D_UNICODE` | 使用 Unicode Win32 API |
| `/std:c++17` | 启用 C++17（`<filesystem>` 等） |

## 运行测试

```cmd
build_test.bat
```

或手动编译运行：

```cmd
cl /nologo /EHsc /std:c++17 /utf-8 /DNOMINMAX /I"src" /DUNICODE /D_UNICODE ^
  tests\test_analyzer.cpp src\ArchiveAnalyzer.cpp ^
  /link shell32.lib ole32.lib shlwapi.lib advapi32.lib ^
  /out:test_analyzer.exe
test_analyzer.exe
```

## 安装

以管理员身份运行：

```cmd
dist\install.bat
```

或手动注册：

```cmd
regsvr32 "path\to\7zSmartExtractHere.dll"
```

## 卸载

以管理员身份运行：

```cmd
dist\uninstall.bat
```

或手动注销：

```cmd
regsvr32 /u "path\to\7zSmartExtractHere.dll"
```

## 分发

`dist/` 目录包含分享所需的全部文件：

```
dist/
  7zSmartExtractHere.dll  # Shell 扩展 DLL
  install.bat             # 注册脚本（需管理员权限）
  uninstall.bat           # 卸载脚本（需管理员权限）
```

打包 `dist/` 文件夹即可分享给他人使用。

## 项目结构

```
dist/                     # 分发目录（可直接分享）
  7zSmartExtractHere.dll  编译后的 DLL
  install.bat             注册脚本（需管理员权限）
  uninstall.bat           卸载脚本（需管理员权限）
src/
  ArchiveAnalyzer.h/cpp   压缩包分析（解析 7z l 输出）
  ConflictHandler.h/cpp   文件冲突检测与处理
  ContextMenu.h/cpp       COM Shell 扩展实现（IShellExtInit, IContextMenu）
  Registry.h/cpp          注册表操作（查找 7-Zip、注册/注销 Shell 扩展）
  main.cpp                DllMain / DllGetClassObject / DllRegisterServer / DllUnregisterServer
  Resource.h / Resource.rc 字符串表、版本信息
  exports.def             DLL 导出定义，供 regsvr32 使用
tests/
  test_analyzer.cpp       ArchiveAnalyzer 单元测试
build/                    # 构建中间文件（已 gitignore）
```
