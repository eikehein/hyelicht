/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

import QtQuick
import QtQml

import com.hyerimandeike.hyelicht
import com.hyerimandeike.hyelicht.animations

NonVisualItem {
    id: main

    Instantiator {
        id: modelInstantiator

        asynchronous: false

        delegate: Startup.onboard ? onboardModelComponent : remoteClientModelComponent
    }

    Component {
        id: onboardModelComponent

        ShelfModel {
            id: shelfModel

            ledStrip: LedStrip {
                id: ledStrip

                enabled: Startup.onboard && !Startup.simulateShelf

                deviceName: Settings.spiDeviceName
                frequency: Settings.spiFrequency

                count: ((Settings.columns * Settings.density + (Settings.columns - 1)
                    * Settings.wallThickness) * Settings.rows)

                gammaCorrection: Settings.gammaCorrection
            }

            rows: Settings.rows
            columns: Settings.columns
            density: Settings.density
            wallThickness: Settings.wallThickness

            animateBrightnessTransitions: Settings.animateBrightnessTransitions
            animateAverageColorTransitions: Settings.animateAverageColorTransitions
            transitionDuration: Settings.transitionDuration

            animation: FireAnimation {}

            remotingEnabled: Startup.remotingApi
            listenAddress: Startup.remotingListenAddress
        }
    }

    Component {
        id: remoteClientModelComponent

        RemoteShelfModel {
            serverAddress: Startup.remotingServerAddress
        }
    }

    Instantiator {
        id: httpServerInstantiator

        active: Startup.onboard

        delegate: HttpServer {
            enabled: Startup.httpApi

            listenAddress: Startup.httpListenAddress
            port: Startup.httpPort

            model: modelInstantiator.object
        }
    }

    Instantiator {
        id: guiInstantiator

        active: !Startup.headless

        delegate: Gui {
            visible: true

            model: modelInstantiator.object
        }
    }
}
