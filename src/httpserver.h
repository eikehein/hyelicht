/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>
 */

#pragma once

#include <QHostAddress>
#include <QObject>
#include <QPointer>
#include <QQmlParserStatus>

class ShelfModel;

#ifdef HYELICHT_BUILD_ONBOARD
namespace QHttpEngine {
    class QObjectHandler;
    class Server;
    class Socket;
}
#endif

//! HTTP REST API binding and server for ShelfModel
/*!
 * \ingroup Backend
 *
 * Implements a read-write HTTP REST API binding and server around the data model
 * and business logic of a ShelfModel instance.
 *
 * The implementation makes use of the [QHttpEngine](https://github.com/nitroshare/qhttpengine)
 * library.
 *
 * In the Hyelicht project, the HTTP REST API is used by the included `hyelichtctl`
 * CLI frontend utility and the [diyHue](https://diyhue.org/) integration plugin.
 *
 * \sa ShelfModel
 * \sa QHttpEngine::Server
 * \sa QHttpEngine::QObjectHandler
 * \sa QQmlParserStatus
 */
class HttpServer : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    //! Toggle the server on or off.
    /*!
    * Defaults to \c false.
    *
    * \sa setEnabled
    * \sa enabledChanged
    * \sa listenAddress
    * \sa port
    */
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

    //! Listen address for the server.
    /*!
    * Defaults to \c 127.0.0.1.
    *
    * \sa setListenAddress
    * \sa listenAddressChanged
    * \sa port
    */
    Q_PROPERTY(QString listenAddress READ listenAddress WRITE setListenAddress NOTIFY listenAddressChanged)

    //! Port the server listens on.
    /*!
    * Defaults to \c 8082.
    *
    * \sa setPort
    * \sa portChanged
    * \sa listenAddress
    */
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)

    //! ShelfModel instance this server provides a HTTP REST API binding for.
    /*!
    * Needs to be set to a valid instance for the REST API to be available on the server.
    *
    * Defaults to \c nullptr.
    *
    * \sa setModel
    * \sa modelChanged
    */
    Q_PROPERTY(ShelfModel* model READ model WRITE setModel NOTIFY modelChanged)

    public:
        //! Create a HTTP REST API server.
        /*!
        * @param parent Parent object
        */
        explicit HttpServer(QObject *parent = nullptr);
        ~HttpServer() override;

        //! Whether the server is on or off.
        /*!
        * @return Server on or off.
        * \sa enabled (property)
        * \sa setEnabled
        * \sa enabledChanged
        */ 
        bool enabled() const;

        //! Turn the server on or off.
        /*!
        * @param enabled Server on or off.
        * \sa enabled
        * \sa enabledChanged
        */
        void setEnabled(bool enabled);

        //! The listen address for the server.
        /*!
        * @return Listen address.
        * \sa listenAddress (property)
        * \sa setListenAddress
        * \sa listenAddressChanged
        * \sa port
        */ 
        QString listenAddress() const;

        //! Set the listen address for the server.
        /*!
        * @param listenAddress Listen address.
        * \sa listenAddress
        * \sa listenAddressChanged
        * \sa port
        */
        void setListenAddress(const QString &listenAddress);

        //! The port the server listens on.
        /*!
        * @return Port number.
        * \sa port (property)
        * \sa setPort
        * \sa portChanged
        * \sa listenAddress
        */ 
        int port() const;

        //! Set the port the server listens on.
        /*!
        * @param port Port number.
        * \sa port
        * \sa portChanged
        * \sa listenAddress
        */
        void setPort(int port);

        //! The ShelfModel instance this server provides a HTTP REST API binding for.
        /*!
        * @return ShelfModel instance.
        * \sa model (property)
        * \sa setModel
        * \sa modelChanged
        */ 
        ShelfModel *model() const;

        //! Set the ShelfModel instance this server provides a HTTP REST API binding for.
        /*!
        * Needs to be set to a valid instance for the REST API to be available on the server.
        *
        * @param model ShelfModel instance.
        * \sa model
        * \sa modelChanged
        */
        void setModel(ShelfModel *model);

        //! Implements the \c QQmlParserStatus interface.
        void classBegin() override;
        //! Implements the \c QQmlParserStatus interface.
        void componentComplete() override;

    Q_SIGNALS:
        //! The server has turned on or off.
        /*!
        * \sa enabled
        * \sa setEnabled
        */
        void enabledChanged() const;

        //! The listen address for the server has changed.
        /*!
        * \sa listenAddress
        * \sa setListenAddress
        */
        void listenAddressChanged() const;

        //! The port the server listens on has changed.
        /*!
        * \sa port
        * \sa setPort
        */
        void portChanged() const;

        //! The ShelfModel instance this server provides a HTTP REST API binding for has changed.
        /*!
        * \sa model
        * \sa setModel
        */
        void modelChanged();

    private Q_SLOTS:
        void updateHandler();

    private:
        void updateServer();
#ifdef HYELICHT_BUILD_ONBOARD
        std::function<void(QHttpEngine::Socket *)> propHandler(const char *prop);
        std::function<void(QHttpEngine::Socket *)> modelRowHandler(const char *prop,
            const int index, const int role);
        void propToJSon(QHttpEngine::Socket *socket, const QObject *obj, const char *name);
        void jsonToProp(QHttpEngine::Socket *socket, QObject *obj, const char *name);
#endif
        QJsonObject rowToJson(const QModelIndex &modelIndex);
        QHash<int, QVariant> jsonToModelRole(const QJsonDocument &document,
            const QString &roleName);

        bool m_enabled;

        QString m_listenAddress;
        int m_port;

#ifdef HYELICHT_BUILD_ONBOARD
        QHttpEngine::QObjectHandler *m_handler;
        QHttpEngine::Server *m_server;
#endif

        QPointer<ShelfModel> m_model;

        bool m_createdByQml;
        bool m_complete;
};
