# CLAUDE.md

本文件为 Claude Code 在本仓库中工作时提供指引。

## 项目概述

一个 Windows Shell 扩展 DLL，为压缩文件右键菜单添加"Smart Extract Here"选项。通过 `7z l` 解析压缩包内容来决定解压策略：单文件/单文件夹直接解压到当前目录，多个顶层条目则解压到以压缩包命名的子文件夹中。最终调用 `7zG.exe` 执行解压并显示进度对话框。

## 构建命令

完整构建（CMake + Ninja + MSVC，构建后自动运行测试）：

```cmd
build.bat
```

快速编译测试（不走 CMake，直接编译 test_analyzer）：

```cmd
build_test.bat
```

仅运行 CTest（需先完成构建）：

```cmd
ctest --test-dir build --output-on-failure
```

运行单个测试：

```cmd
build\bin\test_analyzer.exe
```

清理重建：删除 `build/` 和 `dist/7zSmartExtractHere.dll`，然后执行 `build.bat`。

## 安装 / 卸载

需要管理员权限，通过 `regsvr32` 注册 DLL。

```cmd
dist\install.bat      # 卸载旧版并注册，需管理员权限
```

## 架构

所有代码在 `src/` 下，使用 `SmartExtract` 命名空间。

**COM 入口** — `src/main.cpp` 实现四个标准 DLL 导出函数（`DllGetClassObject`、`DllCanUnloadNow`、`DllRegisterServer`、`DllUnregisterServer`）。CLSID 在此定义，必须与 `Registry.h` 中的 `CLSID_STRING` 保持一致。

**ContextMenu** — `src/ContextMenu.h/cpp` 实现 `IShellExtInit` 和 `IContextMenu`。`Initialize` 获取选中文件路径，`InvokeCommand` 编排完整流程：查找 7-Zip → 分析压缩包 → 决定解压策略 → 启动 7zG.exe。`ContextMenuFactory` 是 Windows 用于实例化扩展的 `IClassFactory`。

**ArchiveAnalyzer** — `src/ArchiveAnalyzer.h/cpp` 解析 `7z l` 输出，统计文件/目录数量，判断压缩包是否只有一个顶层条目。`FindSevenZipPath()` 依次检查注册表安装路径、默认安装目录、PATH 环境变量。`FindSevenZipGUIPath()` 基于 `7z.exe` 路径推导 `7zG.exe` 位置。

**ConflictHandler** — `src/ConflictHandler.h/cpp` 检测解压时是否会覆盖已有文件，并提示用户选择覆盖或跳过。

**Registry** — `src/Registry.h/cpp` 负责写入/删除 `HKCR\CLSID\{...}` 下的 COM 注册键和 Shell 扩展键。`SUPPORTED_EXTENSIONS` 列出右键菜单出现的所有压缩格式扩展名。

## 构建细节

- C++17，MSVC，Ninja 生成器
- 编译器标志：`/utf-8 /DNOMINMAX /DUNICODE /D_UNICODE`
- DLL 输出到 `dist/`（而非 `build/bin/`）
- 测试二进制文件在 `build/bin/`（已 gitignore）
- 链接的系统库：`ole32 shell32 shlwapi user32 advapi32`
- `exports.def` 声明了 regsvr32 所需的四个私有 DLL 导出
