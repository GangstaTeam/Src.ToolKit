cmake_minimum_required(VERSION 3.20)
project(ToolKit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
endif()

file(GLOB TK_P3D_SOURCES CONFIGURE_DEPENDS src/core/*.cc src/p3d/*.cc)
add_library(tk_p3d STATIC ${TK_P3D_SOURCES})
target_include_directories(tk_p3d PUBLIC src)

file(GLOB TK_RCF_SOURCES CONFIGURE_DEPENDS src/rcf/*.cc)
add_library(tk_rcf STATIC ${TK_RCF_SOURCES})
target_link_libraries(tk_rcf PUBLIC tk_p3d)

file(GLOB TK_NIS_SOURCES CONFIGURE_DEPENDS src/nis/*.cc)
add_library(tk_nis STATIC ${TK_NIS_SOURCES})
target_link_libraries(tk_nis PUBLIC tk_p3d)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_dx9.cpp ${IMGUI_DIR}/imgui_impl_win32.cpp)
target_include_directories(imgui PUBLIC ${IMGUI_DIR})
if(MSVC)
    target_compile_options(imgui PRIVATE /W0)
endif()

set(TK_RIG_RENDER_SOURCES src/render/Math3d.cc src/render/ImageDecoder.cc src/render/ImageEncoder.cc)
file(GLOB TK_RIG_SOURCES CONFIGURE_DEPENDS src/rig/*.cc)
add_library(tk_rig STATIC ${TK_RIG_SOURCES} ${TK_RIG_RENDER_SOURCES})
target_link_libraries(tk_rig PUBLIC tk_p3d windowscodecs ole32)

file(GLOB TK_APP_SOURCES CONFIGURE_DEPENDS src/app/*.cc src/doc/*.cc src/ui/*.cc src/render/*.cc src/fe/*.cc src/link/*.cc)
list(REMOVE_ITEM TK_APP_SOURCES ${CMAKE_SOURCE_DIR}/src/render/Math3d.cc ${CMAKE_SOURCE_DIR}/src/render/ImageDecoder.cc
     ${CMAKE_SOURCE_DIR}/src/render/ImageEncoder.cc)
add_executable(ToolKit WIN32 ${TK_APP_SOURCES} res/toolkit.rc)
target_link_libraries(ToolKit PRIVATE tk_rig tk_rcf tk_nis imgui d3d9 dwmapi comdlg32 shell32)
add_custom_command(TARGET ToolKit POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/schema $<TARGET_FILE_DIR:ToolKit>/schema)

if(EXISTS ${CMAKE_SOURCE_DIR}/tools/CMakeLists.txt)
    add_subdirectory(tools)
endif()
