# SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
# SPDX-License-Identifier: GPL-3.0-or-later

project(plasma-camera)

cmake_minimum_required(VERSION 2.8.12)
set(KF_MIN_VERSION "5.65.0")
set(QT_MIN_VERSION "5.14.0")

include(FeatureSummary)

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(ECMPoQmTools)
include(KDECompilerSettings NO_POLICY_SCOPE)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Core Quick Test Gui Svg QuickControls2)

if(ANDROID AND QT_MAJOR_VERSION EQUAL "5")
    find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS AndroidExtras)
endif()
if(NOT ANDROID)
    find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED COMPONENTS Widgets)
endif()

find_package(KF${QT_MAJOR_VERSION} ${KF_MIN_VERSION} REQUIRED COMPONENTS Kirigami2 CoreAddons I18n Config)

add_subdirectory(src)

ki18n_install(po)

install(PROGRAMS org.kde.plasma.camera.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.plasma.camera.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
