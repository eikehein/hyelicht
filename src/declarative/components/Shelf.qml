/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>
 */

import QtQuick 2.15

//! Visualization UI item specific to the Hyelicht shelf
/*!
 * \ingroup GUI
 *
 */
Item {
    id: shelf

    implicitWidth: shelfImage.implicitWidth
    implicitHeight: shelfImage.implicitHeight

    //! ShelfModel (or RemoteShelfModel) instance this item is visualizing.
    property alias model: shelfRepeater.model

    //! Number of boards in the shelf.
    property alias rows: shelfGrid.rows

    //! Number of compartments in each shelf board.
    property alias columns: shelfGrid.columns

    //! A compartment in the shelf visualization has been tapped by the user.
    signal tapped(Item square)

    Image {
        id: shelfImage

        anchors.fill: parent

        readonly property real scaleFactor: paintedWidth / implicitWidth

        source: "shelf.png"
        fillMode: Image.PreserveAspectFit
    }

    Grid {
        id: shelfGrid

        anchors.top: shelfImage.top
        anchors.topMargin: 10.5 * shelfImage.scaleFactor
        anchors.horizontalCenter: shelfImage.horizontalCenter
        anchors.alignWhenCentered: false

        spacing: 2.9 * shelfImage.scaleFactor

        Repeater {
            id: shelfRepeater

            delegate: Rectangle {
                readonly property int modelIndex: index

                width: 94.8 * shelfImage.scaleFactor
                height: ((index < 5) ? 95.2 : ((index > 14) ? 94 : 94.4)) * shelfImage.scaleFactor

                opacity: 0.6

                // NOTE: Falling back to black in case the model returns QVariant(),
                // e.g. due to its remote connection state.
                color: model.decoration || "black"
            }
        }
    }

    TapHandler {
        target: shelfGrid

        onTapped: shelf.tapped(shelfGrid.childAt(point.pressPosition.x, point.pressPosition.y))
    }

    PointHandler {
        id: pointHandler

        target: shelfGrid

        property alias position: pointHandler.point.position

        onPositionChanged: {
            if (active) {
                shelf.tapped(shelfGrid.childAt(position.x, position.y));
            }
        }
    }
}
