cmake_minimum_required(VERSION 3.19)
project(QVibra LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS, ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-fexceptions)

# Try to find the best available Qt6 version
set(QT_PREFERRED_VERSION "6.10.0")
set(QT_FALLBACK_VERSION "6.5.0")
set(QT_MINIMUM_VERSION "6.2.0")
set(KF_MIN_VERSION "6.0.0")

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets LinguistTools)

qt_standard_project_setup()

qt_add_executable(QVibra
    WIN32 MACOSX_BUNDLE
    main.cpp
    settingsdialog.cpp
    settingsdialog.h
    settingsdialog.ui
)

qt_add_translations(
    TARGETS QVibra
    TS_FILES QVibra_en_GB.ts
)

qt6_add_resources(QVibra "resources"
    PREFIX "/"
    FILES resources/app.ico
)

target_link_libraries(QVibra
    PRIVATE
        Qt::Core
        Qt::Widgets
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
