/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

pragma Singleton

import QtQuick

//! Singleton providing UI colors and fonts, and helper functions
/*!
 * \ingroup GUI
 *
 */
QtObject {
    //! Hyerim and Eike "brand color" (from our wedding invitation card).
    /*!
    * Defaults to \c \#003d68.
    */
    readonly property color brandColor: "#003d68"

    //! Window background color.
    /*!
    * Defaults to \c white.
    */
    readonly property color windowBackgroundColor: "white"

    //! Window foreground color (e.g. text).
    /*!
    * Defaults to \c black.
    */
    readonly property color windowForegroundColor: "black"

    //! Button color when active or checked.
    /*!
    * Defaults to \c \#098deb.
    */
    readonly property color activeButtonColor: "#098deb"

    //! Button color when not active or checked.
    /*!
    * Defaults to \c \#e8e8e8.
    */
    readonly property color inactiveButtonColor: "#e8e8e8"

    //! Font family used throughout the UI.
    /*!
    * Defaults to \c Open \c Sans.
    */
    readonly property string fontFamily: "Open Sans"

    //! Rounds a given number to the nearest even number.
    function even(number) {
        return 2 * Math.round(number / 2);
    }

    //! Returns a rounded-to-nearest-even fraction of an object's width or height, depending on which is bigger.
    function proportion(item, fraction) {
        return Math.min(even(item.width * fraction), even(item.height * fraction));
    }
}
