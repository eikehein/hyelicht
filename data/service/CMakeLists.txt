# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>

ecm_install_configured_files(
  INPUT hyelicht.service.in
  DESTINATION ${KDE_INSTALL_SYSTEMDUNITDIR}/system
  DESTINATION ${KDE_INSTALL_SYSTEMDUNITDIR}/user
)
