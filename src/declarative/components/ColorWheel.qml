/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

import QtQuick
import QtQuick.Effects

//! RGB color wheel UI item
/*!
 * \ingroup GUI
 *
 */
Item {
    id: colorWheel

    //! Color selected on the RGB color wheel.
    /*!
    * Defaults to \c white.
    */
    property color color: "white"

    ShaderEffect {
        id: wheel

        anchors.fill: parent

        visible: false

        vertexShader: "qrc:///shaders/ColorWheel.vert.qsb"
        fragmentShader: "qrc:///shaders/ColorWheel.frag.qsb"
    }

    Rectangle {
        id: mask

        anchors.fill: wheel

        visible: false

        color: Theme.windowForegroundColor
        radius: width / 2

        layer.enabled: true
        antialiasing: true
    }

    MultiEffect {
        anchors.fill: wheel

        maskEnabled: true
        maskSource: mask

        maskThresholdMin: 0.5
        maskSpreadAtMin: 1.0

        source: wheel
    }

    Rectangle {
        id: picker

        readonly property point pos: {
            // Convert from the current color to polar and then to
            // cartesian coordinates.
            var x = (parent.width /2) * (1 - colorWheel.color.hsvSaturation
                * Math.cos(2 * Math.PI * colorWheel.color.hsvHue - Math.PI))
                - picker.radius;
            var y = (parent.width /2) * (1 + colorWheel.color.hsvSaturation
                * Math.sin(-2 * Math.PI * colorWheel.color.hsvHue - Math.PI))
                - picker.radius;
            return Qt.point(x, y);
        }

        x: pos.x
        y: pos.y

        width: Theme.even(colorWheel.width * 0.09)
        height: width

        color: "transparent"
        border.color: "#404040"
        border.width: Theme.even(colorWheel.width * 0.024)
        radius: width / 2

        Rectangle {
            anchors.centerIn: picker

            width: picker.width - (parent.border.width / 2)
            height: width

            color: "transparent"
            border.color: Theme.windowBackgroundColor
            border.width: (parent.border.width / 2)
            radius: width / 2
        }
    }

    PointHandler {
        id: pointHandler

        property alias position: pointHandler.point.position

        onPositionChanged: {
            if (active) {
                var x = 0.5 - (position.x / width);
                var y = 0.5 - (position.y / height);
                var angle = Math.min(Math.max((Math.atan2(y, x) / 6.28318530718) + 0.5, 0), 1.0);
                var radius = Math.min(Math.max(Math.sqrt(x * x + y * y) * 2.0, 0), 1.0);
                colorWheel.color = Qt.hsva(angle, radius, 1.0, 1.0);
            }
        }
    }
}
