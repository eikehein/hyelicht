/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>
 */

#include "httpserver.h"
#include "debug_httpserver.h"
#include "shelfmodel.h"

#include <KLocalizedString>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaProperty>
#include <qnamespace.h>

#ifdef HYELICHT_BUILD_ONBOARD
#include <qhttpengine/qobjecthandler.h>
#include <qhttpengine/server.h>
#include <qhttpengine/socket.h>
#endif

HttpServer::HttpServer(QObject *parent)
    : QObject{parent}
    , m_enabled {false}
    , m_listenAddress {QStringLiteral("127.0.0.1")}
    , m_port {8082}
#ifdef HYELICHT_BUILD_ONBOARD
    , m_handler {new QHttpEngine::QObjectHandler {this}}
    , m_server {new QHttpEngine::Server {m_handler}}
#endif
    , m_model {nullptr}
    , m_createdByQml {false}
    , m_complete {false}
{
}

HttpServer::~HttpServer()
{
}

bool HttpServer::enabled() const
{
    return m_enabled;
}

void HttpServer::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;

        if (!m_createdByQml || m_complete) {
            updateServer();
        }

        emit enabledChanged();
    }
}

QString HttpServer::listenAddress() const
{
    return m_listenAddress;
}

void HttpServer::setListenAddress(const QString &listenAddress)
{
    if (m_listenAddress != listenAddress) {
        m_listenAddress = listenAddress;

        if (!m_createdByQml || m_complete) {
            updateServer();
        }

        emit listenAddressChanged();
    }
}

int HttpServer::port() const
{
    return m_port;
}

void HttpServer::setPort(int port)
{
    if (m_port != port) {
        m_port = port;

        if (!m_createdByQml || m_complete) {
            updateServer();
        }

        emit portChanged();
    }
}

ShelfModel *HttpServer::model() const
{
    return m_model;
}

void HttpServer::setModel(ShelfModel *model)
{
    if (m_model != model) {
        m_model = model;

        if (m_model) {
            QObject::connect(m_model, &QAbstractListModel::destroyed,
                this, &HttpServer::updateHandler);
            QObject::connect(m_model, &QAbstractListModel::rowsInserted,
                this, &HttpServer::updateHandler);
            QObject::connect(m_model, &QAbstractListModel::rowsRemoved,
                this, &HttpServer::updateHandler);
            QObject::connect(m_model, &QAbstractListModel::modelReset,
                this, &HttpServer::updateHandler);
        }

        if (!m_createdByQml || m_complete) {
            updateHandler();
        }

        emit modelChanged();
    }
}

void HttpServer::classBegin()
{
    m_createdByQml = true;
}

void HttpServer::componentComplete()
{
    m_complete = true;

    updateHandler();
    updateServer();
}

void HttpServer::updateServer()
{
#ifdef HYELICHT_BUILD_ONBOARD
    if (!m_enabled || m_server->isListening()) {
        m_server->close();
    }

    if (m_enabled) {
        if (!m_server->listen(QHostAddress(m_listenAddress), m_port)) {
            qCCritical(HYELICHT_HTTPSERVER) << i18n("Failed to start HTTP REST API server on: %1",
                QString("%1:%2").arg(m_listenAddress).arg(m_port));
        } else {
            qCInfo(HYELICHT_HTTPSERVER) << i18n("HTTP REST API server now listening on: %1",
                QString("%1:%2").arg(m_listenAddress).arg(m_port));
        }
    }
#endif
}

void HttpServer::updateHandler()
{
#ifdef HYELICHT_BUILD_ONBOARD
    QHttpEngine::QObjectHandler *oldHandler {m_handler};
    m_handler = new QHttpEngine::QObjectHandler {this};
    m_server->setHandler(m_handler);
    delete oldHandler;

    if (m_model) {
        m_handler->registerMethod("v1/shelf", this,
            [=](QHttpEngine::Socket *socket) {
                socket->setHeader("Allow", "GET", true);

                if (socket->method() == QHttpEngine::Socket::GET) {
                    QJsonObject response {
                        {QStringLiteral("enabled"), m_model->enabled()},
                        {QStringLiteral("brightness"), m_model->brightness()},
                        {QStringLiteral("averageColor"), m_model->averageColor().name(QColor::HexRgb)},
                        {QStringLiteral("rows"), m_model->rows()},
                        {QStringLiteral("columns"), m_model->columns()},
                        {QStringLiteral("squares"), m_model->rowCount()},
                        {QStringLiteral("animating"), m_model->animating()}
                    };

                    socket->writeJson(QJsonDocument {response});
                    return;
                }

                socket->writeError(QHttpEngine::Socket::MethodNotAllowed);
            },
        true /* readAll */);

        m_handler->registerMethod("v1/shelf/enabled", this, propHandler("enabled"), true /* readAll */);
        m_handler->registerMethod("v1/shelf/brightness", this, propHandler("brightness"), true /* readAll */);
        m_handler->registerMethod("v1/shelf/averageColor", this, propHandler("averageColor"), true /* readAll */);
        m_handler->registerMethod("v1/shelf/animating", this, propHandler("animating"), true /* readAll */);

        m_handler->registerMethod("v1/squares", this,
            [=](QHttpEngine::Socket *socket) {
                socket->setHeader("Allow", "GET, PUT", true);

                auto printSquares = [=]() {
                    QJsonArray items;

                    for (int i {0}; i < m_model->rowCount(); ++i) {
                        QJsonObject square {rowToJson(m_model->index(i, 0))};
                        square.insert(QStringLiteral("id"), i);
                        square.insert(QStringLiteral("self"), QStringLiteral("v1/squares/%1").arg(i));
                        items.append(square);
                    }

                    QJsonObject data {{QStringLiteral("items"), items}};
                    QJsonObject response {{QStringLiteral("data"), data}};

                    socket->writeJson(QJsonDocument {response});
                };

                if (socket->method() == QHttpEngine::Socket::GET) {
                    printSquares();
                    return;
                } else if (socket->method() == QHttpEngine::Socket::PUT) {
                    QJsonDocument document;

                    if (socket->readJson(document) && document.object().size() >= 1) {
                        // For now only expose this role, and not e.g. the per-square brightness.
                        const QHash<int, QVariant> &newData {jsonToModelRole(document,
                            QStringLiteral("averageColor"))};

                        if (!newData.isEmpty()) {
                            for (const int row : newData.keys()) {
                                m_model->setData(m_model->index(row, 0), newData[row]);
                            }

                            printSquares();
                            return;
                        }
                    }
                } else {
                    socket->writeError(QHttpEngine::Socket::MethodNotAllowed);
                }

                socket->writeError(QHttpEngine::Socket::BadRequest);
            },
        true /* readAll */);

        for (int i {0}; i < m_model->rowCount(); ++i) {
            m_handler->registerMethod(QString("v1/squares/%1").arg(i), this,
                [=](QHttpEngine::Socket *socket) {
                    socket->setHeader("Allow", "GET, PUT", true);

                    if (socket->method() == QHttpEngine::Socket::GET) {
                        socket->writeJson(QJsonDocument {rowToJson(m_model->index(i, 0))});
                        return;
                    } else if (socket->method() == QHttpEngine::Socket::PUT) {
                        QJsonDocument document;

                        if (socket->readJson(document) && document.object().size() >= 1) {
                            // Don't expose per-square brightness to the frontends for the moment.
                            const QJsonValue &value {document.object().value(QStringLiteral("averageColor"))};

                            if (value != QJsonValue::Undefined) {
                                const QModelIndex &modelIndex {m_model->index(i, 0)};
                                m_model->setData(modelIndex, value.toVariant());
                                socket->writeJson(QJsonDocument {rowToJson(modelIndex)});
                                return;
                            }
                        }
                    } else {
                        socket->writeError(QHttpEngine::Socket::MethodNotAllowed);
                    }

                    socket->writeError(QHttpEngine::Socket::BadRequest);
                },
            true /* readAll */);

            // Don't expose per-square brightness to the frontends for the moment.
            // m_handler->registerMethod(QString("v1/squares/%1/brightness").arg(i), this,
            //     modelRowHandler("brightness", i, ShelfModel::Brightness), true /* readAll */);
            m_handler->registerMethod(QString("v1/squares/%1/averageColor").arg(i), this,
                modelRowHandler("averageColor", i, Qt::EditRole), true /* readAll */);
        }
    }
#endif
}

#ifdef HYELICHT_BUILD_ONBOARD
std::function<void(QHttpEngine::Socket *)> HttpServer::propHandler(const char *prop)
{
    return [=](QHttpEngine::Socket *socket) {
        socket->setHeader("Allow", "GET, PUT", true);

        if (socket->method() == QHttpEngine::Socket::GET) {
            propToJSon(socket, m_model, prop);
            socket->close();
            return;
        } else if (socket->method() == QHttpEngine::Socket::PUT) {
            jsonToProp(socket, m_model, prop);
            socket->close();
            return;
        } else {
            socket->writeError(QHttpEngine::Socket::MethodNotAllowed);
        }

        socket->writeError(QHttpEngine::Socket::BadRequest);
    };
}

std::function<void(QHttpEngine::Socket *)> HttpServer::modelRowHandler(const char *prop,
    const int index, const int role)
{
    return [=](QHttpEngine::Socket *socket) {
        socket->setHeader("Allow", "GET, PUT", true);

        const QModelIndex &modelIndex = m_model->index(index, 0);

        if (socket->method() == QHttpEngine::Socket::GET) {
            QJsonObject response {{prop, QJsonValue::fromVariant(modelIndex.data(role))}};
            socket->writeJson(QJsonDocument {response});
            return;
        } else if (socket->method() == QHttpEngine::Socket::PUT && role == Qt::EditRole) {
            QJsonDocument document;

            if (socket->readJson(document) && document.object().size() >= 1) {
                if (m_model->setData(modelIndex, document.object().begin().value().toVariant())) {
                    QJsonObject response {{prop, QJsonValue::fromVariant(modelIndex.data(role))}};
                    socket->writeJson(QJsonDocument {response});
                    return;
                }
            }
        } else {
            socket->writeError(QHttpEngine::Socket::MethodNotAllowed);
        }

        socket->writeError(QHttpEngine::Socket::MethodNotAllowed);
    };
}

void HttpServer::propToJSon(QHttpEngine::Socket *socket, const QObject *obj, const char *name)
{
    if (!obj) {
        socket->writeError(QHttpEngine::Socket::BadRequest);
    }

    const int propIndex {obj->metaObject()->indexOfProperty(name)};

    if (propIndex == -1) {
        socket->writeError(QHttpEngine::Socket::BadRequest);
    }

    const QMetaProperty &metaProp {obj->metaObject()->property(propIndex)};

    if (!metaProp.isReadable()) {
        socket->writeError(QHttpEngine::Socket::BadRequest);
    }

    QJsonObject response {{name, QJsonValue::fromVariant(obj->property(name))}};
    socket->writeJson(QJsonDocument(response));
}

void HttpServer::jsonToProp(QHttpEngine::Socket *socket, QObject *obj, const char *name)
{
    if (!obj) {
        socket->writeError(QHttpEngine::Socket::BadRequest);
    }

    const int propIndex {obj->metaObject()->indexOfProperty(name)};

    if (propIndex == -1) {
        socket->writeError(QHttpEngine::Socket::BadRequest);
    }

    const QMetaProperty &metaProp {obj->metaObject()->property(propIndex)};

    if (!metaProp.isWritable()) {
        socket->writeError(QHttpEngine::Socket::BadRequest);
    }

    QJsonDocument document;

    if (socket->readJson(document)) {
        QVariantMap body {document.object().toVariantMap()};

        if (body.contains(name)) {
            const QVariant &value {body.value(name)};

            if (value.canConvert(metaProp.type())) {
                obj->setProperty(name, value);
            }
        }
    } else {
        return;
    }

    if (metaProp.isReadable()) {
        QJsonObject response {{name, QJsonValue::fromVariant(obj->property(name))}};
        socket->writeJson(QJsonDocument {response});
    }
}
#endif

QJsonObject HttpServer::rowToJson(const QModelIndex &modelIndex)
{
    const QHash<int, QByteArray> &roles = m_model->roleNames();
    
    QJsonObject rowObj;

    for (const int role : roles.keys()) {
        // For now only expose this role, and not e.g. the per-square brightness.
        // if (role >= Qt::UserRole) {
        if (role == ShelfModel::AverageColor) {
            rowObj.insert(roles[role], QJsonValue::fromVariant(modelIndex.data(role)));
        }
    }

    return rowObj;
}

QHash<int, QVariant> HttpServer::jsonToModelRole(const QJsonDocument &document, const QString &roleName)
{
    QHash<int, QVariant> newData;

    const QJsonValue &data {document.object().value(QStringLiteral("data"))};

    if (data.isObject()) {
        const QJsonValue &items {data.toObject().value(QStringLiteral("items"))};

        if (items.isArray()) {
            for (const QJsonValue &item : items.toArray()) {
                if (!item.isObject()) {
                    return newData;
                } 

                const QJsonValue &id = {item.toObject().value(QStringLiteral("id"))};
                const QJsonValue &value {item.toObject().value(roleName)};

                if (id == QJsonValue::Undefined || value == QJsonValue::Undefined) {
                    return newData;
                }

                const int row {id.toInt(-1)};

                if (row >= 0 && row < m_model->rowCount()) {
                    newData.insert(id.toInt(), value.toVariant()); 
                }
            }
        }
    }

    return newData;
}
