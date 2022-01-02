/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR
 * LicenseRef-KDE-Accepted-GPL SPDX-FileCopyrightText: 2021-2022 Eike Hein
 * <sho@eikehein.com>
 */

#pragma once

#include <QIdentityProxyModel>
#include <QQmlParserStatus>
#include <QUrl>

class QAbstractItemModelReplica;
class QRemoteObjectDynamicReplica;
class QRemoteObjectNode;

//! Client for the remoting server provided by ShelfModel
/*!
 * \ingroup Backend
 *
 * Connects to the remoting server provided by a ShelfModel instance and makes
 * most of the ShelfModel API available out of process or over the network.
 * This allows running the onboard GUI out of process and also enables the 
 * PC/Android offboard instances of the application.
 *
 * For detailed documentation of the mirrored API please see ShelfModel.
 *
 * Communication between RemoteShelfModel and ShelfModel is implemented using
 * [Qt Remote Objects](https://doc.qt.io/qt-5/qtremoteobjects-index.html).
 *
 * Implements \c QQmlParserStatus for use from QML.
 *
 * \sa ShelfModel
 * \sa QAbstractListModel
 * \sa QQmlParserStatus
 */
class RemoteShelfModel : public QIdentityProxyModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    //! Whether there is a healthy connection to a ShelfModel.
    /*!
    *
    * \sa serverAddress
    */
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

    //! Address used to connect to a ShelfModel instance.
    /*!
    * Can be e.g. \c tcp:// or \c local:.
    *
    * Defaults to \c tcp://192.168.178.129:8042.
    *
    * \sa setServerAddress
    * \sa serverAddressChanged
    */
    Q_PROPERTY(QUrl serverAddress READ serverAddress WRITE setServerAddress NOTIFY serverAddressChanged)

    //! \sa ShelfModel::enabled
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

    //! \sa ShelfModel::rows
    Q_PROPERTY(int rows READ rows WRITE setRows NOTIFY rowsChanged)
    //! \sa ShelfModel::columns
    Q_PROPERTY(int columns READ columns WRITE setColumns NOTIFY columnsChanged)
    //! \sa ShelfModel::density
    Q_PROPERTY(int density READ density WRITE setDensity NOTIFY densityChanged)
    //! \sa ShelfModel::wallThickness
    Q_PROPERTY(int wallThickness READ wallThickness WRITE setWallThickness NOTIFY wallThicknessChanged)
    
    //! \sa ShelfModel::brightness
    Q_PROPERTY(qreal brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    //! \sa ShelfModel::animateBrightnessTransitions
    Q_PROPERTY(bool animateBrightnessTransitions READ animateBrightnessTransitions
        WRITE setAnimateBrightnessTransitions NOTIFY animateBrightnessTransitionsChanged)
    
    //! \sa ShelfModel::averageColor
    Q_PROPERTY(QColor averageColor READ averageColor WRITE setAverageColor NOTIFY averageColorChanged)
    //! \sa ShelfModel::animateAverageColorTransitions
    Q_PROPERTY(bool animateAverageColorTransitions READ animateAverageColorTransitions
        WRITE setAnimateAverageColorTransitions NOTIFY animateAverageColorTransitionsChanged)

    //! \sa ShelfModel::transitionDuration
    Q_PROPERTY(int transitionDuration READ transitionDuration WRITE setTransitionDuration NOTIFY transitionDurationChanged)
    
    //! \sa ShelfModel::animating
    Q_PROPERTY(bool animating READ animating WRITE setAnimating NOTIFY animatingChanged)

    public:
        //! Create a remote client to a ShelfModel.
        /*!
        * @param parent Parent object
        */
        explicit RemoteShelfModel(QObject *parent = nullptr);
        ~RemoteShelfModel() override;

        //! Whether there is a healthy connection to a ShelfModel.
        /*!
        * @return Connected or not.
        * \sa serverAddress
        */
        bool connected() const;

        //! The address used to connect to a ShelfModel instance.
        /*!
        * @return Server address, e.g. \c tcp:// or \c local:.
        * \sa serverAddress (property)
        * \sa setServerAddress
        * \sa serverAddressChanged
        */
        QUrl serverAddress() const;

        //! Set the address used to connect to a ShelfModel instance.
        /*!
        * @param url Server address, e.g. \c tcp:// or \c local:.
        * \sa serverAddress
        * \sa serverAddressChanged
        */
        void setServerAddress(const QUrl &url);

        //! \sa ShelfModel::enabled
        bool enabled() const;
        //! \sa ShelfModel::setEnabled
        void setEnabled(bool enabled);

        //! \sa ShelfModel::rows
        int rows() const;
        //! \sa ShelfModel::setRows
        void setRows(int rows);

        //! \sa ShelfModel::columns
        int columns() const;
        //! \sa ShelfModel::setColumns
        void setColumns(int columns);

        //! \sa ShelfModel::density
        int density() const;

        //! \sa ShelfModel::setDensity
        void setDensity(int density);

        //! \sa ShelfModel::wallThickness
        int wallThickness() const;
        //! \sa ShelfModel::setWallThickness
        void setWallThickness(int thickness);

        //! \sa ShelfModel::brightness
        qreal brightness() const;
        //! \sa ShelfModel::setBrightness
        void setBrightness(qreal brightness);
        //! \sa ShelfModel::animateBrightnessTransitions
        bool animateBrightnessTransitions() const;
        //! \sa ShelfModel::setAnimateBrightnessTransitions
        void setAnimateBrightnessTransitions(bool animate);

        //! \sa ShelfModel::averageColor
        QColor averageColor() const;
        //! \sa ShelfModel::setAverageColor
        void setAverageColor(const QColor &color);
        //! \sa ShelfModel::animateAverageColorTransitions
        bool animateAverageColorTransitions() const;
        //! \sa ShelfModel::setAnimateAverageColorTransitions
        void setAnimateAverageColorTransitions(bool animate);

        //! \sa ShelfModel::transitionDuration
        int transitionDuration() const;
        //! \sa ShelfModel::setTransitionDuration
        void setTransitionDuration(int duration);

        //! \sa ShelfModel::animating
        bool animating() const;
        //! \sa ShelfModel::setAnimating
        void setAnimating(bool animating);

        //! Implements the \c QQmlParserStatus interface.
        void classBegin() override;
        //! Implements the \c QQmlParserStatus interface.
        void componentComplete() override;

    Q_SIGNALS:
        //! Whether there is a healthy connection to a ShelfModel has changed.
        /*!
        * \sa connected
        */
        void connectedChanged() const;

        //! The address used to connect to a ShelfModel instance has changed.
        /*!
        * \sa serverAddress
        * \sa setServerAddress
        */
        void serverAddressChanged() const;

        //! \sa ShelfModel::enabledChanged
        void enabledChanged() const;

        //! \sa ShelfModel::rowsChanged
        void rowsChanged() const;
        //! \sa ShelfModel::columnsChanged
        void columnsChanged() const;
        //! \sa ShelfModel::densityChanged
        void densityChanged() const;
        //! \sa ShelfModel::wallThicknessChanged
        void wallThicknessChanged() const;

        //! \sa ShelfModel::shelfRowsChanged
        void shelfRowsChanged() const;
        //! \sa ShelfModel::shelfColumnsChanged
        void shelfColumnsChanged() const;

        //! \sa ShelfModel::brightnessChanged
        void brightnessChanged() const;
        //! \sa ShelfModel::animateBrightnessTransitionsChanged
        void animateBrightnessTransitionsChanged() const;

        //! \sa ShelfModel::averageColorChanged
        void averageColorChanged() const;
        //! \sa ShelfModel::animateAverageColorTransitionsChanged
        void animateAverageColorTransitionsChanged() const;

        //! \sa ShelfModel::transitionDurationChanged
        void transitionDurationChanged() const;

        //! \sa ShelfModel::animatingChanged
        void animatingChanged() const;

    private:
        void updateSource();

        bool m_enabled;

        QUrl m_serverAddress;
        QRemoteObjectNode *m_remotingServer;
        QAbstractItemModelReplica *m_remoteModel;
        QRemoteObjectDynamicReplica *m_remoteModelApi;

        bool m_createdByQml;
        bool m_complete;
};
