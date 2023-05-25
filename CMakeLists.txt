# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.22)

set(PROJECT_ROOT_NODE ${CMAKE_CURRENT_SOURCE_DIR})

set(VCPKG_ROOT "/home/software/vcpkg")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
CACHE STRING "Vcpkg toolchain file")

project ("PhigrosCrowAPI")

# 包含子项目。
add_subdirectory(Phi)
add_subdirectory(ProxyLoadBalancingAPI)