# SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)
set(KF_MIN_VERSION "6.0.0")
set(QT_MIN_VERSION "6.6.0")
set(PROJECT_VERSION "2.1.1")

project(plasma-camera VERSION ${PROJECT_VERSION})

include(FeatureSummary)

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMConfiguredInstall)
include(ECMPoQmTools)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEGitCommitHooks)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEClangFormat)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX PLASMACAMERA
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS
    Core
    Quick
    Test
    Gui
    Svg
    QuickControls2
    Sensors
)

if(NOT ANDROID)
    find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Widgets)
endif()

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS Kirigami CoreAddons I18n Config)

find_package(LibExiv2 REQUIRED)

add_subdirectory(src)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

ki18n_install(po)

install(PROGRAMS org.kde.plasma.camera.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.plasma.camera.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
