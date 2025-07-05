cmake_minimum_required(VERSION 3.21)

# Set C++ standard before project declaration
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(BluetoothAudioReceiver)

# Set vcpkg toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")

# Find packages
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Windows-specific libraries
set(WINDOWS_LIBS
    windowsapp
    ole32
    oleaut32
    propsys
    mmdevapi
    avrt
    ksuser
    winmm
    setupapi
    cfgmgr32
    shell32
    comctl32
    user32
    gdi32
    advapi32
    d3d11
    dxgi
    d3dcompiler
)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Source files - all implementation files
set(SOURCES
    src/main.cpp
    src/Application.cpp
    src/Core/AudioManager.cpp
    src/Core/BluetoothSinkService.cpp
    src/Core/AudioMixer.cpp
    src/Core/ConfigManager.cpp
    src/Platform/WindowsAPI.cpp
    src/UI/ImGuiApp.cpp
    src/Utils/Logger.cpp
    src/Utils/StringHelpers.cpp
    src/Utils/Threading.cpp
)

# Header files
set(HEADERS
    src/Application.h
    src/Core/AudioManager.h
    src/Core/BluetoothSinkService.h
    src/Core/AudioMixer.h
    src/Core/ConfigManager.h
    src/Models/AppConfig.h
    src/Models/AudioDevice.h
    src/Models/BluetoothDevice.h
    src/Models/ConnectedAudioSource.h
    src/Models/MixingState.h
    src/Platform/WindowsAPI.h
    src/UI/ImGuiApp.h
    src/Utils/Logger.h
    src/Utils/StringHelpers.h
    src/Utils/Threading.h
)

# Create executable as a Windows application (not console)
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

# Add application icon
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.ico")
    target_sources(${PROJECT_NAME} PRIVATE resources/app.rc)
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    src
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    spdlog::spdlog
    fmt::fmt
    nlohmann_json::nlohmann_json
    imgui::imgui
    ${WINDOWS_LIBS}
)

# Compiler-specific options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE 
        /W4 
        /permissive-
        /Zc:__cplusplus
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
        WINVER=0x0A00
        _WIN32_WINNT=0x0A00
    )
    
    # Set subsystem to Windows
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
endif()

# Set target properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
) 
