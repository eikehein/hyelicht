/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15

import com.hyerimandeike.hyelicht 1.0

//! Main window UI item
/*!
 * \ingroup GUI
 *
 */
Window {
    id: gui
    
    width: 1024
    height: 600

    //! ShelfModel (or RemoteShelfModel) instance this GUI operates on.
    property QtObject model: null
    
    //! Whether the window is wider than it is tall.
    readonly property bool landscape: width > height

    //! Color selected on the embedded ColorWheel instance, or \c black when the eraser tool is active.
    readonly property color currentColor: (toolButtons.checkedButton == eraserButton
        ? "black" : colorWheel.color)

    //! DisplayController instance created and managed by this item.
    property alias displayController: displayControllerInstantiator.object

    //! Curtain instance created and managed by this item.
    property alias curtain: curtainLoader.item

    //! Black rectangle shown when the display is turned off, to avoid burn-in.
    property alias contentBlanking: contentBlankingLoader.item

    Instantiator {
        id: displayControllerInstantiator

        active: Startup.onboard
        asynchronous: false

        delegate: DisplayController {
            enabled: !Startup.simulateDisplay

            serialPortName: Settings.serialPortName
            baudRate: Settings.baudRate

            brightness: Settings.displayBrightness

            idleTimeout: Settings.idleTimeout
            fadeDuration: Settings.fadeDuration

            onOnChanged: {
                if (!on) {
                    // We don't do this declaratively because we need to be sure
                    // blanking is on/off before/after the respective action in
                    // DisplayController.
                    contentBlanking.opacity = 1.0;

                    if (curtain && !Startup.simulateDisplay) {
                        curtain.open = false;
                    }
                }
            }
        }
    }

    Rectangle {
        id: mainWindowBackground
        
        anchors.fill: parent
        
        color: Theme.windowBackgroundColor
    }

    TapHandler {
        id: pointHandler

        property alias position: pointHandler.point.position

        enabled: Startup.onboard

        onActiveChanged: {
            displayController.resetIdleTimeout();
        }

        onTapped: {
            if (!displayController.on) {
                // We don't do this declaratively because we need to be sure
                // blanking is on/off before/after the respective action in
                // DisplayController.
                contentBlanking.opacity = 0.0;

                displayController.on = true;
            } else if (curtain && !curtain.open) {
                curtain.open = true;
            }
        }

        onPositionChanged: displayController.resetIdleTimeout()
    }

    Loader {
        id: connectionStatusLoader

        anchors.top: parent.top
        anchors.right: parent.right

        // TODO: Once on Qt 6, use `QNetworkInformation` to also visualize
        // device online state (e.g. wifi disconnected) to make the reason
        // behind a disconnect discoverable.
        active: !Startup.onboard && !gui.model.connected
        sourceComponent: connectionStatusComponent
    }

    Component {
        id: connectionStatusComponent

        Image {
            opacity: 0.84

            width: Theme.proportion(gui, 0.129)
            height: width

            sourceSize.width: width
            sourceSize.height: height

            source: "network-disconnect.svg"
        }
    }

    Grid {
        id: mainGrid

        anchors.centerIn: parent

        verticalItemAlignment: Grid.AlignVCenter
        horizontalItemAlignment: Grid.AlignHCenter

        spacing: Math.floor(Theme.proportion(gui, 0.032) * 2.2)

        Shelf {
            id: shelf

            enabled: (curtain && curtain.open) || !curtain

            // Unset to avoid unnecessary busy work while the display is off.
            model: (curtain && curtain.open || !curtain) ? gui.model : null

            rows: model ? model.rows : 0
            columns: model ? model.columns : 0

            onTapped: {
                if (displayController) {
                    displayController.resetIdleTimeout();
                }
                
                if (!square) {
                    return;
                }

                if (toolButtons.checkedButton == colorPickerButton) {
                    if (Qt.colorEqual(square.color, "black")) {
                        eraserButton.checked = true;
                    } else {
                        colorWheel.color = square.color;
                        colorButton.checked = true;
                    }
                } else if (!Qt.colorEqual(square.color, gui.currentColor)) {
                    model.setData(model.index(square.modelIndex, 0),
                        gui.currentColor);
                }
            }

            states: [
                State {
                    name: "landscape"
                    when: gui.landscape

                    PropertyChanges {
                        target: shelf
                        width: Math.min(gui.width - controlsGrid.width 
                            - (3 * parent.spacing), Math.floor(controlsGrid.height * 1.046))
                        height: width
                    }
                },
                State {
                    name: "portrait"
                    when: !gui.landscape

                    PropertyChanges {
                        target: shelf
                        width: height
                        height: Math.min(gui.height - controlsGrid.height 
                            - (3 * parent.spacing), Math.floor(controlsGrid.width * 1.046))
                    }
                }
            ]
        }

        ButtonGroup {
            id: toolButtons

            exclusive: true
        }

        Grid {
            id: controlsGrid

            enabled: (curtain && curtain.open) || !curtain

            verticalItemAlignment: Grid.AlignVCenter
            horizontalItemAlignment: Grid.AlignHCenter

            spacing: Math.floor(1.48 * Theme.proportion(gui, 0.032))

            ColorWheel {
                id: colorWheel

                width: Theme.even((2 * colorButton.width) + controlsGrid.spacing)
                height: width

                onColorChanged: {
                    if (displayController) {
                        displayController.resetIdleTimeout();
                    }
                    
                    colorButton.checked = true;
                }
            }

            // Reason for the grid of grids: We want two clusters of buttons
            // to retain specific arrangements (i.e. keeping the paint-related
            // quad together) across the landscape-to-portrait orientation
            // change.
            // An alternative would be a single QtQuick.Layouts.GridLayout
            // and using States to set the cell coordinates, but this would
            // not be much more readable, either.
            Grid {
                id: controlButtonsGrid

                verticalItemAlignment: Grid.AlignVCenter
                horizontalItemAlignment: Grid.AlignHCenter

                spacing: Theme.proportion(gui, 0.029)

                Grid {
                    id: paintButtonsGrid

                    rows: 2
                    columns: 2

                    verticalItemAlignment: Grid.AlignVCenter
                    horizontalItemAlignment: Grid.AlignHCenter

                    spacing: Theme.proportion(gui, 0.029)

                    ColorButton {
                        id: colorButton


                        width: Theme.proportion(gui, 0.129)
                        height: width

                        checked: true // Default action on launch.

                        ButtonGroup.group: toolButtons

                        onCheckedChanged: {
                            if (checked) {
                                colorButtonClickBlockTimer.restart();
                            }
                        }
                        
                        onClicked: {
                            if (displayController) {
                                displayController.resetIdleTimeout();
                            }

                            if (!colorButtonClickBlockTimer.running) {
                                model.averageColor = gui.currentColor;
                            }
                        }

                        // Used to make sure that a click that checks `colorButton`
                        // doesn't end up setting the shelf's average color. Unfor-
                        // tunately, `Button.checked` is already true when the
                        // `onClicked` handler is true.
                        Timer {
                            id: colorButtonClickBlockTimer

                            interval: Qt.styleHints.mouseDoubleClickInterval
                            repeat: false
                        }
                    }

                    Button {
                        id: colorPickerButton

                        width: colorButton.width
                        height: width

                        source: "color-picker.svg"

                        checkable: true

                        ButtonGroup.group: toolButtons

                        onClicked: {
                            if (displayController) {
                                displayController.resetIdleTimeout();
                            }
                        }
                    }

                    Button {
                        id: eraserButton

                        width: colorButton.width
                        height: width

                        source: "draw-eraser.svg"

                        checkable: true

                        ButtonGroup.group: toolButtons

                        onClicked: {
                            if (displayController) {
                                displayController.resetIdleTimeout();
                            }
                        }
                    }

                    Button {
                        id: brightnessButton

                        width: colorButton.width
                        height: width

                        source: "high-brightness.svg"

                        checkable: true

                        onClicked: {
                            if (displayController) {
                                displayController.resetIdleTimeout();
                            }
                        }

                        onCheckedChanged: {
                            if (checked) {
                                brightnessPopup.open();
                            }
                        }

                        Connections {
                            target: brightnessPopup

                            function onClosed() {
                                brightnessButton.checked = false;
                            }
                        }
                    }
                }

                Grid {
                    id: stateButtonsGrid

                    verticalItemAlignment: Grid.AlignVCenter
                    horizontalItemAlignment: Grid.AlignHCenter

                    spacing: Theme.proportion(gui, 0.029)

                    Button {
                        id: fireplaceModeButton

                        width: colorButton.width
                        height: width

                        source: "fire.svg"

                        checkable: true
                        invertIconWhenChecked: false

                        onClicked: {
                            if (displayController) {
                                displayController.resetIdleTimeout();
                            }

                            model.animating = !model.animating;
                        }

                        Connections {
                            target: model
                            
                            function onAnimatingChanged() {
                                fireplaceModeButton.checked = model.animating;
                            }
                        }
                    }

                    Button {
                        id: onOffButton

                        width: colorButton.width
                        height: width

                        source: "system-shutdown.svg"

                        checkable: true

                        onClicked: {
                            if (displayController) {
                                displayController.resetIdleTimeout();
                            }

                            model.enabled = !model.enabled;
                            colorButton.checked = true;
                        }

                        Component.onCompleted: {
                            
                        }

                        // Why so convoluted? 
                        // -> https://bugreports.qt.io/browse/QTBUG-84746
                        Connections {
                            target: gui
                            
                            function onModelChanged() {
                                if (gui.model) {
                                    gui.model.enabledChanged.connect(function() {
                                        onOffButton.checked = gui.model.enabled;
                                    });
                                }
                            }

                            Component.onCompleted: {
                                if (gui.model) {
                                    onOffButton.checked = gui.model.enabled;
                                }
                            }
                        }
                    }

                    states: [
                        State {
                            name: "landscape"
                            when: gui.landscape
                            PropertyChanges { target: stateButtonsGrid; rows: 1; columns: 2 }
                        },
                        State {
                            name: "portrait"
                            when: !gui.landscape
                            PropertyChanges { target: stateButtonsGrid; rows: 2; columns: 1 }
                        }
                    ]
                }

                states: [
                    State {
                        name: "landscape"
                        when: gui.landscape
                        PropertyChanges { target: controlButtonsGrid; rows: 2; columns: 1 }
                    },
                    State {
                        name: "portrait"
                        when: !gui.landscape
                        PropertyChanges { target: controlButtonsGrid; rows: 1; columns: 2 }
                    }
                ]
            }

            states: [
                State {
                    name: "landscape"
                    when: gui.landscape

                    PropertyChanges { target: controlsGrid; rows: 2; columns: 1 }
                },
                State {
                    name: "portrait"
                    when: !gui.landscape
                    PropertyChanges { target: controlsGrid; rows: 1; columns: 2 }
                }
            ]
        }

        states: [
            State {
                name: "landscape"
                when: gui.landscape
                PropertyChanges { target: mainGrid; rows: 1; columns: 2 }
            },
            State {
                name: "portrait"
                when: !gui.landscape
                PropertyChanges { target: mainGrid; rows: 2; columns: 1 }
            }
        ]
    }

    BrightnessPopup {
        id: brightnessPopup
        
        width: parent.width
        height: colorButton.width + (Theme.proportion(gui, 0.029) * 2) + Onboard.topMargin

        onAboutToShow: value = model.brightness

        onChangedInteractive: model.brightness = value

        Connections {
            target: model

            function onBrightnessChanged() {
                if (displayController) {
                    displayController.resetIdleTimeout();
                }

                if (!brightnessPopup.inUse) {
                    brightnessPopup.value = model.brightness;
                }
            }
        }
    }

    Loader {
        id: curtainLoader

        anchors.fill: parent

        active: Startup.onboard && Settings.curtain
        sourceComponent: curtainComponent
    }

    Component {
        id: curtainComponent

        Curtain {
            enabled: Startup.onboard

            showInviteButton: !open
            animateInviteButton: displayController.on && showInviteButton
        }
    }

    // When the display backlight is off, also render fully black
    // window content to avoid burn-in (which may occurs regardless
    // of the backlight level).
    Loader {
        id: contentBlankingLoader

        anchors.fill: parent

        active: Startup.onboard
        sourceComponent: contentBlankingComponent
    }

    Component {
        id: contentBlankingComponent

        Rectangle {
            id: contentBlanking

            anchors.fill: parent

            opacity: 0.0

            Behavior on opacity {
                enabled: Startup.simulateDisplay

                OpacityAnimator {
                    duration: displayController.fadeDuration
                    easing: displayController.fadeEasing

                    onRunningChanged: {
                        if (!running) {
                            curtain.open = (to === 1.0);
                        }
                    }
                }
            }

            color: "black"
        }
    }
}
