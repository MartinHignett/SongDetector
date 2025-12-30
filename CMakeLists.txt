cmake_minimum_required(VERSION 3.19)
project(QVibra LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-fexceptions)

# Try to find the best available Qt6 version
set(QT_PREFERRED_VERSION "6.10.0")
set(QT_FALLBACK_VERSION "6.4.0")
set(QT_MINIMUM_VERSION "6.2.0")
set(KF_MIN_VERSION "6.0.0")

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Widgets LinguistTools Multimedia)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)

qt_standard_project_setup()

qt_add_executable(QVibra
    WIN32 MACOSX_BUNDLE
    audiostream.cpp
    audiostream.h
    main.cpp
    settingsdialog.cpp
    settingsdialog.h
    settingsdialog.ui
    shazam_body.cpp
    shazam_body.h
)

qt_add_translations(
    TARGETS QVibra
    TS_FILES translations/QVibra_en_GB.ts
)

qt6_add_resources(QVibra "resources"
    PREFIX "/"
    FILES resources/app.ico
)

# Find Vibra library and headers
find_path(VIBRA_INCLUDE_DIR
    NAMES vibra.h
    PATHS vibra/include
    REQUIRED
)

find_library(VIBRA_LIBRARY
    NAMES vibra_shared
    PATHS vibra/build/lib vibra/build
    REQUIRED
)

if(VIBRA_INCLUDE_DIR AND VIBRA_LIBRARY)
    message(STATUS "Found Vibra library: ${VIBRA_LIBRARY}")
    message(STATUS "Found Vibra headers: ${VIBRA_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "Could not find Vibra library or headers")
endif()

add_library(Vibra SHARED IMPORTED)
set_target_properties(Vibra
    PROPERTIES
        IMPORTED_LOCATION ${VIBRA_LIBRARY}
)

target_include_directories(QVibra PRIVATE ${VIBRA_INCLUDE_DIR})

target_link_libraries(QVibra
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
        PkgConfig::PIPEWIRE
        Vibra
)

include(GNUInstallDirs)

install(TARGETS QVibra
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET QVibra
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
