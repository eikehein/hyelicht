/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

#pragma once

#include <QTimeLine>
#include <QPointer>

#include "ledstrip.h"

//! Abstract base class for LED strip animations operating on LedStrip
/*!
 * \ingroup Animation
 *
 * Extends QTimeLine with useful defaults and member-based access to a LedStrip instance.
 *
 * AbstractAnimations are set on a ShelfModel instance by calling its ShelfModel::setAnimation method.
 *
 * \attention AbstractAnimations must provide a \ref name and emit the \ref frameComplete signal.
 *
 * AbstractAnimations loop forever by default.
 *
 * \sa ShelfModel
 * \sa QTimeLine
 */
class AbstractAnimation : public QTimeLine
{
    Q_OBJECT

    //! Name of this animation.
    /*!
    * Subclasses must implement this to provide an animation name.
    */
    Q_PROPERTY(QString name READ name CONSTANT)

    //! LedStrip this animation operates on.
    /*!
    * \sa setLedStrip
    * \sa ledStripChanged
    */
    Q_PROPERTY(LedStrip* ledStrip READ ledStrip WRITE setLedStrip NOTIFY ledStripChanged)

    public:
        //! Create an animation.
        /*!
        * @param parent Parent object
        */
        explicit AbstractAnimation(QObject *parent = nullptr);
        virtual ~AbstractAnimation() override;

        //! The name of this animation.
        /*!
        * @return Animation name.
        * \sa name (property)
        */
        virtual QString name() const = 0;

        //! The LedStrip this animation operates on.
        /*!
        * Defaults to \c nullptr.
        *
        * @return A LedStrip.
        * \sa ledStrip (property)
        * \sa setLedStrip
        * \sa ledStripChanged
        */
        LedStrip *ledStrip() const;

        //! Set the LedStrip this animation operates on.
        /*!
        * @param ledStrip A LedStrip.
        * \sa ledStrip
        * \sa ledStripChanged
        */
        void setLedStrip(LedStrip *ledStrip);

    Q_SIGNALS:
        //! The LedStrip this animation operates on has changed.
        /*!
        * \sa ledStrip
        * \sa setLedStrip
        */
        void ledStripChanged() const;

        //! Subclasses must emit this signal after they have finished painting a frame.
        /*!
        * A subclass will commonly perform painting operations on its \ref ledStrip in
        * response to a \c QTimeLine::valueChanged signal. After painting is concluded
        * for a frame it must emit this signal.
        */
        void frameComplete() const;

    protected:
        QPointer<LedStrip> m_ledStrip; //!< LedStrip instance to operate on.
};
