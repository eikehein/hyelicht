/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>
 */

#include "fireanimation.h"
#include "debug_animations.h"
#include "ledstrip.h"

#include <KLocalizedString>

FireAnimation::FireAnimation(QObject *parent)
    : AbstractAnimation(parent)
    , m_baseColor {255, 96, 12}
    , m_skipFrame {false}
    , m_e {m_rd()}
    , m_distColor {0, 100}
    , m_distInterval {40, 60}
{
    QObject::connect(this, &AbstractAnimation::valueChanged, this,
        [=](qreal value) {
            Q_UNUSED(value)

            if (!m_ledStrip) {
                m_skipFrame = false;
                stop();
                return;
            }

            if (m_skipFrame) {
                m_skipFrame = false;
                return;
            }

            for (int i {0}; i < m_ledStrip->count(); ++i) {
                const int flicker {m_distColor(m_e)};

                m_ledStrip->setColor(i, {
                        std::max(0, m_baseColor.red() - flicker),
                        std::max(0, m_baseColor.green() - flicker),
                        std::max(0, m_baseColor.blue() - flicker)
                    }
                );
            }

            m_ledStrip->show();
            emit frameComplete();

            blockSignals(true);
            setUpdateInterval(m_distInterval(m_e));
            m_skipFrame = true;
            stop();
            start();
            blockSignals(false);
        }
    );
}

FireAnimation::~FireAnimation()
{
}

QString FireAnimation::name() const
{
    return i18n("Fire");
}
