/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Effects

//! Themed button UI item displaying a color choice
/*!
 * \ingroup GUI
 *
 */
AbstractButton {
    id: colorButton

    //! Color shown on the button.
    property alias color: circle.color

    checkable: true

    state: pressed || checked ? "active" : "inactive"

    onClicked: glowAnimation.restart()

    MultiEffect {
        id: glowEffect

        anchors.fill: background

        visible: true

        opacity: 0

        colorization: 1.0
        colorizationColor: Theme.activeButtonColor

        blurEnabled: true
        blurMax: 64
        blur: 1.0

        ParallelAnimation {
            id: glowAnimation

            loops: 1
            alwaysRunToEnd: true

            NumberAnimation {
                target: glowEffect
                property: "blur"
                from: 0
                to: 0.6
                duration: 600
                easing.type: Easing.OutCubic
            }

            OpacityAnimator {
                target: glowEffect
                from: 0.8
                to: 0.2
                duration: 600
                easing.type: Easing.OutCubic
            }
        }

        source: background
    }

    Rectangle {
        id: background

        anchors.fill: parent

        radius: width / 2

        Rectangle {
            id: circle

            anchors.centerIn: parent

            width: parent.width - Theme.even(colorButton.width * 0.08)
            height: width

            color: colorWheel.color
            border.color: Theme.windowBackgroundColor
            border.width: Theme.even(colorButton.width * 0.06)
            radius: width / 2
        }
    }

    states: [
        State {
            name: "active"
            PropertyChanges { target: background; color: Theme.activeButtonColor }
        },
        State {
            name: "inactive"
            PropertyChanges { target: background; color: Theme.inactiveButtonColor }
        }
    ]
}
