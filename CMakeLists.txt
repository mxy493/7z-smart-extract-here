cmake_minimum_required(VERSION 3.15)
project(7zSmartExtractHere VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Output DLL to dist directory for easy distribution
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_compile_options(/utf-8 /DNOMINMAX)

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
    src/exports.def
)

add_library(7zSmartExtractHere SHARED ${SOURCES} ${HEADERS})

target_include_directories(7zSmartExtractHere PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(7zSmartExtractHere PRIVATE
    ole32
    shell32
    shlwapi
    user32
    advapi32
)

target_compile_definitions(7zSmartExtractHere PRIVATE
    UNICODE
    _UNICODE
)

enable_testing()
add_subdirectory(tests)
