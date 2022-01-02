/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>
 */

pragma Singleton

import QtQuick 2.15

import com.hyerimandeike.hyelicht 1.0

//! Singleton providing UI margins to use when the UI in onboard mode
/*!
 * \ingroup GUI
 *
 */
QtObject {
    readonly property int leftMargin: Startup.onboard ? 94 : 0
    readonly property int rightMargin: Startup.onboard ? 102 : 0

    readonly property int topMargin: Startup.onboard ? 10 : 0
    readonly property int bottomMargin: 0
}
