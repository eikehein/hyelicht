/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>
 */

import QtQuick 2.15
import QtGraphicalEffects 1.15

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

        vertexShader: "
            uniform highp mat4 qt_Matrix;
            attribute highp vec4 qt_Vertex;
            attribute highp vec2 qt_MultiTexCoord0;
            varying highp vec2 coord;

            void main() {
                coord = qt_MultiTexCoord0;
                gl_Position = qt_Matrix * qt_Vertex;
            }"

        fragmentShader: "
            // Use high precision if available.
            #ifdef GL_FRAGMENT_PRECISION_HIGH
            precision highp float;
            #endif

            varying highp vec2 coord;
            #define TWO_PI 6.28318530718

            // Convert from HSV to RGB with some cubic smoothing for
            // a nicer, more perceptual appearance.
            vec3 hsv2rgb(in vec3 c) {
                vec3 rgb = clamp(abs(mod(c.x * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
                rgb = rgb * rgb * (3.0 - 2.0 * rgb);
                return c.z * mix(vec3(1.0), rgb, c.y);
            }

            void main() {
                vec2 st = coord;
                vec3 color = vec3(0.0);

                // Convert cartesian to polar coordinates.
                vec2 toCenter = vec2(0.5) - st;
                float angle = atan(toCenter.y, toCenter.x);
                float radius = length(toCenter) * 2.0;

                color = hsv2rgb(vec3((angle / TWO_PI) + 0.5, radius, 1.0));
                gl_FragColor = vec4(color, 1.0);
            }"
    }

    Rectangle {
        id: mask

        anchors.fill: wheel

        visible: false

        color: Theme.windowForegroundColor
        radius: width / 2
    }

    OpacityMask {
        anchors.fill: wheel

        source: wheel
        maskSource: mask

        cached: true
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
