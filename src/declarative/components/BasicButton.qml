/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Effects

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
                to: 0.0
                duration: 600
                easing.type: Easing.OutCubic
            }
        }

        source: background
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

        layer.enabled: true
    }

    Rectangle {
        anchors.fill: icon

        visible: false

        id: invertedIcon

        color: Theme.windowBackgroundColor
    }

    MultiEffect {
        id: mask

        anchors.fill: icon

        maskEnabled: true
        maskSource: icon

        maskThresholdMin: 0.5
        maskSpreadAtMin: 1.0

        source: invertedIcon
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
