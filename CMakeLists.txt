cmake_minimum_required(VERSION 3.16)
project(devgraph LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

add_executable(devgraph
    src/main.cpp
    src/util.cpp
    src/device_node.cpp
    src/sysfs_scanner.cpp
    src/devicetree.cpp
    src/devnode_index.cpp
    src/filter.cpp
    src/render_tree.cpp
    src/render_list.cpp
    src/render_dot.cpp
)

target_include_directories(devgraph PRIVATE src)

install(TARGETS devgraph RUNTIME DESTINATION bin)
