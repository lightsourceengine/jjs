include ("${CMAKE_CURRENT_LIST_DIR}/zig.cmake")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86)

set(CMAKE_C_FLAGS "-target x86-linux-gnu.2.17")
set(FLAGS_COMMON_ARCH "${CMAKE_C_FLAGS}")