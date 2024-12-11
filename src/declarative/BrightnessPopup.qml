/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Layouts

//! Popup UI item with embedded slider to adjust LED brightness
/*!
 * \ingroup GUI
 *
 */
Popup {
    id: popup

    //! The user is currently using the embedded slider.
    /*!
    * \sa value
    * \sa changed
    */
    readonly property bool inUse: brightnessSlider.pressed

    //! The current value of the embedded slider.
    /*!
    * \sa changedInteractive
    */
    property alias value: brightnessSlider.value

    //! The value has changed as an explicit result of the user moving the embedded slider.
    /*!
    * \sa value
    */
    signal changedInteractive

    leftPadding: Theme.proportion(gui, 0.029) + Onboard.leftMargin
    rightPadding: Theme.proportion(gui, 0.029) + Onboard.rightMargin

    topPadding: Theme.proportion(gui, 0.029) + Onboard.topMargin
    bottomPadding: Theme.proportion(gui, 0.029) + Onboard.bottomMargin

    modal: true

    onOpenedChanged: {
        if (opened) {
            autoCloseTimer.interval = 6000;
            autoCloseTimer.restart();
        }
    }

    background: Item {
        Rectangle {
            id: bgRect

            anchors.fill: parent

            color: Theme.windowBackgroundColor
        }

        MultiEffect {
            anchors.fill: parent

            opacity: 0.6

            shadowEnabled: true
            shadowHorizontalOffset: 0.0
            shadowVerticalOffset: 1
            shadowBlur: 0.5
            blurMax: 64

            source: bgRect
        }
    }

    contentItem: RowLayout {
        spacing: Theme.proportion(gui, 0.029)

        Image {
            Layout.alignment: Qt.AlignHCenter

            opacity: 0.84

            width: Theme.even(parent.height * 0.8)
            height: width

            sourceSize.width: width
            sourceSize.height: height

            source: "qrc:///assets/icons/low-brightness.svg"
        }

        Slider {
            id: brightnessSlider

            Layout.fillWidth: true
            Layout.fillHeight: true

            from: 0
            to: 1

            onMoved: {
                popup.changedInteractive();
                autoCloseTimer.interval = 2000;
                autoCloseTimer.restart();
            }

            background: Rectangle {
                x: parent.leftPadding
                y: parent.topPadding + parent.availableHeight / 2 - height / 2

                implicitWidth: 200
                implicitHeight: 4

                width: parent.availableWidth
                height: implicitHeight

                radius: 2

                color: Theme.inactiveButtonColor

                Rectangle {
                    width: brightnessSlider.visualPosition * parent.width
                    height: parent.height

                    radius: 2

                    color: Qt.lighter(Theme.activeButtonColor, 1.4)
                }
            }

            handle: Rectangle {
                x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                y: parent.topPadding + parent.availableHeight / 2 - height / 2

                width: parent.height
                height: width

                radius: width / 2

                color: Theme.activeButtonColor
            }
        }

        Image {
            Layout.alignment: Qt.AlignHCenter

            opacity: 0.84

            width: Theme.even(parent.height * 0.8)
            height: width

            sourceSize.width: width
            sourceSize.height: height

            source: "qrc:///assets/icons/high-brightness.svg"
        }
    }

    Timer {
        id: autoCloseTimer

        interval: 6000
        repeat: false

        onTriggered: popup.close()
    }
}
