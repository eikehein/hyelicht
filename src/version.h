/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

#pragma once

#include <KAboutData>
#include <KLocalizedString>

namespace Hyelicht {
    inline KAboutData createAboutData(const QString &appName, const QString &description)
    {
        KAboutData aboutData {appName,
            xi18nc("@title", "<application>%1</application>", appName),
            QStringLiteral(HYELICHT_VERSION),
            description,
            KAboutLicense::Unknown,
            i18nc("@info:credit", "(c) 2021-2024 Hyerim and Eike"),
            QStringLiteral(),
            QStringLiteral("https://www.hyerimandeike.com/"),
            QStringLiteral("sho@eikehein.com")};

        aboutData.addLicense(KAboutLicense::GPL_V2, KAboutLicense::OrLaterVersions);

        aboutData.addAuthor(xi18nc("@info:credit", "Eike Hein"),
            xi18nc("@info:credit", "Lead Developer"), QStringLiteral("sho@eikehein.com"));
        aboutData.addAuthor(xi18nc("@info:credit", "Hyerim Jang"),
            xi18nc("@info:credit", "QA Lead"), QStringLiteral("huilin702@gmail.com"));

        return aboutData;
    }
}
