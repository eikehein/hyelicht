/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>
 */

import QtQuick 2.15
import QtQuick.Window 2.15
import QtGraphicalEffects 1.15

//! Animated welcome page UI item for onboard mode
/*!
 * \ingroup GUI
 *
 */
Item {
    id: curtain

    //! Duration for the opening animation.
    /*!
    * Defaults to \c 400.
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
        clip: true
        color: Theme.brandColor
        
        Item {
            width: Window.width
            height: Window.height
            
            Image {
                anchors.centerIn: parent
                
                height: parent.height
                
                fillMode: Image.PreserveAspectFit
                source: "ocean.png"
            }
        }
    }
    
    DropShadow {
        id: upperCurtain
        
        width: parent.width
        height: parent.height / 2

        horizontalOffset: 0
        verticalOffset: upperCurtainTransition.running ? 2 : 0
        radius: upperCurtainTransition.running ? 12.0 : 0.0
        samples: 16
        color: "#80000000"
        cached: true
        source: upperCurtainRenderSource
        
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
        
        Item {
            anchors.bottom: parent.bottom
            
            width: Window.width
            height: Window.height
            
            Image {
                anchors.centerIn: parent
                
                height: parent.height
                
                fillMode: Image.PreserveAspectFit
                source: "ocean.png"
            }
        }
    }

    RectangularGlow {
        id: glowEffect

        anchors.fill: inviteButton

        visible: showInviteButton

        opacity: 0

        color: "white"
        spread: 1.0
        glowRadius: 0
        cornerRadius: inviteButton.radius + glowRadius

        ParallelAnimation {
            running: inviteButton.animateInviteButton
            loops: Animation.Infinite

            NumberAnimation {
                target: glowEffect
                property: "glowRadius"
                from: 0
                to: 10
                duration: 1000
                easing.type: Easing.OutCubic
            }

            OpacityAnimator {
                target: glowEffect
                from: 0.8
                to: 0.0
                duration: 1000
                easing.type: Easing.OutCubic
            }
        }
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
