/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

import QtQuick
import QtQuick.Effects
import QtQuick.Window

//! Animated welcome page UI item for onboard mode
/*!
 * \ingroup GUI
 *
 */
Item {
    id: curtain

    //! Duration for the opening animation.
    /*!
    * Defaults to \c 440.
    */
    property int animationDuration: 440

    //! Whether the welcome page is shown.
    /*!
    * Defaults to \c false.
    */
    property bool open: false

    //! Whether the invite button is shown.
    /*!
    * Defaults to \c false.
    */
    property alias showInviteButton: inviteButton.showInviteButton

    //! Whether the invite button is animated.
    /*!
    * Defaults to \c false.
    */
    property alias animateInviteButton: inviteButton.animateInviteButton

    Rectangle {
        id: darkOut

        anchors.fill: parent

        opacity: 0.2
        color: "black"

        state: curtain.open ? "open" : "closed"

        states: [
            State {
                name: "closed"
                PropertyChanges { target: darkOut; opacity: 0.2 }
            },
            State {
                name: "open"
                PropertyChanges { target: darkOut; opacity: 0.0 }
            }
        ]

        transitions: Transition {
            to: "open"

            OpacityAnimator {
                duration: curtain.animationDuration
                easing.type: Easing.InCubic
            }
        }
    }

    Rectangle {
        id: upperCurtainRenderSource

        width: parent.width
        height: parent.height / 2

        visible: false

        color: Theme.brandColor

        Image {
            id: upperImage

            anchors.fill: parent

            anchors.leftMargin: (parent.width - lowerImage.paintedWidth) / 2
            anchors.rightMargin: (parent.width - lowerImage.paintedWidth) / 2

            fillMode: Image.PreserveAspectCrop
            source: "qrc:///assets/ocean.png"

            horizontalAlignment: Image.AlignHCenter
            verticalAlignment: Image.AlignTop
        }
    }

    MultiEffect {
        id: upperCurtain

        width: upperCurtainRenderSource.width
        height: upperCurtainRenderSource.height

        visible: true

        shadowEnabled: upperCurtainTransition.running
        shadowHorizontalOffset: 0.0
        shadowVerticalOffset: 1
        shadowBlur: 1.0
        blurMax: 64

        state: curtain.open ? "open" : "closed"

        states: [
            State {
                name: "closed"
                PropertyChanges { target: upperCurtain; y: 0 }
            },
            State {
                name: "open"
                PropertyChanges { target: upperCurtain; y: -(parent.height / 2) }
            }
        ]

        transitions: Transition {
            id: upperCurtainTransition

            to: "open"

            YAnimator {
                duration: curtain.animationDuration
                easing.type: Easing.InCubic
            }
        }

        source: upperCurtainRenderSource
    }

    Rectangle {
        id: lowerCurtain

        y: parent.height / 2

        width: parent.width
        height: parent.height / 2

        clip: true

        color: Theme.brandColor

        state: curtain.open ? "open" : "closed"

        states: [
            State {
                name: "closed"
                PropertyChanges { target: lowerCurtain; y: parent.height / 2 }
            },
            State {
                name: "open"
                PropertyChanges { target: lowerCurtain; y: parent.height }
            }
        ]

        transitions: Transition {
            to: "open"

            YAnimator {
                duration: curtain.animationDuration
                easing.type: Easing.InCubic
            }
        }

        Image {
            id: lowerImage

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom

            width: Window.width
            height: Window.height

            fillMode: Image.PreserveAspectFit
            source: "qrc:///assets/ocean.png"
        }
    }

    MultiEffect {
        id: glowEffect

        anchors.fill: inviteButton

        visible: showInviteButton

        opacity: 0

        colorization: 1.0
        colorizationColor: "white"

        blurEnabled: true
        blurMax: 64
        blur: 0.0
        blurMultiplier: 0.2

        ParallelAnimation {
            id: inviteButtonAnimation

            loops: 1

            NumberAnimation {
                target: glowEffect
                property: "blur"
                from: 0
                to: 1.0
                duration: 1100
                easing.type: Easing.OutCubic
            }

            OpacityAnimator {
                target: glowEffect
                from: 0.8
                to: 0.0
                duration: 1100
                easing.type: Easing.OutCubic
            }
        }

        Timer {
            running: inviteButton.animateInviteButton
            interval: 1800
            repeat: true

            onTriggered: inviteButtonAnimation.start()
            onRunningChanged: inviteButtonAnimation.stop()
        }

        source: inviteButton
    }

    Rectangle {
        id: inviteButton

        y: (curtain.height / 2) - 8 // Aesthetically-pleasing magic numbers.
        anchors.horizontalCenter: parent.horizontalCenter

        property bool showInviteButton: false
        property bool animateInviteButton: false

        onShowInviteButtonChanged: {
            if (!showInviteButton) {
                inviteButtonFadeOut.running = true;
            } else {
                inviteButton.opacity = 1.0;
            }
        }

        OpacityAnimator {
            id: inviteButtonFadeOut

            target: inviteButton
            from: 1.0
            to: 0.0
            duration: 160
        }

        width: inviteText.width + 24
        height: inviteText.height + 4

        color: "white"

        radius: height / 2

        Text {
            id: inviteText

            y: parent.height / 2 - height / 2 - 1 // Aesthetically-pleasing magic numbers.
            anchors.horizontalCenter: parent.horizontalCenter

            text: i18n("Touch to begin")
            color: Theme.brandColor
            font.pixelSize: Theme.proportion(gui, 0.042)
            font.family: Theme.fontFamily
            font.weight: Font.DemiBold
        }
    }
}
