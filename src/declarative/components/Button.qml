/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15

//! Themed button UI item
/*!
 * \ingroup GUI
 *
 */
AbstractButton {
    id: button

    //! Toggle whether the button should outline-mask and color-invert the icon when checked.
    /*!
    * Defaults to \c true.
    */
    property bool invertIconWhenChecked: true

    //! The source URL for the button icon.
    property alias source: icon.source

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
        anchors.fill: parent

        id: background

        radius: width * 0.14
    }

    Image {
        id: icon

        anchors.centerIn: parent

        opacity: 0.84

        width: Theme.even(parent.width * 0.8)
        height: width

        fillMode: Image.PreserveAspectFit

        sourceSize.width: width
        sourceSize.height: height
    }

    Rectangle {
        anchors.fill: icon

        visible: false

        id: invertedIcon

        color: Theme.windowBackgroundColor
    }

    OpacityMask {
        id: mask

        anchors.fill: icon

        source: invertedIcon
        maskSource: icon
    }

    states: [
        State {
            name: "active"

            PropertyChanges { target: background; color: Theme.activeButtonColor }
            PropertyChanges { target: icon; visible: !button.invertIconWhenChecked }
            PropertyChanges { target: icon; opacity: 1.0 }
            PropertyChanges { target: mask; visible: button.invertIconWhenChecked }
        },
        State {
            name: "inactive"
            PropertyChanges { target: background; color: Theme.inactiveButtonColor }
            PropertyChanges { target: icon; visible: true }
            PropertyChanges { target: icon; opacity: 0.84 }
            PropertyChanges { target: mask; visible: false }
        }
    ]
}
