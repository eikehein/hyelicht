/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15

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

    RectangularGlow {
        id: glowEffect

        anchors.fill: background

        visible: true

        opacity: 0

        color: Theme.activeButtonColor
        spread: 1.0
        glowRadius: 0
        cornerRadius: background.radius + glowRadius

        ParallelAnimation {
            id: glowAnimation

            loops: 1
            alwaysRunToEnd: true

            NumberAnimation {
                target: glowEffect
                property: "glowRadius"
                from: 0
                to: Theme.even(colorButton.width * 0.12)
                duration: 600
                easing.type: Easing.OutCubic
            }

            OpacityAnimator {
                target: glowEffect
                from: 0.8
                to: 0.0
                duration: 600
                easing.type: Easing.OutCubic
            }
        }
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
