cmake_minimum_required(VERSION 3.16)
project(rt_aspa C)

set(CMAKE_C_STANDARD 99)

# Include public headers directory
include_directories(include)

# Build the core RT-ASPA static DSP library
add_library(rt_aspa_lib STATIC
    src/rt_aspa_window.c
    src/rt_aspa_fft.c
    src/rt_aspa_pitch.c
)

# Set execution source file (Default to desktop test harness if not specified)
if(NOT EXEC_SRC)
    set(EXEC_SRC "${CMAKE_CURRENT_SOURCE_DIR}/main/main_desktop.c")
endif()

add_executable(rt_aspa_demo ${EXEC_SRC})
target_link_libraries(rt_aspa_demo PRIVATE rt_aspa_lib)