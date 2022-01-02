# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>

add_subdirectory(icons)

if (NOT ANDROID)
    if(BUILD_ONBOARD)
        add_subdirectory(service)
    endif()

    configure_file(com.hyerimandeike.hyelicht.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/${HYELICHT_DOMAIN_NAME}.desktop)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${HYELICHT_DOMAIN_NAME}.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})

    configure_file(com.hyerimandeike.hyelicht.appdata.xml.in ${CMAKE_CURRENT_BINARY_DIR}/${HYELICHT_DOMAIN_NAME}.appdata.xml)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${HYELICHT_DOMAIN_NAME}.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
endif()
