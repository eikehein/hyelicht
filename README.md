<!--
SPDX-License-Identifier: CC-BY-4.0
SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>
-->

<img id="body-logo" src="data/icons/sc-apps-hyelicht.svg?raw=true" alt="Hyelicht icon" width="64" height="64" align="right" />

# Hyelicht

[![REUSE status](https://api.reuse.software/badge/github.com/eikehein/hyelicht)](https://api.reuse.software/info/github.com/eikehein/hyelicht) 
[![Linux (Onboard)](https://github.com/eikehein/hyelicht/actions/workflows/linux_onboard.yml/badge.svg)](https://github.com/eikehein/hyelicht/actions/workflows/linux_onboard.yml) [![Android (Client)](https://github.com/eikehein/hyelicht/actions/workflows/android_client.yml/badge.svg)](https://github.com/eikehein/hyelicht/actions/workflows/android_client.yml) 
[![Documentation](https://github.com/eikehein/hyelicht/actions/workflows/doxygen.yml/badge.svg)](https://eikehein.github.io/hyelicht/)

**Hyelicht** is an IoT/embedded project for home decoration/automation. Its software allows you to do colorful painting and animations on a LED-backlit shelf, using an embedded touchscreen and a companion app for phones and PCs. It also offers integration with a smart lights system.

Hyelicht's reference hardware is a 5x5 IKEA Kallax shelf with multiple rows of LED backlighting and a side-mounted 7″ touchscreen integrated into an aesthetically-pleasing picture frame. A single-board computer running Linux, a helper MCU and two 12A 5V bench power supplies are hidden inside the shelf.

A photo of the assembled shelf can be found <a href="https://www.eikehein.com/stuff/hyelicht/shelf.jpg">here</a>.

## Features

- Responsive Touch GUI with support for landscape and portrait orientations
  - Turn shelf on/off, adjust brightness, painting tools, trigger animations
  - Runs synchronized on embedded touchscreen and multiple Android/PC devices
  - Runs in-process or standalone
- SK9822/APA102 LED paint engine supporting gamma correction and HSV-based brightness derivation
- Animation framework
  - Fireplace animation 🔥
- Embedded display backlight control with MCU-generared PWM signal
  - Smooth display fade-in on user interaction, fade-out on idle timeout
- [HTTP REST API](#http-rest-api)
- CLI frontend utility [`hyelichtctl`](#hyelichtctl-cli-utility) (HTTP-based), works locally and over the network
- [Philips Hue](https://en.wikipedia.org/wiki/Philips_Hue) smart lights integration: HTTP-based [plugin](#diyhue-integration-plugin) for the [diyHue](https://diyhue.org/) open source bridge emulator
- [systemd integration](#systemd-service-unit) (service unit)
- [Doxygen](https://www.doxygen.nl/)-based [documentation](#source-code)
- [REUSE](https://reuse.software/)- and [SPDX](https://spdx.org/licenses/)-compliant free software written in C++17 and QML

## Screenshots

<img src="docs/screenshot.png?raw=true" alt="Screenshot of onboard Touch GUI" width="600"/> <img src="docs/screenshot_phone.png?raw=true" alt="Screenshot of Touch GUI on Android phone"  height="348"/>

A photo of the assembled shelf can be found <a href="https://www.eikehein.com/stuff/hyelicht/shelf.jpg">here</a>.

## Licensing

Most of Hyelicht is available under **GNU GPL v2**. All licenses used in the project:

- GNU GPL v2.0, v3.0 or any later version approved by [KDE e.V.](https://ev.kde.org/) (Most source code)
- LGPL-3.0-or-later (Most icons)
- CC-BY-4.0 (Most other graphical assets, metadata and documentation)
- BSD 3-Clause "New" or "Revised" License
- Creative Commons Zero v1.0 Universal

The codebase is [REUSE](https://reuse.software/)- and [SPDX](https://spdx.org/licenses/)-compliant. See files in `LICENSES/` and `.reuse/dep5`.

***

<div id="dummy"></div>

## Table of Contents

- [Documentation](#documentation)
  - [Architecture](#architecture)
  - [Source code](#source-code)
- [Installation and using](#installation-and-using)
  - [Supported platforms](#supported-platforms)
      - [LEDs](#leds)
      - [Helper MCU (PWM signal generator)](#helper-mcu-pwm-signal-generator)
  - [Dependencies](#dependencies)
      - [General build dependencies](#general-build-dependencies)
      - [Optional build dependencies](#optional-build-dependencies)
      - [Optional runtime dependencies](#optional-runtime-dependencies)
      - [Build dependencies for included helper MCU program](#build-dependencies-for-included-helper-mcu-program)
  - [Build instructions](#build-instructions)
      - [Android build](#android)
  - [Build options](#build-options)
      - [User options](#user-options)
      - [Developer options](#developer-options)
  - [Deployment](#deployment)
      - [systemd service unit](#systemd-service-unit)
      - [LED wiring](#wiring)
      - [diyHue integration plugin](#diyhue-integration-plugin)
  - [Running](#running)
      - [Command line options](#command-line-options)
      - [Additional command line options for onboard build configuration](#additional-command-line-options-for-onboard-build-configuration)
  - [Config file](#config-file)
  - [Logging](#logging)
- [HTTP REST API](#http-rest-api)
- [hyelichtctl CLI utility](#hyelichtctl-cli-utility)

***

## Documentation

### Architecture

The full Hyelicht system contains the following elements:

<img src="docs/system_diagram.png?raw=true" alt="System diagram" width="740"/>

 (See below for additional detail on [wiring](#wiring).)

Hyelicht has a main application codebase with multiple build+runtime modes for **onboard** (embedded with or without in-process Touch GUI) and **standalone Touch GUI** (client) use. A monolithic-by-default single-application architecture was chosen to minimize resource needs and loading times in onboard mode. Internally, the application heavily optimizes for each mode. For details on how to choose the mode, see the sections on [build options](#build-options) and [command line options](#command-line-options).

In **onboard mode**, the application controls the LED strip and the display backlight (with the help of additional software on a companion MCU) and offers servers for instances running in Touch GUI mode as well as HTTP clients. In **standalone Touch GUI mode**, these features are disabled and the application acts as a pure client only.

Additionally, the onboard application can also be run in [**headless mode**](#additional-command-line-options-for-onboard-build-configuration). This disables the in-process Touch GUI. An additional instance in Touch GUI mode can then be run as a pure client on the same device, if process separation is desired.

Included clients to the HTTP REST API include a command line frontend utility [`hyelichtctl`](#hyelichtctl-cli-utility) and a [diyHue integration plugin](#diyhue-integration-plugin).

### Source code

Hyelicht's source code has full [Doxygen](https://www.doxygen.n/l)-based documentation. You can browse a generated snapshot of the latest documentation [here](https://eikehein.github.io/hyelicht/).

If you want to generate the documentation locally yourself, please check the [build instructions](#doxygen).

***

## Installation and using

### Supported platforms

Hyelicht has a largely platform-agnostic codebase that should build and run on any platform providing its build dependencies. However, its custom LED paint engine makes use of a Linux-specific API for SPI communication.

Hyelicht is being tested on the following hardware and software:

- Onboard mode: Raspberry Pi 4B running Raspberry OS
- Standalone Touch GUI mode:
  - Desktop PC: Fedora Linux
  - Android phone: Android+Qt+KDE SDK via the [KDE Android Docker image](https://community.kde.org/Android/Environment_via_Container) and several Android 11/12 phones
- LED strip: 416x SK9822 
- Helper MCU: Android Nano 3.0 (AVR ATmega328)

See section [Architecture](#architecture) for an explanation of the modes.

#### LEDs

The reference hardware for the project uses SK9822 LEDs. However, the custom LED paint engine should also be compatible with APA102 LEDs. The SK9822 is a later clone of APA102 with minimal differences in the accepted input.

#### Helper MCU (PWM signal generator)

In onboard mode, the Hyelicht application uses serial communication to a helper MCU that drives the embedded display backlight with a PWM signal. The helper MCU is expected to read 8-bit integer values between `0` and `255` on serial and convert them to the corresponding signal. In principle, any MCU programmed to conform to this expectation will work.

The reference hardware used in the project is an Android Nano 3.0 (AVR ATmega328). See `src/arduino_pwm_generator.cpp` for the reference program.

### Dependencies

#### General build dependencies

- C++17-supporting C++ compiler
- CMake v3.16+
- Qt v5.15+ modules QtCore, QtNetwork, QtQML, QtQuick, QtRemoteObjects
- KDE Frameworks 5 v5.78+ modules Extra CMake Modules/ECM, KCoreAddons, KConfig, KI18n from
- Linux headers (for SPI)

#### Optional build dependencies

- For [onboard](#architecture)  build configuration:
    - Qt v5.15+ module QtSerialPort
    - QHttpEngine v1.0.1+ 
- For [Android build](#android):
    - Qt v5.15+ module QtAndroidExtras
    - Android v30+ SDK+NDK (see below)
- For [documentation](#developer-options) build:
    - Doxygen v1.8.8+
    - Doxyqml

A convenient way to access a complete Android+Qt+KDE SDK is via the [KDE Android Docker image](https://community.kde.org/Android/Environment_via_Container).

#### Optional runtime dependencies

- Raspberry OS: SPI enabled
- For [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) developer build configuration: clang-tidy

#### Build dependencies for included helper MCU program

- [arduino-cli](https://github.com/arduino/arduino-cli) or Arduino IDE

### Build instructions

Hyelicht has a [CMake](https://cmake.org/)-based build system. Here are some general build commands:

    (Hyelicht codebase is in directory `hyelicht`.)
    $ cd hyelicht/
    $ mkdir build && cd build/
    $ cmake ../build -DCMAKE_INSTALL_PREFIX=<install path>
    $ make
    $ sudo make install

When running the `cmake` command, you may want to pass additional build options, like so:

    $ cmake ../build -DSOME_OPTION=value

See below for available build options and their default values.

#### Android

Building for Android is easy using the [KDE Android Docker image](https://community.kde.org/Android/Environment_via_Container).

See the `support/kde_android_docker_build.sh` build script and its embedded comments for specific instructions.

#### Doxygen
To generate [source code documentation](#source-code) using Doxygen during a build, enable the `BUILD_DOCS` [build option](#build-options).

### Build options

See the [build instructions](#build-instructions) for how to set these options when building Hyelicht.

#### User options

| Option | Default | Description
| - | - | - |
| **BUILD_ONBOARD** | **TRUE** unless Android build | In the [onboard](#architecture) build configuration, additional features and command line options become available that only make sense when Hyelicht is running on the hardware embedded into the shelf. For example, to control the LEDs and the embedded display backlight, and servers for the Touch GUI and HTTP clients. This alters the list of [build dependencies](#general-build-dependencies). |
| **BUILD_CLI** | **TRUE** unless Android build | Builds the HTTP-based [`hyelichtctl`](#hyelichtctl-cli-utility) command line utility. |

#### Developer options

| Option | Default | Description
| - | - | - |
| **BUILD_DOCS** | **FALSE** | Generates project documentation using [Doxygen](https://www.doxygen.nl/). This alters the list of [build dependencies](#general-build-dependencies). The generated documentation will appear inside the `docs/html/` sub-directory of the build directory. |
| **CLANG_TIDY** | **FALSE** | Reformats the source code using [clang-tidy](https://clang.llvm.org/extra/clang-tidy/). |
| **COMPILE_QML** | **TRUE** | Pre-compiles QML source files for faster loading speeds. |
| **DEBUG_QML** | **FALSE** | Enables the QML debug server. |

### Deployment

#### systemd service unit

In the [onboard](#architecture) build configuration, Hyelicht installs a [systemd](https://www.freedesktop.org/wiki/Software/systemd/) service unit. It is recommended to run it as a user unit, like so:

    $ systemctl --user enable hyelicht

If you just installed Hyelicht, you may need to run `systemctl --user daemon-reload` first.

#### diyHue integration plugin

Plugin code for simple integration with [diyHue](https://diyhue.org/) is provided at `support/diyhue_hyelicht.py`.

However, diyHue does not support external plugin installation and must be patched to use the plugin. The shelf must also be manually added to the diyHue configuration. Both are left as an exercise for the reader.

The plugin acts as a client to the [HTTP REST API](#http-rest-api).

#### Wiring

The LEDs are connected to the Raspberry Pi 4B and the bench PSUs (part of the [reference hardware](#supported-platforms) used by the project) as follows:

<img src="docs/wiring_diagram.png?raw=true" alt="Wiring diagram for Raspberry Pi and LEDS" width="480"/>

In the actual shelf, there are two 12A 5V bench PSUs to power 416 LEDs (50 mA each at full brightness). Both PSUs are connected to both ends (5V and GND) of two LED strips each, with each LED strip having 104 LEDs. Connecting to both ends potentially improves signal integrity (GND) and avoids a one-sided brownout if (5V), should your PSUs be too weak (instead, the brownout would occur in the middle of the strips). All grounds are connected, including the Raspberry Pi's, to ground the SPI connection.

The Raspberry Pi, and the helper MCU and the embedded touchscreen connected to it, is seperately powered via the official Raspberry Pi USB-C power supply. The inputs and outputs of all three PSUs are connected on a single wall power plug.

See also the [system diagram](#architecture).

### Running

The main application executable is named `hyelicht`.

It can be run manually:
    
    $ hyelicht

Or (in an [onboard](#architecture) build configuration) started in [onboard mode](#architecture)  via the included [systemd](https://www.freedesktop.org/wiki/Software/systemd/) unit:

    $ systemctl --user start hyelicht

The `hyelichtctl` command line utility is described [below](#hyelichtctl-cli-utility).

#### Command line options

`hyelicht` supports standard command options options such as **-h, --help**.

Various command line options will override an equivalent setting that can be changed via the [config file](#config-file).

Command line options unique to `hyelicht`:

| Option | Default | Description
| - | - | - |
| **-r, --remotingApiServerAddress** | from [Settings](#config-file) | Overrides the address Touch GUI client instances contact the remoting API server on. If unset, the listen address is taken from the settings (see the section [Config file](#config-file)). |

#### Additional command line options for onboard build configuration

| Option | Default | Description
| - | - | - |
| **-o, --onboard** | **UNSET** | Runs the application in [onboard mode](#architecture) . This is used by the [systemd service unit](#systemd-service-unit). |
| **--headless** | **UNSET** | Disables the in-process Touch GUI. Useful if running the GUI out-of-process as a standalone client instance is desired. |
| **--simulateDisplay** | **UNSET** | Disables serial communication to the helper MCU and simulates display backlight status in the Touch GUI. |
| **--simulateShelf** | **UNSET** | Disables SPI communication with the LED strip. The Touch GUI and all APIs can be used regardless. |
| **-s, --httpListenAddress** | from [Settings](#config-file) | Overrides the listen address for the [HTTP REST API](#http-rest-api) server. If unset, the listen address is taken from the settings (see the section [Config file](#config-file)). |
| **-p, --httpListenPort** | from [Settings](#config-file) | Overrides the port the [HTTP REST API](#http-rest-api) server listens on. If unset, the listen address is taken from the settings (see the section [Config file](#config-file)). |
| **--disableHttpApi** | from [Settings](#config-file) | If set, disables the HTTP REST API server. If unset, whether to enable this API server is taken from the settings (see the section [Config file](#config-file)). |
| **-l, --remotingApiListenAddress** | from [Settings](#config-file) | Overrides the listen address for the remoting API server used by Touch GUI client instances. If unset, the listen address is taken from the settings (see the section [Config file](#config-file)). |
| **--disableRemotingApi** | from [Settings](#config-file) | If set, disables the remoting API server. If unset, whether to enable this API server is taken from the settings (see the section [Config file](#config-file)). |

### Config file

Hyelicht supports a config file in INI-style format. It should be located at `$XDG_CONFIG_HOME/hyelichtrc` or in an equivalent system or user location following the [XDG Base Directory](https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html) specification.

See the XML source file `src/settings/hyelicht.kcfg` for the INI group and key names and the description and default value of each setting. Those attempting to build their own shelf (something the authors hope you may do!) will are likely to take special interest in the various settings related to shelf size and server addresses.

### Logging

Hyelicht's applications can output error and debug messages on `stdout` and `stderr` using Qt's categorized logging framework.

When you run `hyelicht` or [`hyelichtctl`](#hyelichtctl-cli-utility) manually, you should enable its logging categories to see this output:

    $ export QT_LOGGING_RULES=com.hyerimandeike.*=true
    $ hyelicht

If the application is run via the included [systemd service unit](#systemd-service-unit), the categories are enabled by default and messages can be looked up in the journal:

    $ journalctl --user -u hyelicht.service

The complete list of logging categories:

| Name | Description
| - | - |
| com.hyerimandeike.hyelicht | General messages |
| com.hyerimandeike.hyelicht.Animations | Animation framework |
| com.hyerimandeike.hyelicht.DisplayController | Serial communication with helper MCU for display backlight control |
| com.hyerimandeike.hyelicht.HttpServer | [HTTP REST API](#http-rest-api) server |
| com.hyerimandeike.hyelichtctl | [`hyelichtctl` command line utility](#hyelichtctl-cli-utility) |
| com.hyerimandeike.hyelicht.LedStrip | SPI communication with LED strip and LED paint engine |
| com.hyerimandeike.hyelicht.Remoting | Communication between onboard and standalone Touch GUI clients |

***

## HTTP REST API

In [onboard mode](#architecture), the main application offers a HTTP REST API (unless disabled via the [Config file](#config-file) or [command line option](#command-line-options).) with the following available resources:

| Resource | Supported methods |
| - | - |
| v1/shelf | GET |
| v1/shelf/enabled | GET, PUT |
| v1/shelf/brightness | GET, PUT |
| v1/shelf/averageColor | GET, PUT |
| v1/shelf/animating | GET, PUT |
| v1/squares | GET, PUT |
| v1/square/:index/averageColor | GET, PUT |

Data is returned and accepted in [JSON](https://www.json.org/) format.

See the [system diagram](#architecture) to understand the numbering of squares for square indices.

The HTTP REST API is used by the included [`hyelichtctl`](#hyelichtctl-cli-utility) command line frontend and [diyHue integration plugin](#diyhue-integration-plugin).

***

## hyelichtctl CLI utility

The `hyelichtctl` command line utility can be used to check on and control various aspects of the shelf. It is a client to the [HTTP REST API](#http-rest-api) of the [onboard application](#architecture) and can therefore run both locally and remotely, depending on the onboard configuration.

`hyelichtctl` is run like this:

    $ hyelichtctl [options] command [args...]

Supported commands are:

| Command | Parameters | Description
| - | - | - |
| status | none | Returns shelf status, including enabled, brightness, average color, etc. |
| enabled | [bool] | Reads or sets enabled status of the shelf. |
| enable | none | Enables the shelf. |
| disable | none | Disables the shelf. |
| brightness | [0.0 - 1.0] | Reads or sets the shelf brightness. |
| color | [square index] [color] | Reads or sets the color of the shelf or a square. |
| animating | [bool] | Starts or stops the animation. |

See the [system diagram](#architecture) to understand the numbering of squares for **square index**.

A **color** can be a CSS color name or a RGB HEX color code.

Supported command line options are:

| Option | Default | Description
| - | - | - |
| **-s, --server** | **127.0.0.1** | The server address to contact the onboard application at. |
| **-p, --port** | **8082** | The port to contact the onboard application on. |
| **-j, --json** | **UNSET** | If set, the output is in JSON format instead of more easily-read INI-style. |

Additionally, standard command line options such as **-h, --help** are supported as well.
