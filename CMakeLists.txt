# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>

cmake_minimum_required(VERSION 3.25)

find_package(ECM 6.1.0 REQUIRED NO_MODULE)

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)
set(QT_MIN_VERSION "6.8.0")
set(KF6_MIN_VERSION "6.8.0")

project(Hyelicht
    VERSION 2.0
    DESCRIPTION "Controller application for the Hyelicht shelf. Paint on the shelf \
                 with colors, turn on the fireplace mode, and more.")
string(TOLOWER ${PROJECT_NAME} PROJECT_LOWERCASE_NAME)
set(HYELICHT_DOMAIN_NAME "com.hyerimandeike.${PROJECT_LOWERCASE_NAME}")

set(CMAKE_AUTOMOC ON)
set(AUTOMOC_MOC_OPTIONS -Muri=${HYELICHT_DOMAIN_NAME})
set(CMAKE_AUTORCC ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(CMakePackageConfigHelpers)
include(FeatureSummary)
include(ECMAddAppIcon)
include(ECMInstallIcons)
include(ECMFindQmlModule)
include(ECMQtDeclareLoggingCategory)
include(GNUInstallDirs)
include(KDEInstallDirs)
include(ECMConfiguredInstall)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)

# For convenience, default BUILD_ONBOARD and BUILD_CLI to off on Android.
if(ANDROID)
    set(HYELICHT_OPTIONAL_FEATURES_DEFAULT FALSE)
else()
    set(HYELICHT_OPTIONAL_FEATURES_DEFAULT TRUE)
endif()

set(BUILD_CLI_HELP "Build the `hyelichtctl` CLI frontend to the HTTP REST API.")
option(BUILD_CLI ${BUILD_CLI_HELP} ${HYELICHT_OPTIONAL_FEATURES_DEFAULT})
add_feature_info(BUILD_CLI BUILD_CLI ${BUILD_CLI_HELP})

set(BUILD_DOCS_HELP "Generate documentation using Doxygen.")
option(BUILD_DOCS ${BUILD_DOCS_HELP} FALSE)
add_feature_info(BUILD_DOCS BUILD_DOCS ${BUILD_DOCS_HELP})

set(BUILD_ONBOARD_HELP "Build onboard features and set onboard behavior defaults.")
option(BUILD_ONBOARD ${BUILD_ONBOARD_HELP} ${HYELICHT_OPTIONAL_FEATURES_DEFAULT})
add_feature_info(BUILD_ONBOARD BUILD_ONBOARD ${BUILD_ONBOARD_HELP})

set(CLANG_TIDY_HELP "Enable clang-tidy during the build process.")
option(CLANG_TIDY ${CLANG_TIDY_HELP} FALSE)
add_feature_info(CLANG_TIDY CLANG_TIDY ${CLANG_TIDY_HELP})

set(DEBUG_QML_HELP "Enable QML debugger/profiler support.")
option(DEBUG_QML ${DEBUG_QML_HELP} FALSE)
add_feature_info(DEBUG_QML DEBUG_QML ${DEBUG_QML_HELP})

if(BUILD_ONBOARD)
    set(qtOnboardComponents
	    "HttpServer"
        "SerialPort"
    )
endif()

if(ANDROID)
    include(ECMAddAndroidApk)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/android/version.gradle.in ${CMAKE_BINARY_DIR}/version.gradle)
endif()

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS
    Core
    Network
    Qml
    Quick
    RemoteObjects
    ShaderTools
    ${qtOnboardComponents}
    ${qtAndroidComponents}
)

find_package(KF6 ${KF6_MIN_VERSION} REQUIRED
    Config
    CoreAddons
    I18n
)

if(BUILD_DOCS)
    find_package(Doxygen 1.9.8 REQUIRED dot)

    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    endif()
endif()

ecm_find_qmlmodule(QtQml REQUIRED)
ecm_find_qmlmodule(QtQuick.Controls REQUIRED)
ecm_find_qmlmodule(QtQuick.Effects REQUIRED)
ecm_find_qmlmodule(QtQuick.Layouts REQUIRED)
ecm_find_qmlmodule(QtQuick.Window REQUIRED)

if(CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*;-fix")
endif()

if(DEBUG_QML)
    message(STATUS "To enable the QML debugger/profiler, run with: '-qmljsdebugger=port:1234'")
    add_definitions(-DQT_QML_DEBUG)
endif()

add_definitions(
  -DQT_USE_QSTRINGBUILDER
  -DQT_NO_CAST_TO_ASCII
  -DQT_STRICT_ITERATORS
  -DQT_NO_URL_CAST_FROM_STRING
  -DQT_NO_CAST_FROM_BYTEARRAY
  -DQT_NO_SIGNALS_SLOTS_KEYWORDS
  -DQT_USE_FAST_OPERATOR_PLUS
)

# New in Qt 6.5, see https://doc.qt.io/qt-6/qt-cmake-policy-qtp0001.html
qt_policy(SET QTP0001 NEW)

# New in Qt 6.8, see https://doc.qt.io/qt-6/qt-cmake-policy-qtp0004.html
qt_policy(SET QTP0004 NEW)

add_subdirectory(data)
add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
