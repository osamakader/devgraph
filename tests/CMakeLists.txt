add_executable(devgraph_tests
    test_main.cpp
    fixture.cpp
    util_test.cpp
    devnode_index_test.cpp
    hwid_db_test.cpp
    filter_test.cpp
    render_test.cpp
    sysfs_scanner_test.cpp
)

target_link_libraries(devgraph_tests PRIVATE devgraph_core)
target_include_directories(devgraph_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(NAME devgraph_tests COMMAND devgraph_tests)
