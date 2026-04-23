#include <cassert>
#include <iostream>
#include <string>
#include <fstream>
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

void test_tgz_analysis() {
    std::wstring sevenZip = ArchiveAnalyzer::FindSevenZipPath();
    if (sevenZip.empty()) {
        std::cout << "test_tgz_analysis: SKIP (7z not found)" << std::endl;
        return;
    }

    // 使用无空格的临时目录
    std::wstring testDir = L"C:\\temp\\test_tgz_analysis";
    std::filesystem::create_directories(testDir);
    std::wstring testFolder = testDir + L"\\mytestfolder";
    std::filesystem::create_directories(testFolder);
    {
        std::ofstream ofs(testFolder + L"\\test.txt");
        ofs << "hello";
    }

    std::wstring tarPath = testDir + L"\\mytest.tar";
    std::wstring tgzPath = testDir + L"\\mytest.tgz";

    // 7z a mytest.tar mytestfolder (在 testDir 下执行)
    {
        std::wstring cmd = L"cmd /c \"cd /d \"" + testDir + L"\" && \"" + sevenZip +
                          L"\" a mytest.tar mytestfolder\"";
        int r = _wsystem(cmd.c_str());
        assert(r == 0);
    }

    // 7z a mytest.tgz mytest.tar
    {
        std::wstring cmd = L"cmd /c \"cd /d \"" + testDir + L"\" && \"" + sevenZip +
                          L"\" a mytest.tgz mytest.tar\"";
        int r = _wsystem(cmd.c_str());
        assert(r == 0);
    }

    // 分析 .tgz — 只看外层（gzip 包裹一个 .tar），不需要递归解压内部 tar
    ArchiveAnalysis a = ArchiveAnalyzer::Analyze(tgzPath, sevenZip);
    std::wcout << L"  tgz analysis: fileCount=" << a.fileCount
               << L" dirCount=" << a.dirCount
               << L" topName=" << a.topName << std::endl;
    std::cout << "  isSingleFile=" << a.isSingleFile()
              << " isSingleFolder=" << a.isSingleFolder() << std::endl;

    // 预期: 外层显示单个 mytest.tar 文件 → isSingleFile=true
    // 7zG 会自动递归解压 gzip→tar→文件，无需在分析阶段解压整个 tgz
    if (!a.isSingleFile()) {
        std::cerr << "  FAIL: expected isSingleFile=true" << std::endl;
        std::cerr << "  fileCount=" << a.fileCount << " dirCount=" << a.dirCount << std::endl;
        // 清理
        std::filesystem::remove(tarPath);
        std::filesystem::remove(tgzPath);
        std::filesystem::remove_all(testFolder);
        std::exit(1);
    }

    // 清理
    std::filesystem::remove(tarPath);
    std::filesystem::remove(tgzPath);
    std::filesystem::remove_all(testFolder);

    std::cout << "test_tgz_analysis: PASS" << std::endl;
}

void test_multi_entry_tar() {
    std::wstring sevenZip = ArchiveAnalyzer::FindSevenZipPath();
    if (sevenZip.empty()) {
        std::cout << "test_multi_entry_tar: SKIP (7z not found)" << std::endl;
        return;
    }

    // 创建包含多个顶层条目的目录结构
    std::wstring testDir = L"C:\\temp\\test_multi_entry_tar";
    std::filesystem::create_directories(testDir);

    // 多个子目录 + 文件
    std::filesystem::create_directories(testDir + L"\\mca_log");
    {
        std::ofstream ofs(testDir + L"\\mca_log\\test.log");
        ofs << "mca data";
    }
    std::filesystem::create_directories(testDir + L"\\charge_log");
    {
        std::ofstream ofs(testDir + L"\\charge_log\\test.log");
        ofs << "charge data";
    }
    {
        std::ofstream ofs(testDir + L"\\data.csv");
        ofs << "csv data";
    }

    std::wstring tarPath = testDir + L"\\multi_entry.tar";

    // 7z a multi_entry.tar mca_log charge_log data.csv
    {
        std::wstring cmd = L"cmd /c \"cd /d \"" + testDir + L"\" && \"" + sevenZip +
                          L"\" a multi_entry.tar mca_log charge_log data.csv\"";
        int r = _wsystem(cmd.c_str());
        assert(r == 0);
    }

    ArchiveAnalysis a = ArchiveAnalyzer::Analyze(tarPath, sevenZip);
    std::wcout << L"  multi_entry tar analysis: fileCount=" << a.fileCount
               << L" dirCount=" << a.dirCount
               << L" topName=[" << a.topName << L"]" << std::endl;
    std::cout << "  isSingleFile=" << a.isSingleFile()
              << " isSingleFolder=" << a.isSingleFolder() << std::endl;

    // 多个顶层条目：不应视为单文件夹
    assert(!a.isSingleFile());
    assert(!a.isSingleFolder());

    // 清理
    std::filesystem::remove(tarPath);
    std::filesystem::remove_all(testDir);

    std::cout << "test_multi_entry_tar: PASS" << std::endl;
}

int main() {
    std::cout << "Running tests..." << std::endl;
    test_find_7z();
    test_find_7zg();
    test_tgz_analysis();
    test_multi_entry_tar();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
