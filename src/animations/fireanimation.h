/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>
 */

#pragma once

#include "abstractanimation.h"

#include <QColor>

#include <algorithm>
#include <random>

//! Simple fire animation to turn the shelf into a digital fireplace
/*!
 * \ingroup Animation
 *
 * Animates every LED in the LedStrip instance, rather than the compartments of the shelf.
 *
 * \sa AbstractAnimation
 * \sa QTimeLine
 * \sa ShelfModel
 */
class FireAnimation : public AbstractAnimation
{
    Q_OBJECT

    public:
        //! Create a fire animation.
        /*!
        * @param parent Parent object
        */
        explicit FireAnimation(QObject *parent = nullptr);
        ~FireAnimation() override;

        //! The name of this animation.
        /*!
        * @return "Fire".
        */
        QString name() const override;

    private:
        QColor m_baseColor;
        bool m_skipFrame;


    std::random_device m_rd;
    std::mt19937 m_e;
    std::uniform_int_distribution<int> m_distColor;
    std::uniform_int_distribution<int> m_distInterval;
};
