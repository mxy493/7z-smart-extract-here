add_executable(test_analyzer test_analyzer.cpp ${CMAKE_SOURCE_DIR}/src/ArchiveAnalyzer.cpp)
set_target_properties(test_analyzer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
target_include_directories(test_analyzer PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_analyzer PRIVATE advapi32 shell32)
add_test(NAME test_analyzer COMMAND test_analyzer)
