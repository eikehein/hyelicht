/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

#include "abstractanimation.h"
#include "debug_animations.h"

AbstractAnimation::AbstractAnimation(QObject *parent)
    : QTimeLine(1000, parent)
{
    // Our animations run forever by default.
    setLoopCount(0);
}

AbstractAnimation::~AbstractAnimation() noexcept
{
}

LedStrip *AbstractAnimation::ledStrip() const
{
    return m_ledStrip;
}

void AbstractAnimation::setLedStrip(LedStrip *ledStrip)
{
    if (m_ledStrip != ledStrip) {
        m_ledStrip = ledStrip;

        Q_EMIT ledStripChanged();
    }
}
