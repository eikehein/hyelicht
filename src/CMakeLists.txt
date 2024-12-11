# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>

add_compile_definitions(HYELICHT_VERSION="${PROJECT_VERSION}")
add_compile_definitions(HYELICHT_DOMAIN_NAME="${HYELICHT_DOMAIN_NAME}")

if(BUILD_ONBOARD)
    add_compile_definitions(HYELICHT_BUILD_ONBOARD)
endif()

set(hyelicht_SRCS
    animations/fireanimation.cpp
    abstractanimation.cpp
    displaycontroller.cpp
    httpserver.cpp
    ledstrip.cpp
    main.cpp
    remoteshelfmodel.cpp
    shelfmodel.cpp
)

kconfig_add_kcfg_files(hyelicht_SRCS GENERATE_MOC settings/settings.kcfgc)

ecm_qt_declare_logging_category(hyelicht_SRCS
    HEADER debug.h
    IDENTIFIER HYELICHT
    DEFAULT_SEVERITY Warning
    CATEGORY_NAME "com.hyerimandeike.hyelicht"
    DESCRIPTION "hyelicht (General)"
    EXPORT hyelicht
)

ecm_qt_declare_logging_category(hyelicht_SRCS
    HEADER debug_animations.h
    IDENTIFIER HYELICHT_ANIMATIONS
    DEFAULT_SEVERITY Warning
    CATEGORY_NAME "com.hyerimandeike.hyelicht.Animations"
    DESCRIPTION "hyelicht (Animations)"
    EXPORT hyelicht
)

ecm_qt_declare_logging_category(hyelicht_SRCS
    HEADER debug_displaycontroller.h
    IDENTIFIER HYELICHT_DISPLAYCONTROLLER
    DEFAULT_SEVERITY Warning
    CATEGORY_NAME "com.hyerimandeike.hyelicht.DisplayController"
    DESCRIPTION "hyelicht (DisplayController)"
    EXPORT hyelicht
)

ecm_qt_declare_logging_category(hyelicht_SRCS
    HEADER debug_httpserver.h
    IDENTIFIER HYELICHT_HTTPSERVER
    DEFAULT_SEVERITY Warning
    CATEGORY_NAME "com.hyerimandeike.hyelicht.HttpServer"
    DESCRIPTION "hyelicht (HttpServer)"
    EXPORT hyelicht
)

ecm_qt_declare_logging_category(hyelicht_SRCS
    HEADER debug_ledstrip.h
    IDENTIFIER HYELICHT_LEDSTRIP
    DEFAULT_SEVERITY Warning
    CATEGORY_NAME "com.hyerimandeike.hyelicht.LedStrip"
    DESCRIPTION "hyelicht (LedStrip)"
    EXPORT hyelicht
)

ecm_qt_declare_logging_category(hyelicht_SRCS
    HEADER debug_remoting.h
    IDENTIFIER HYELICHT_REMOTING
    DEFAULT_SEVERITY Warning
    CATEGORY_NAME "com.hyerimandeike.hyelicht.Remoting"
    DESCRIPTION "hyelicht (Remoting)"
    EXPORT hyelicht
)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../data/icons/*apps-hyelicht.svg")
ecm_add_app_icon(hyelicht_SRCS ICONS ${ICONS_SRCS})

qt_add_resources(RESOURCES resources.qrc)

add_executable(hyelicht ${hyelicht_SRCS} ${RESOURCES})

set(qml_singletons
    declarative/components/Onboard.qml
    declarative/components/Theme.qml
)

set_source_files_properties(${qml_singletons}
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(hyelicht
    URI
        com.hyerimandeike.hyelicht
    QML_FILES
        declarative/main.qml
        declarative/BrightnessPopup.qml
        declarative/Curtain.qml
        declarative/Gui.qml
        declarative/components/BasicButton.qml
        declarative/components/ColorButton.qml
        declarative/components/ColorWheel.qml
        declarative/components/NonVisualItem.qml
        declarative/components/Shelf.qml
        ${qml_singletons}
)

qt_add_shaders(hyelicht "hyelicht_shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "shaders/"
    BASE
        "declarative/components/shaders/"
    FILES
        "declarative/components/shaders/ColorWheel.frag"
        "declarative/components/shaders/ColorWheel.vert"
)

qt_add_repc_merged(hyelicht
    remoteshelfmodeliface.rep
)

target_compile_definitions(hyelicht PRIVATE -DUSE_QRC)

target_link_libraries(hyelicht
    PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::Quick
        Qt6::RemoteObjects
        KF6::ConfigCore
        KF6::ConfigGui
        KF6::CoreAddons
        KF6::I18n
)

if(BUILD_ONBOARD)
    target_link_libraries(hyelicht
        PRIVATE
            Qt6::HttpServer
            Qt6::SerialPort
    )
endif()

if(ANDROID)
    ecm_add_android_apk(hyelicht ANDROID_DIR ${CMAKE_SOURCE_DIR}/data/android)
endif()

install(TARGETS hyelicht ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})


# Optional `hyelichtctl` CLI frontend.
if(BUILD_CLI)
    set(hyelichtctl_SRCS
        hyelichtctl.cpp
    )

    ecm_qt_declare_logging_category(hyelichtctl_SRCS
        HEADER debug_hyelichtctl.h
        IDENTIFIER HYELICHTCTL
        DEFAULT_SEVERITY Warning
        CATEGORY_NAME "com.hyerimandeike.hyelichtctl"
        DESCRIPTION "hyelichtctl"
        EXPORT hyelicht
    )

    add_executable(hyelichtctl ${hyelichtctl_SRCS})

    target_link_libraries(hyelichtctl
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        KF6::CoreAddons
        KF6::I18n
    )

    install(TARGETS hyelichtctl ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
endif()

# Must come last to collect the `HYELICHTCTL` category.
ecm_qt_install_logging_categories(
    EXPORT hyelicht
    FILE hyelicht.categories
    DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}"
)
