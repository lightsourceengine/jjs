include ("${CMAKE_CURRENT_LIST_DIR}/zig.cmake")

set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_FLAGS "-target aarch64-macos-none")
set(FLAGS_COMMON_ARCH "${CMAKE_C_FLAGS}")
