cmake_minimum_required(VERSION 3.19)
project(SongDetector LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-fexceptions)

# Try to find the best available Qt6 version
set(QT_PREFERRED_VERSION "6.10.0")
set(QT_FALLBACK_VERSION "6.9.0")
set(QT_MINIMUM_VERSION "6.7.0")
set(KF_MIN_VERSION "6.0.0")

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Widgets LinguistTools Multimedia)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)

# Find FFTW3 library (required by vibra)
find_library(FFTW3_LIBRARY NAMES fftw3)
find_path(FFTW3_INCLUDE_DIR fftw3.h)
if(NOT FFTW3_LIBRARY OR NOT FFTW3_INCLUDE_DIR)
    message(FATAL_ERROR "FFTW3 library not found. Please install libfftw3-dev")
endif()

qt_standard_project_setup()

qt_add_executable(SongDetector
    WIN32 MACOSX_BUNDLE
    audiostream.cpp
    audiostream.h
    main.cpp
    settingsdialog.cpp
    settingsdialog.h
    settingsdialog.ui
    shazam_body.cpp
    shazam_body.h
    song.h
    song.cpp
    song_detector.h
    song_detector.cpp
)

qt_add_translations(SongDetector
    TS_FILES translations/SongDetector_en_GB.ts
)

qt6_add_resources(SongDetector "resources"
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
    NAMES vibra libvibra.so libvibra.a
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

target_include_directories(SongDetector PRIVATE ${VIBRA_INCLUDE_DIR})

target_link_libraries(SongDetector
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
        PkgConfig::PIPEWIRE
        Vibra
        ${FFTW3_LIBRARY}
)

include(GNUInstallDirs)

install(TARGETS SongDetector
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

message(STATUS "Found Qt6: ${Qt6Core_VERSION}")

# GitHub has an older version of Qt on ubuntu-latest,
# so this workaround allows configuration on
# Qt version 6.5 (GitHub) vs 6.5 (My laptop)
if(Qt6Core_VERSION VERSION_GREATER_EQUAL "6.5")
    # Newer syntax for your Ubuntu 25.10
    qt_generate_deploy_app_script(
        TARGET SongDetector
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
elseif(COMMAND qt_generate_deploy_app_script)
    # Older Qt6 syntax
    qt_generate_deploy_app_script(
        TARGET SongDetector
        FILENAME_VARIABLE deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
else()
    message(STATUS "Qt deployment script not available in this Qt version")
endif()
