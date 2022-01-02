# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>

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

if(COMPILE_QML)
    qtquick_compiler_add_resources(RESOURCES resources.qrc)
else()
    qt5_add_resources(RESOURCES resources.qrc)
endif()

add_executable(hyelicht ${hyelicht_SRCS} ${RESOURCES})

target_compile_definitions(hyelicht PRIVATE -DUSE_QRC)

target_link_libraries(hyelicht
    Qt5::Core
    Qt5::Qml
    Qt5::Quick
    Qt5::RemoteObjects
    KF5::ConfigCore
    KF5::ConfigGui
    KF5::CoreAddons
    KF5::I18n
)

if(BUILD_ONBOARD)
    target_link_libraries(hyelicht
        Qt5::SerialPort
        qhttpengine
    )
endif()

if(ANDROID)
    target_link_libraries(hyelicht
        Qt5::AndroidExtras
        Qt5::Svg # For icon rendering.
    )
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
        Qt5::Core
        Qt5::Gui
        Qt5::Network
        KF5::CoreAddons
        KF5::I18n
    )

    install(TARGETS hyelichtctl ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
endif()

# Must come last to collect the `HYELICHTCTL` category.
ecm_qt_install_logging_categories(
    EXPORT hyelicht
    FILE hyelicht.categories
    DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}"
)
