/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QPointer>
#include <QQmlParserStatus>
#include <QRemoteObjectHost>
#include <QUrl>
#include <QVariantAnimation>

#include "abstractanimation.h"
#include "ledstrip.h"

//! Data model and business logic specific to the Hyelicht shelf
/*!
 * \ingroup Backend
 *
 * The Hyelicht shelf is a 5x5 IKEA Kallax shelf, of which the top 4 rows have
 * LED backlighting of 104 LEDs each. This data model maps the 416 LEDs to rows
 * in a \c QAbstractListModel, each row representing one compartment in the
 * shelf.
 *
 * In addition to this mapping the extended API of the model provides painting
 * operations and sophisticated application behaviors on top of LedStrip.
 *
 * ShelfModel with the \ref remotingEnabled property enabled can act as an API
 * server for instances of RemoteShelfModel, which act as client, either out of
 * process or over the network.
 * This allows running the onboard GUI out of process and also enables the
 * PC/Android offboard instances of the application.
 *
 * Communication between RemoteShelfModel and ShelfModel is implemented using
 * [Qt Remote Objects](https://doc.qt.io/qt-5/qtremoteobjects-index.html).
 *
 * Implements \c QQmlParserStatus for use from QML.
 *
 * \sa LedStrip
 * \sa AbstractAnimation
 * \sa QAbstractListModel
 * \sa QQmlParserStatus
 */
class ShelfModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    //! LedStrip instance this model operates on.
    /*!
    * LedStrip::enabled is not required to be \c true in order to use the model.
    *
    * Defaults to \c nullptr.
    *
    * \sa setLedStrip
    * \sa ledStripChanged
    */
    Q_PROPERTY(LedStrip* ledStrip READ ledStrip WRITE setLedStrip NOTIFY ledStripChanged)

    //! Toggle the shelf on or off.
    /*!
    * When turned on, the shelf brightness is set to the current value of \ref brightness.
    *
    * When turned off, the shelf brightness is set to \c 0 (without changing \ref brightness).
    *
    * Defaults to \c false.
    *
    * \sa setEnabled
    * \sa enabledChanged
    */
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

    //! Number of boards in the shelf.
    /*!
    * Cannot be lower than 1.
    *
    * Defaults to \c 4.
    *
    * \sa setRows
    * \sa rowsChanged
    * \sa columns
    * \sa density
    * \sa wallThickness
    */
    Q_PROPERTY(int rows READ rows WRITE setRows NOTIFY rowsChanged)

    //! Number of compartments in each shelf board.
    /*!
    * Cannot be lower than \c 1.
    *
    * Defaults to \c 5.
    *
    * \sa setColumns
    * \sa columnsChanged
    * \sa rows
    * \sa density
    * \sa wallThickness
    */
    Q_PROPERTY(int columns READ columns WRITE setColumns NOTIFY columnsChanged)

    //! Number of LEDs in each shelf compartment.
    /*!
    * Cannot be lower than \c 1.
    *
    * Defaults to \c 20.
    *
    * \sa setDensity
    * \sa densityChanged
    * \sa rows
    * \sa columns
    * \sa wallThickness
    */
    Q_PROPERTY(int density READ density WRITE setDensity NOTIFY densityChanged)

    //! Number of LEDs behind each compartment-dividing wall.
    /*!
    * The application will turn these off most of the time, in order to improve light bleed.
    *
    * Cannot be lower than \c 0.
    *
    * Defaults to \c 1.
    *
    * \sa setWallThickness
    * \sa wallThicknessChanged
    * \sa rows
    * \sa columns
    * \sa density
    */
    Q_PROPERTY(int wallThickness READ wallThickness WRITE setWallThickness NOTIFY wallThicknessChanged)

    //! The shelf brightness level while on.
    /*!
    * Shelf brightness is set in a range between \c 0.0 and \c 1.0.
    *
    * This property is independent of the value of the property \ref enabled.
    *
    * Defaults to \c 1.0.
    *
    * \sa setBrightness
    * \sa brightnessChanged
    */
    Q_PROPERTY(qreal brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)

    //! Toggle animated transitions between brightness levels.
    /*!
    * Defaults to \c true.
    *
    * \sa setAnimateBrightnessTransitions
    * \sa animateBrightnessTransitionsChanged
    * \sa transitionDuration
    * \sa brightness
    */
    Q_PROPERTY(bool animateBrightnessTransitions READ animateBrightnessTransitions
        WRITE setAnimateBrightnessTransitions NOTIFY animateBrightnessTransitionsChanged)

    //! Average color of the shelf.
    /*!
    * While animating, this is the average color of all LEDs in the \ref ledStrip. Otherwise,
    * it is only the color average of LEDs found in shelf compartments (in the Hyelicht shelf,
    * some LEDs are located behind divider walls and usually turned off to reduce color bleed).
    *
    * If \ref ledStrip is not set, this has the initial or the last set value.
    *
    * When set, this sets all LEDs found in shelf compartments to the given color.
    *
    * Defaults to \c white. The shelf is initialized to this value at application startup.
    *
    * \sa setAverageColor
    * \sa averageColorChanged
    */
    Q_PROPERTY(QColor averageColor READ averageColor WRITE setAverageColor NOTIFY averageColorChanged)

    //! Toggle animated transitions between full-shelf color fills.
    /*!
    * Defaults to \c true.
    *
    * \sa setAnimateAverageColorTransitions
    * \sa animateAverageColorTransitionsChanged
    * \sa transitionDuration
    * \sa averageColor
    */
    Q_PROPERTY(bool animateAverageColorTransitions READ animateAverageColorTransitions
        WRITE setAnimateAverageColorTransitions NOTIFY animateAverageColorTransitionsChanged)

    //! Duration in milliseconds for an animated fade between brightness levels or full-shelf color fills.
    /*!
    * The actual duration of a brightness fade is scaled by the delta between the old and the
    * new brightness levels, as a fraction of the full range of \c 0.0 - \c 1.0.
    *
    * Can be set to \c 0 to disable all animated fading and change to new brightness levels
    * or full-shelf color fills immediately instead.
    *
    * Defaults to \c 400.
    *
    * \sa setTransitionDuration
    * \sa transitionDurationChanged
    * \sa animateBrightnessTransitions
    * \sa animateAverageColorTransitions
    */
    Q_PROPERTY(int transitionDuration READ transitionDuration WRITE setTransitionDuration NOTIFY transitionDurationChanged)

    //! Animation to operate on \ref ledStrip.
    /*!
    * When set, the animation will be started or stopped based on the value of \ref animating.
    *
    * Should the animation be destroyed or set to \c nullptr, \ref animating is automatically set to \c false.
    *
    * Defaults to \c nullptr.
    *
    * \sa setAnimation
    * \sa animationChanged
    * \sa animating
    */
    Q_PROPERTY(AbstractAnimation* animation READ animation WRITE setAnimation NOTIFY animationChanged)

    //! Toggle the \ref animation.
    /*!
    * The animation will be paused when \ref enabled is false (without changing this property).
    *
    * Defaults to \c false.
    *
    * \sa setAnimating
    * \sa animatingChanged
    * \sa animation
    */
    Q_PROPERTY(bool animating READ animating WRITE setAnimating NOTIFY animatingChanged)

    //! Toggle the remoting API server.
    /*!
    * When enabled acts as an API server for instances of RemoteShelfModel, which act as
    * client, either out of process or over the network.
    * This allows running the onboard GUI out of process and also enables the
    * PC/Android offboard instances of the application.
    *
    * Defaults to \c true.
    *
    * \sa setRemotingEnabled
    * \sa remotingEnabledChanged
    * \sa listenAddress
    */
    Q_PROPERTY(bool remotingEnabled READ remotingEnabled WRITE setRemotingEnabled NOTIFY remotingEnabledChanged)

    //! Listen address for the remoting API server.
    /*!
    * Can be e.g. \c tcp:// or \c local:.
    *
    * Defaults to \c tcp://0.0.0.0:8042.
    *
    * \sa setListenAddress
    * \sa listenAddressChanged
    * \sa remotingEnabled
    */
    Q_PROPERTY(QUrl listenAddress READ listenAddress WRITE setListenAddress NOTIFY listenAddressChanged)

    public:
        //! Non-standard model data roles offered by this model.
        enum AdditionalRoles : int {
            AverageColor = Qt::UserRole + 1, //!< Average color of the LEDs in a shelf compartment.
            AverageRed,       //!< Average red channel of the LEDs in a shelf compartment.
            AverageGreen,     //!< Average green channel of the LEDs in a shelf compartment.
            AverageBlue,      //!< Average blue channel of the LEDs in a shelf compartment.
            AverageBrightness //!< Average brightness channel of the LEDs in a shelf compartment.
        };
        Q_ENUM(AdditionalRoles)

        //! Create a shelf model.
        /*!
        * @param parent Parent object
        */
        explicit ShelfModel(QObject *parent = nullptr);
        ~ShelfModel() override;

        //! The LedStrip instance this model operates on.
        /*!
        * @return ShelfModel instance.
        * \sa ledStrip (property)
        * \sa ledStrip
        * \sa ledStripChanged
        */
        LedStrip *ledStrip() const;

        //! Set the LedStrip instance this model operates on.
        /*!
        * LedStrip::enabled is not required to be \c true in order to use the model.
        *
        * @param ledStrip LedStrip instance.
        * \sa ledStrip
        * \sa ledStripChanged
        */
        void setLedStrip(LedStrip *ledStrip);

        //! Whether the shelf is on or off.
        /*!
        * When on, the shelf brightness is set to the current value of \ref brightness.
        *
        * When off, the shelf brightness is set to \c 0 (without changing \ref brightness).
        *
        * @return Shelf on or off.
        * \sa enabled (property)
        * \sa setEnabled
        * \sa enabledChanged
        * \sa brightness
        */
        bool enabled() const;

        //! Turn the shelf on or off.
        /*!
        * When turned on, the shelf brightness is set to the current value of \ref brightness.
        *
        * When turned off, the shelf brightness is set to \c 0 (without changing \ref brightness).
        *
        * @param enabled Shelf on or off.
        * \sa enabled
        * \sa enabledChanged
        * \sa brightness
        */
        void setEnabled(bool enabled);

        //! The number of boards in the shelf.
        /*!
        * @return Number of boards.
        * \sa rows (property)
        * \sa setRows
        * \sa rowsChanged
        * \sa columns
        * \sa density
        * \sa wallThickness
        */
        int rows() const;

        //! Set the number of boards in the shelf.
        /*!
        * Cannot be lower than 1.
        *
        * @param rows Number of boards.
        * \sa rows
        * \sa rowsChanged
        * \sa columns
        * \sa density
        * \sa wallThickness
        */
        void setRows(int rows);

        //! The number of compartments in each shelf board.
        /*!
        * @return Number of compartments.
        * \sa columns (property)
        * \sa setColumns
        * \sa columnsChanged
        * \sa rows
        * \sa density
        * \sa wallThickness
        */
        int columns() const;

        //! Set the number of compartments in each shelf board.
        /*!
        * Cannot be lower than 1.
        *
        * @param columns Number of compartments.
        * \sa columns
        * \sa columnsChanged
        * \sa rows
        * \sa density
        * \sa wallThickness
        */
        void setColumns(int columns);

        //! The number of LEDs in each shelf compartment.
        /*!
        * @return Number of LEDs.
        * \sa density (property)
        * \sa setDensity
        * \sa densityChanged
        * \sa rows
        * \sa columns
        * \sa wallThickness
        */
        int density() const;

        //! Set the number of LEDs in each shelf compartment.
        /*!
        * Cannot be lower than 1.
        *
        * @param density Number of LEDs.
        * \sa density
        * \sa densityChanged
        * \sa rows
        * \sa columns
        * \sa wallThickness
        */
        void setDensity(int density);

        //! The number of LEDs behind each compartment-dividing wall.
        /*!
        * The application will turn these off most of the time, in order to improve light bleed.
        *
        * @return Wall thickness in LEDs.
        * \sa wallThickness (property)
        * \sa setWallThickness
        * \sa wallThicknessChanged
        * \sa rows
        * \sa columns
        * \sa density
        */
        int wallThickness() const;

        //! Set the number of LEDs behind each compartment-dividing wall.
        /*!
        *
     * The application will turn these off most of the time, in order to improve light bleed.
        * Cannot be lower than 0.
        *
        * @param thickness Wall thickness in LEDs.
        * \sa wallThickness
        * \sa wallThicknessChanged
        * \sa rows
        * \sa columns
        * \sa density
        */
        void setWallThickness(int thickness);

        //! The shelf brightness level while on.
        /*!
        * This property is independent of the value of the property \ref enabled.
        *
        * @return Shelf brightness level between \c 0.0 and \c 1.0.
        * \sa brightness (property)
        * \sa setBrightness
        * \sa brightnessChanged
        * \sa enabled
        * \sa animateBrightnessTransitions
        */
        qreal brightness() const;

        //! Set the shelf brightness level while on.
        /*!
        * @param brightness Shelf brightness level between \c 0.0 and \c 1.0.
        * \sa brightness
        * \sa brightnessChanged
        * \sa enabled
        * \sa animateBrightnessTransitions
        */
        void setBrightness(qreal brightness);

        //! Whether to animate transitions between brightness levels.
        /*!
        * This property is independent of the value of the property \ref enabled.
        *
        * @return Brightness transitions on or off.
        * \sa animateBrightnessTransitions (property)
        * \sa setAnimateBrightnessTransitions
        * \sa animateBrightnessTransitionsChanged
        * \sa transitionDuration
        */
        bool animateBrightnessTransitions() const;

        //! Set whether to animate transitions between brightness levels.
        /*!
        * @param animate Brightness transitions on or off.
        * \sa animateBrightnessTransitions
        * \sa animateBrightnessTransitionsChanged
        * \sa transitionDuration
        */
        void setAnimateBrightnessTransitions(bool animate);

        //! The average color of the shelf.
        /*!
        * While animating, this is the average color of all LEDs in the \ref ledStrip. Otherwise,
        * it is only the color average of LEDs found in shelf compartments (in the Hyelicht shelf,
        * some LEDs are located behind divider walls and usually turned off to reduce color bleed).
        *
        * If \ref ledStrip is not set, this has the initial or the last set value.
        *
        * @return Average shelf color.
        * \sa averageColor (property)
        * \sa setAverageColor
        * \sa averageColorChanged
        * \sa animateAverageColorTransitions
        */
        QColor averageColor() const;

        //! Sets the average color of the shelf.
        /*!
        * When set, this sets all LEDs found in shelf compartments to the given color.
        *
        * @param color Color to fill with.
        * \sa averageColor
        * \sa averageColorChanged
        * \sa animateAverageColorTransitions
        */
        void setAverageColor(const QColor &color);

        //! Whether to animate transitions between full-shelf color fills.
        /*!
        * This property is independent of the value of the property \ref enabled.
        *
        * @return Full-shelf color fill transitions on or off.
        * \sa animateAverageColorTransitions (property)
        * \sa setAnimateAverageColorTransitions
        * \sa animateAverageColorTransitionsChanged
        * \sa transitionDuration
        */
        bool animateAverageColorTransitions() const;

        //! Set whether to animate transitions between full-shelf color fills.
        /*!
        * @param animate Full-shelf color fill transitions on or off.
        * \sa animateAverageColorTransitions
        * \sa animateAverageColorTransitionsChanged
        * \sa transitionDuration
        */
        void setAnimateAverageColorTransitions(bool animate);

        //! The duration in milliseconds for an animated fade between brightness levels or full-shelf color fills has changed.
        /*!
        * This property is independent of the value of the property \ref enabled.
        *
        * @return Full-shelf color fill transitions on or off.
        * \sa transitionDuration (property)
        * \sa setTransitionDuration
        * \sa transitionDurationChanged
        * \sa animateAverageColorTransitions
        * \sa animateBrightnessTransitions
        */
        int transitionDuration() const;

        //! Set the duration in milliseconds for an animated fade between brightness levels or full-shelf color fills has changed.
        /*!
        * The actual duration of a brightness fade is scaled by the delta between the old and the
        * new brightness levels, as a fraction of the full range of \c 0.0 - \c 1.0.
        *
        * Can be set to \c 0 to disable all animated fading and change to new brightness levels
        * or full-shelf color fills immediately instead.
        *
        * @param duration Brightness transitions on or off.
        * \sa transitionDuration
        * \sa transitionDurationChanged
        * \sa animateAverageColorTransitions
        * \sa animateBrightnessTransitions
        */
        void setTransitionDuration(int duration);

        //! The animation operating on \ref ledStrip.
        /*!
        * @return An AbstractAnimation.
        * \sa animation (property)
        * \sa setAnimation
        * \sa animationChanged
        * \sa animating
        */
        AbstractAnimation *animation() const;

        //! Set animation operating on \ref ledStrip.
        /*!
        * The animation will be started or stopped based on the value of \ref animating.
        *
        * Should the animation be destroyed or set to \c nullptr, \ref animating is automatically set to \c false.
        *
        * @param animation An AbstractAnimation.
        * \sa animation
        * \sa animationChanged
        * \sa animating
        */
        void setAnimation(AbstractAnimation *animation);

        //! Whether to run the \ref animation.
        /*!
        * The animation will be paused when \ref enabled is false (without changing this property).
        *
        * @return Animation on or off.
        * \sa animating (property)
        * \sa setAnimating
        * \sa animatingChanged
        * \sa animation
        */
        bool animating() const;

        //! Set whether to run the \ref animation.
        /*!
        * The animation will be paused when \ref enabled is false (without changing this property).
        *
        * @param animating Animation on or off.
        * \sa animating
        * \sa animatingChanged
        * \sa animation
        */
        void setAnimating(bool animating);

        //! Whether to enable the remoting API server.
        /*!
        * @return Server on or off.
        * \sa remotingEnabled (property)
        * \sa setRemotingEnabled
        * \sa remotingEnabledChanged
        * \sa listenAddress
        */
        bool remotingEnabled() const;

        //! Set whether to enable the remoting API server.
        /*!
        * When enabled acts as an API server for instances of RemoteShelfModel, which act as
        * client, either out of process or over the network.
        * This allows running the onboard GUI out of process and also enables the
        * PC/Android offboard instances of the application.
        *
        * @param enabled Server on or off.
        * \sa remotingEnabled
        * \sa remotingEnabledChanged
        * \sa listenAddress
        */
        void setRemotingEnabled(bool enabled);

        //! The listen address for the remoting API server.
        /*!
        * @return Animation on or off.
        * \sa listenAddress (property)
        * \sa setListenAddress
        * \sa listenAddressChanged
        * \sa remotingEnabled
        */
        QUrl listenAddress() const;

        //! Set the listen address for the remoting API server.
        /*!
      * Can be e.g. \c tcp:// or \c local:.
        *
        * @param url Animation on or off.
        * \sa listenAddress
        * \sa listenAddressChanged
        * \sa remotingEnabled
        */
        void setListenAddress(const QUrl &url);

        //! \sa \c QAbstractItemModel::roleNames
        QHash<int, QByteArray> roleNames() const override;

        //! \sa \c QAbstractItemModel::rowCount
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;

        //! \sa \c QAbstractItemModel::headerData
        QVariant headerData(int section, Qt::Orientation orientation,
            int role = Qt::DisplayRole) const override;

        /*!
        * \sa AdditionalRoles
        * \sa \c QAbstractItemModel::data
        */
        QVariant data(const QModelIndex &proxyIndex, int role) const override;

        /*!
        * \sa AdditionalRoles
        * \sa \c QAbstractItemModel::data
        */
        bool setData(const QModelIndex &index, const QVariant &value,
            int role = Qt::EditRole) override;

        //! Implements the \c QQmlParserStatus interface.
        void classBegin() override;
        //! Implements the \c QQmlParserStatus interface.
        void componentComplete() override;

    Q_SIGNALS:
        //! The LedStrip instance this model operates on has changed.
        /*!
        * \sa ledStrip
        * \sa setLedStrip
        */
        void ledStripChanged() const;

        //! The shelf has turned on or off.
        /*!
        * @param enabled Shelf on or off.
        * \sa enabled
        * \sa setEnabled
        */
        void enabledChanged(bool enabled) const;

        //! The number of boards in the shelf has changed.
        /*!
        * @param rows Number of boards.
        * \sa rows
        * \sa setRows
        */
        void rowsChanged(int rows) const;

        //! The number of compartments in each shelf board has changed.
        /*!
        * @param columns Number of compartments.
        * \sa columns
        * \sa setColumns
        */
        void columnsChanged(int columns) const;

        //! The number of LEDs in each shelf compartment has changed.
        /*!
        * @param density Number of LEDs.
        * \sa density
        * \sa setDensity
        */
        void densityChanged(int density) const;

        //! The number of LEDs behind each compartment-dividing wall has changed.
        /*!
        * @param thickness Wall thickness in LEDs.
        * \sa wallThickness
        * \sa setWallThickness
        */
        void wallThicknessChanged(int thickness) const;

        //! The brightness of the shelf has changed.
        /*!
        * @param brightness Shelf brightness level between \c 0.0 and \c 1.0.
        * \sa brightness
        * \sa setBrightness
        */
        void brightnessChanged(qreal brightness) const;

        //! Whether to animate transitions between brightness levels has changed.
        /*!
        @param animate Brightness transitions on or off.
        * \sa animateBrightnessTransitions
        * \sa setAnimateBrightnessTransitions
        */
        void animateBrightnessTransitionsChanged(bool animate) const;

        //! The average color of the shelf has changed.
        /*!
        * @param color Color to fill with.
        * \sa averageColor
        * \sa setAverageColor
        */
        void averageColorChanged(const QColor &color) const;

        //! Whether to animate transitions between full-shelf color fills has changed.
        /*!
        * @param animate Full-shelf color fill transitions on or off.
        * \sa animateAverageColorTransitions
        * \sa setAnimateAverageColorTransitions
        */
        void animateAverageColorTransitionsChanged(bool animate) const;

        //! The duration in milliseconds for an animated fade between brightness levels or full-shelf color fills has changed.
        /*!
        * @param duration Brightness transitions on or off.
        * \sa transitionDuration
        * \sa setTransitionDuration
        */
        void transitionDurationChanged(int duration) const;

        //! The animation operating on \ref ledStrip has changed.
        /*!
        * \sa animation
        * \sa setAnimation
        */
        void animationChanged() const;

        //! Whether to run the \ref animation has changed.
        /*!
        * @param animating Animation on or off.
        * \sa animating
        * \sa setAnimating
        */
        void animatingChanged(bool animating) const;

        //! Whether to enable the remoting API server has changed.
        /*!
        * \sa remotingEnabled
        * \sa setRemotingEnabled
        */
        void remotingEnabledChanged() const;

        //! The listen address for the remoting API server has changed.
        /*!
        * \sa listenAddress
        * \sa setEnabled
        */
        void listenAddressChanged() const;

    private:
        inline QPair<int, int> rowIndexToRange(const int rowIndex) const;
        void transitionToCurrentBrightness();
        void syncBrightness(bool show = true);
        void setRangesToColor(const QColor &color);
        void abortTransitions();
        void updateLedStrip();
        void updateAnimation();
        void updateRemoting();

        QPointer<LedStrip> m_ledStrip;

        bool m_enabled;

        int m_rows;
        int m_columns;
        int m_density;
        int m_wallThickness;

        qreal m_brightness;
        qreal m_targetBrightness;
        bool m_animateBrightnessTransitions;
        bool m_pendingBrightnessTransition;
        QVariantAnimation m_brightnessTransition;

        QColor m_averageColor;
        bool m_animateAverageColorTransitions;
        QVariantAnimation m_averageColorTransition;

        int m_transitionDuration;

        QPointer<AbstractAnimation> m_animation;
        bool m_animating;

        bool m_remotingEnabled;
        QUrl m_listenAddress;
        QRemoteObjectHost *m_remotingServer;

        bool m_createdByQml;
        bool m_complete;
};
