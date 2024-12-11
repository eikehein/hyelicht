# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>

set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

set(DOXYGEN_PROJECT_NAME "${PROJECT_NAME}")
set(DOXYGEN_PROJECT_LOGO ${PROJECT_SOURCE_DIR}/data/icons/sc-apps-hyelicht.svg)

set(DOXYGEN_FILE_PATTERNS
    "*.h"
    "*.cpp"
    "*.qml"
    "*.md")

# Enable Doxyqml to extract documentation from QML source files.
set(DOXYGEN_FILTER_PATTERNS "*.qml=doxyqml")
set(DOXYGEN_EXTENSION_MAPPING "qml=C++")

# Eliminate some false positives and skip unnecessary parsing.
set(DOXYGEN_EXCLUDE_PATTERNS
    "LICENSES"
    "data"
    "support"
    "assets"
    "main.qml"
    ${PROJECT_SOURCE_DIR}/README.md)

# Hide the internal KConfigXt-generated `SettingsHelper` class.
# TODO: Enable once the problem with the `Settings` class
# mentioned below is resolved - it will then become necessary,
# as `SettingsHelper` resides in the same files.
# set(DOXYGEN_EXCLUDE_SYMBOLS "SettingsHelper")

# Prepare the project README.md for Doxygen consumption by generating
# header id attributes for cross-linking and the table-of-contents.
# We don't have them statically in the file because some Markdown
# renderers don't understand them, e.g. GitHub's.
file(READ ${PROJECT_SOURCE_DIR}/README.md README)
string(REGEX REPLACE "([#]+) ([^\n-]+)" "\\1 \\2 {#\\2}" README ${README})
while(${README} MATCHES "{[^\n]+([ ]+|[A-Z][^\n ]+)[^\n]+}\n")
    string(TOLOWER ${CMAKE_MATCH_0} MATCH)
    string(REPLACE "(" "" MATCH ${MATCH})
    string(REPLACE ")" "" MATCH ${MATCH})
    string(REPLACE " " "-" MATCH ${MATCH})
    string(REPLACE ${CMAKE_MATCH_0} ${MATCH} README ${README})
endwhile()

# Add the special `[TOC]` command to the README.md to enable Doxygen's
# table of contents. We don't have it statically in the file because
# some Markdown renderers don't understand and then display it, e.g.
# GitHub's.
string(REGEX REPLACE "SPDX[^\n]+\n-->\n" "-->\n[TOC]" README ${README})

# Set the pre-processed project README.md as the Doxygen main page and
# copy over the graphical assets used inside the document.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/README.md ${README})
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_CURRENT_BINARY_DIR}/README.md)
set(DOXYGEN_HTML_EXTRA_STYLESHEET ${PROJECT_SOURCE_DIR}/docs/doxygen_custom.css)
configure_file(${PROJECT_SOURCE_DIR}/data/icons/sc-apps-hyelicht.svg
    ${CMAKE_CURRENT_BINARY_DIR}/html/docs/data/icons COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/docs/screenshot.png
    ${CMAKE_CURRENT_BINARY_DIR}/html/docs COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/docs/screenshot_phone.png
    ${CMAKE_CURRENT_BINARY_DIR}/html/docs COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/docs/system_diagram.png
    ${CMAKE_CURRENT_BINARY_DIR}/html/docs COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/docs/wiring_diagram.png
    ${CMAKE_CURRENT_BINARY_DIR}/html/docs COPYONLY)

doxygen_add_docs(${PROJECT_NAME}_docs
    ${PROJECT_SOURCE_DIR}
    # TODO: Enable once KConfigXT no longer generates code that produces
    # Doxygen warnings.
    # ${CMAKE_CURRENT_BINARY_DIR}/src/settings.h
    # ${CMAKE_CURRENT_BINARY_DIR}/src/settings.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/README.md
    ALL
    COMMENT "Generate documentation.")
