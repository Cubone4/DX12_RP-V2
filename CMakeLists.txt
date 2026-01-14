cmake_minimum_required(VERSION 3.20)
project(GraphicsTesting LANGUAGES CXX)

file(GLOB SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
)

add_executable(GraphicsTesting WIN32
    ${SOURCES}
)

set_target_properties(GraphicsTesting PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
)

find_path(WINSDK_INCLUDE_DIR DirectXMath.h
    PATHS
        "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/Include"
    PATH_SUFFIXES
        10.0.26100.0/um
        10.0.26100.0/shared
        10.0.26100.0/ucrt
        10.0.26100.0/winrt
    NO_DEFAULT_PATH
)

target_include_directories(GraphicsTesting PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    "${WINSDK_INCLUDE_DIR}/.."
)

target_link_libraries(GraphicsTesting PRIVATE
    d3d12
    dxgi
    dxguid
    d3dcompiler
)
