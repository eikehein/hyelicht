# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2024 Eike Hein <sho@eikehein.com>

import info
from CraftCore import CraftCore
from Package.CMakePackageBase import CMakePackageBase

class subinfo(info.infoclass):
    def setTargets(self):
        self.displayName = "heylicht"
        self.description = "Controller for the Hyelicht shelf"
        self.svnTargets["main"] = "https://github.com/eikehein/hyelicht.git|main"
        self.defaultTarget = "main"

    def setDependencies(self):
        self.runtimeDependencies["virtual/base"] = None
        self.runtimeDependencies["libs/qt6/qtbase"] = None
        self.runtimeDependencies["libs/qt6/qtdeclarative"] = None
        self.runtimeDependencies["libs/qt6/qtremoteobjects"] = None
        self.runtimeDependencies["kde/frameworks/tier1/kcoreaddons"] = None
        self.runtimeDependencies["kde/frameworks/tier1/ki18n"] = None
        self.runtimeDependencies["kde/frameworks/tier1/kconfig"] = None

        if not CraftCore.compiler.isAndroid:
            self.runtimeDependencies["libs/qt6/qthttpserver"] = None
            self.runtimeDependencies["libs/qt6/qtserialport"] = None
        #else:
        #    self.runtimeDependencies["libs/qt6/qtsvg"] = None

class Package(CMakePackageBase):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def createPackage(self):
        self.defines["executable"] = r"bin\hyelicht.exe"

        if not CraftCore.compiler.isLinux:
            self.ignoredPackages.append("libs/dbus")

        return super().createPackage()
