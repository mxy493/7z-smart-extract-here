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
