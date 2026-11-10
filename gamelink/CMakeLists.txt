cmake_minimum_required(VERSION 3.20)
project(ToolKitLink LANGUAGES C CXX)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "ToolKitLink must be built for Win32 (-A Win32): scarface.exe is a 32-bit process.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(SCARFACE_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "Folder that contains the SDK folder")
if(NOT EXISTS "${SCARFACE_SDK_ROOT}/SDK/.SDK.hh")
    message(FATAL_ERROR "Scarface SDK not found: set SCARFACE_SDK_ROOT to the folder that contains SDK/.SDK.hh.")
endif()

set(MINHOOK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/minhook)
add_library(minhook STATIC
    ${MINHOOK_DIR}/src/buffer.c ${MINHOOK_DIR}/src/hook.c ${MINHOOK_DIR}/src/trampoline.c ${MINHOOK_DIR}/src/hde32.c)
target_include_directories(minhook PUBLIC ${MINHOOK_DIR}/include)
target_compile_options(minhook PRIVATE /W0)

add_library(ToolKitLink SHARED LinkMain.cc LinkServer.cc LiveFrontend.cc)
set_target_properties(ToolKitLink PROPERTIES SUFFIX ".asi")
target_include_directories(ToolKitLink PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_include_directories(ToolKitLink SYSTEM PRIVATE ${SCARFACE_SDK_ROOT})
target_compile_options(ToolKitLink PRIVATE /W4 /permissive- /utf-8 /external:anglebrackets /external:W0)
target_compile_definitions(ToolKitLink PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN DIRECTINPUT_VERSION=0x0800)
target_link_libraries(ToolKitLink PRIVATE minhook)
