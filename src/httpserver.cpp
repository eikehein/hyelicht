/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

#include "httpserver.h"
#include "debug_httpserver.h"

#include <KLocalizedString>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaProperty>
#include <qnamespace.h>

#ifdef HYELICHT_BUILD_ONBOARD
#include <QTcpServer>
#include <QHttpServer>
#endif

HttpServer::HttpServer(QObject *parent)
    : QObject{parent}
    , m_enabled {false}
    , m_listenAddress {QStringLiteral("127.0.0.1")}
    , m_port {8082}
#ifdef HYELICHT_BUILD_ONBOARD
    , m_tcpServer {new QTcpServer {this}}
    , m_httpServer {new QHttpServer {this}}
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

        Q_EMIT enabledChanged();
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

        Q_EMIT listenAddressChanged();
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

        Q_EMIT portChanged();
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

        Q_EMIT modelChanged();
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
    if (!m_enabled || m_tcpServer->isListening()) {
        m_tcpServer->close();
    }

    if (m_enabled) {
        if (!m_tcpServer->listen(QHostAddress(m_listenAddress), m_port)) {
            qCCritical(HYELICHT_HTTPSERVER) << i18n("Failed to start HTTP REST API server on: %1",
                QLatin1StringView("%1:%2").arg(m_listenAddress).arg(m_port));
        } else {
            if (!m_httpServer->servers().contains(m_tcpServer)) {
                m_httpServer->bind(m_tcpServer);
            }

            qCInfo(HYELICHT_HTTPSERVER) << i18n("HTTP REST API server now listening on: %1",
                QLatin1StringView("%1:%2").arg(m_listenAddress).arg(m_port));
        }
    }
#endif
}

void HttpServer::updateHandler()
{
#ifdef HYELICHT_BUILD_ONBOARD
    if (!m_model) {
        return;
    }

    m_httpServer->route(QStringLiteral("/v1/shelf"), [&](const QHttpServerRequest &request,
        QHttpServerResponder &responder) {
        QHttpHeaders headers;
        headers.replaceOrAppend(QHttpHeaders::WellKnownHeader::Allow, QStringLiteral("GET"));

        if (request.method() == QHttpServerRequest::Method::Get) {
            QJsonObject response {
                {QStringLiteral("enabled"), m_model->enabled()},
                {QStringLiteral("brightness"), m_model->brightness()},
                {QStringLiteral("averageColor"), m_model->averageColor().name(QColor::HexRgb)},
                {QStringLiteral("rows"), m_model->rows()},
                {QStringLiteral("columns"), m_model->columns()},
                {QStringLiteral("squares"), m_model->rowCount()},
                {QStringLiteral("animating"), m_model->animating()}
            };

            responder.write(QJsonDocument {response}, headers);
        } else {
            responder.write(headers, QHttpServerResponder::StatusCode::MethodNotAllowed);
        }
    });

    m_httpServer->route(QStringLiteral("/v1/shelf/enabled"), propHandler("enabled"));
    m_httpServer->route(QStringLiteral("/v1/shelf/brightness"), propHandler("brightness"));
    m_httpServer->route(QStringLiteral("/v1/shelf/averageColor"), propHandler("averageColor"));
    m_httpServer->route(QStringLiteral("/v1/shelf/animating"), propHandler("animating"));

    m_httpServer->route(QStringLiteral("/v1/squares"), [=](const QHttpServerRequest &request,
        QHttpServerResponder &responder) {
        QHttpHeaders headers;
        headers.replaceOrAppend(QHttpHeaders::WellKnownHeader::Allow, QStringLiteral("GET"));

        auto printSquares = [&]() {
            QJsonArray items;

            for (int i {0}; i < m_model->rowCount(); ++i) {
                QJsonObject square {rowToJson(m_model->index(i, 0))};
                square.insert(QStringLiteral("id"), i);
                square.insert(QStringLiteral("self"), QStringLiteral("/v1/squares/%1").arg(QString::number(i)));
                items.append(square);
            }

            QJsonObject data {{QStringLiteral("items"), items}};
            QJsonObject response {{QStringLiteral("data"), data}};

            responder.write(QJsonDocument {response}, headers);
        };

        if (request.method() == QHttpServerRequest::Method::Get) {
            printSquares();
            return;
        } else if (request.method() == QHttpServerRequest::Method::Put) {
            QJsonParseError jsonStatus;
            const QJsonDocument &document = QJsonDocument::fromJson(request.body(), &jsonStatus);

            if (jsonStatus.error == QJsonParseError::NoError && document.object().size() >= 1) {
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
            responder.write(headers, QHttpServerResponder::StatusCode::MethodNotAllowed);
        }

        responder.write(headers, QHttpServerResponder::StatusCode::BadRequest);
    });

    for (int i {0}; i < m_model->rowCount(); ++i) {
        m_httpServer->route(QLatin1StringView("/v1/squares/%1").arg(QString::number(i)),
            [&](const QHttpServerRequest &request,
            QHttpServerResponder &responder) {
            QHttpHeaders headers;
            headers.replaceOrAppend(QHttpHeaders::WellKnownHeader::Allow, QStringLiteral("GET"));

            if (request.method() == QHttpServerRequest::Method::Get) {
                responder.write(QJsonDocument {rowToJson(m_model->index(i, 0))}, headers);
                return;
            } else if (request.method() == QHttpServerRequest::Method::Put) {
                QJsonParseError jsonStatus;
                const QJsonDocument &document = QJsonDocument::fromJson(request.body(), &jsonStatus);

                if (jsonStatus.error == QJsonParseError::NoError && document.object().size() >= 1) {
                    // Don't expose per-square brightness to the frontends for the moment.
                    const QJsonValue &value {document.object().value(QStringLiteral("averageColor"))};

                    if (value != QJsonValue::Undefined) {
                        const QModelIndex &modelIndex {m_model->index(i, 0)};
                        m_model->setData(modelIndex, value.toVariant());
                        responder.write(QJsonDocument {rowToJson(modelIndex)}, headers);
                        return;
                    }
                }
            } else {
                responder.write(headers, QHttpServerResponder::StatusCode::MethodNotAllowed);
            }

            responder.write(headers, QHttpServerResponder::StatusCode::BadRequest);
        });

        // Don't expose per-square brightness to the frontends for the moment.
        // m_handler->registerMethod(QString("v1/squares/%1/brightness").arg(i), this,
        //     modelRowHandler("brightness", i, ShelfModel::Brightness), true /* readAll */);
        m_httpServer->route(QLatin1StringView("/v1/squares/%1/averageColor").arg(QString::number(i)),
            modelRowHandler("averageColor", i, Qt::EditRole));
    }
#endif
}
#ifdef HYELICHT_BUILD_ONBOARD
std::function<void(const QHttpServerRequest &, QHttpServerResponder &)> HttpServer::propHandler(const char *prop)
{
    return [=](const QHttpServerRequest &request, QHttpServerResponder &responder) {
        QHttpHeaders headers;
        headers.replaceOrAppend(QHttpHeaders::WellKnownHeader::Allow, QStringLiteral("GET, PUT"));

        if (request.method() == QHttpServerRequest::Method::Get) {
            propToJSon(responder, headers, m_model, prop);
        } else if (request.method() == QHttpServerRequest::Method::Put) {
            jsonToProp(request, responder, headers, m_model, prop);
        } else {
            responder.write(headers, QHttpServerResponder::StatusCode::MethodNotAllowed);
        }

        responder.write(headers, QHttpServerResponder::StatusCode::BadRequest);
    };
}

std::function<void(const QHttpServerRequest &, QHttpServerResponder &)> HttpServer::modelRowHandler(const char *prop,
    const int index, const int role)
{
    return [=](const QHttpServerRequest &request, QHttpServerResponder &responder) {
        QHttpHeaders headers;
        headers.replaceOrAppend(QHttpHeaders::WellKnownHeader::Allow, QStringLiteral("GET, PUT"));

        const QModelIndex &modelIndex = m_model->index(index, 0);

        if (request.method() == QHttpServerRequest::Method::Get) {
            QJsonObject response {{QLatin1StringView(prop), QJsonValue::fromVariant(modelIndex.data(role))}};
            responder.write(QJsonDocument {response}, headers);
            return;
        } else if (request.method() == QHttpServerRequest::Method::Put && role == Qt::EditRole) {
            QJsonParseError jsonStatus;
            const QJsonDocument &document = QJsonDocument::fromJson(request.body(), &jsonStatus);

            if (jsonStatus.error == QJsonParseError::NoError && document.object().size() >= 1) {
                if (m_model->setData(modelIndex, document.object().begin().value().toVariant())) {
                    QJsonObject response {{QLatin1StringView(prop), QJsonValue::fromVariant(modelIndex.data(role))}};
                    responder.write(QJsonDocument {response}, headers);
                    return;
                }
            }
        } else {
            responder.write(headers, QHttpServerResponder::StatusCode::MethodNotAllowed);
        }

        responder.write(headers, QHttpServerResponder::StatusCode::MethodNotAllowed);
    };
}

void HttpServer::propToJSon(QHttpServerResponder &responder, const QHttpHeaders &headers,
    const QObject *obj, const char *name)
{
    if (!obj) {
        responder.write(headers, QHttpServerResponder::StatusCode::BadRequest);
    }

    const int propIndex {obj->metaObject()->indexOfProperty(name)};

    if (propIndex == -1) {
        responder.write(headers, QHttpServerResponder::StatusCode::BadRequest);
    }

    const QMetaProperty &metaProp {obj->metaObject()->property(propIndex)};

    if (!metaProp.isReadable()) {
        responder.write(headers, QHttpServerResponder::StatusCode::BadRequest);
    }

    QJsonObject response {{QLatin1StringView(name), QJsonValue::fromVariant(obj->property(name))}};
    responder.write(QJsonDocument {response}, headers);
}

void HttpServer::jsonToProp(const QHttpServerRequest &request, QHttpServerResponder &responder,
    const QHttpHeaders &headers, QObject *obj, const char *name)
{
    if (!obj) {
        responder.write(headers, QHttpServerResponder::StatusCode::BadRequest);
    }

    const int propIndex {obj->metaObject()->indexOfProperty(name)};

    if (propIndex == -1) {
        responder.write(headers, QHttpServerResponder::StatusCode::BadRequest);
    }

    const QMetaProperty &metaProp {obj->metaObject()->property(propIndex)};

    if (!metaProp.isWritable()) {
        responder.write(headers, QHttpServerResponder::StatusCode::BadRequest);
    }

    QJsonParseError jsonStatus;
    const QJsonDocument &document = QJsonDocument::fromJson(request.body(), &jsonStatus);

    if (jsonStatus.error == QJsonParseError::NoError) {
        QVariantMap body {document.object().toVariantMap()};
        QLatin1StringView latin1Name {name};

        if (body.contains(latin1Name)) {
            const QVariant &value {body.value(latin1Name)};

            if (value.canConvert(metaProp.metaType())) {
                obj->setProperty(name, value);
            }
        }
    } else {
        return;
    }

    if (metaProp.isReadable()) {
        QJsonObject response {{QLatin1StringView(name), QJsonValue::fromVariant(obj->property(name))}};
        responder.write(QJsonDocument {response}, headers);
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
            rowObj.insert(QLatin1StringView(roles[role]), QJsonValue::fromVariant(modelIndex.data(role)));
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
