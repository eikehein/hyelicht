#! /bin/sh

# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>

$XGETTEXT `find . -name \*.cpp -o -name \*.qml -o -name \*.js` -o $podir/hyelicht.pot
