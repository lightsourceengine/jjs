include ("${CMAKE_CURRENT_LIST_DIR}/zig.cmake")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm7l)

set(CMAKE_C_FLAGS "-target arm-linux-gnueabihf -mthumb -mfpu=vfp -mcpu=cortex_a7")
set(FLAGS_COMMON_ARCH "${CMAKE_C_FLAGS}")
