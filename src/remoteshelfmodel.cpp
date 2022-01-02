/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>
 */

#include "remoteshelfmodel.h"
#include "debug_remoting.h"

#include <KLocalizedString>

#include <QAbstractItemModelReplica>
#include <QColor>
#include <QRemoteObjectNode>
#include <QAbstractItemModelReplica>

RemoteShelfModel::RemoteShelfModel(QObject *parent)
    : QIdentityProxyModel(parent)
    , m_enabled {true}
    , m_serverAddress {QStringLiteral("tcp://192.168.178.129:8042")}
    , m_remotingServer {nullptr}
    , m_remoteModel {nullptr}
    , m_remoteModelApi {nullptr}
    , m_createdByQml {false}
    , m_complete {false}
{
}

RemoteShelfModel::~RemoteShelfModel()
{
}

bool RemoteShelfModel::connected() const
{
    return m_remoteModelApi && (m_remoteModelApi->state() == QRemoteObjectReplica::Valid);
}

QUrl RemoteShelfModel::serverAddress() const
{
    return m_serverAddress;
}

void RemoteShelfModel::setServerAddress(const QUrl &url)
{
    if (m_serverAddress != url) {
        m_serverAddress = url;

        if (!m_createdByQml || m_complete) {
            updateSource();
        }

        emit serverAddressChanged();
    }
}

bool RemoteShelfModel::enabled() const
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return false;
    }
    
    return m_remoteModelApi->property("enabled").toBool();
}

void RemoteShelfModel::setEnabled(bool enabled)
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return;
    }

    m_remoteModelApi->setProperty("enabled", enabled);
}

int RemoteShelfModel::rows() const
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return 4;
    }
    
    return m_remoteModelApi->property("rows").toInt();
}

void RemoteShelfModel::setRows(int rows)
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return;
    }

    m_remoteModelApi->setProperty("rows", rows);
}

int RemoteShelfModel::columns() const
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return 5;
    }
    
    return m_remoteModelApi->property("columns").toInt();
}

void RemoteShelfModel::setColumns(int columns)
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return;
    }

    m_remoteModelApi->setProperty("columns", columns);
}

int RemoteShelfModel::density() const
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return 20;
    }
    
    return m_remoteModelApi->property("density").toInt();
}

void RemoteShelfModel::setDensity(int density)
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return;
    }

    m_remoteModelApi->setProperty("density", density);
}

int RemoteShelfModel::wallThickness() const
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return 1;
    }
    
    return m_remoteModelApi->property("wallThickness").toInt();
}

void RemoteShelfModel::setWallThickness(int thickness)
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return;
    }

    m_remoteModelApi->setProperty("wallThickness", thickness);
}

qreal RemoteShelfModel::brightness() const
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return 1.0;
    }
    
    return m_remoteModelApi->property("brightness").toReal();
}

void RemoteShelfModel::setBrightness(qreal brightness)
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return;
    }

    m_remoteModelApi->setProperty("brightness", brightness);
}

bool RemoteShelfModel::animateBrightnessTransitions() const
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return true;
    }
    
    return m_remoteModelApi->property("animatedBrightnessTransitions").toBool();
}

void RemoteShelfModel::setAnimateBrightnessTransitions(bool animate)
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return;
    }

    m_remoteModelApi->setProperty("animatedBrightnessTransitions", animate); 
}

QColor RemoteShelfModel::averageColor() const
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return QColor(QStringLiteral("white"));
    }
    
    return m_remoteModelApi->property("averageColor").value<QColor>();
}

void RemoteShelfModel::setAverageColor(const QColor &color)
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return;
    }

    m_remoteModelApi->setProperty("averageColor", color);
}

bool RemoteShelfModel::animateAverageColorTransitions() const
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return true;
    }
    
    return m_remoteModelApi->property("animateAverageColorTransitions").toBool();
}

void RemoteShelfModel::setAnimateAverageColorTransitions(bool animate)
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return;
    }

    m_remoteModelApi->setProperty("animateAverageColorTransitions", animate);
}

bool RemoteShelfModel::animating() const
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return false;
    }
    
    return m_remoteModelApi->property("animating").toBool();
}

void RemoteShelfModel::setAnimating(bool animating)
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return;
    }

    m_remoteModelApi->setProperty("animating", animating);
}

int RemoteShelfModel::transitionDuration() const
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return 400;
    }
    
    return m_remoteModelApi->property("transitionDuration").toInt();
}

void RemoteShelfModel::setTransitionDuration(int duration)
{
    if (!m_remoteModelApi || !m_remoteModelApi->isInitialized()) {
        return;
    }

    m_remoteModelApi->setProperty("transitionDuration", duration);  
}

void RemoteShelfModel::classBegin()
{
    m_createdByQml = true;
}

void RemoteShelfModel::componentComplete()
{
    m_complete = true;

    updateSource();
}

void RemoteShelfModel::updateSource()
{
    if ((m_serverAddress.isEmpty() || !m_serverAddress.isValid()) && m_remotingServer) {
        setSourceModel(nullptr);

        delete m_remoteModel;
        m_remoteModel = nullptr;

        delete m_remoteModelApi;
        m_remoteModelApi = nullptr;

        delete m_remotingServer;
        m_remotingServer = nullptr;

        emit connectedChanged();

        emit enabledChanged();

        emit rowsChanged();
        emit columnsChanged();
        emit densityChanged();
        emit wallThicknessChanged();

        emit shelfRowsChanged();
        emit shelfColumnsChanged();

        emit brightnessChanged();
        emit animateBrightnessTransitionsChanged();

        emit averageColorChanged();
        emit animateAverageColorTransitionsChanged();

        emit transitionDurationChanged();

        emit animatingChanged();
    }

    if (!m_serverAddress.isValid()) {
        qCCritical(HYELICHT_REMOTING) << i18n("Failed to connect to remoting API server due to invalid address: %1",
            m_serverAddress.errorString());

        return;
    }

    if (!m_remotingServer) {
        m_remotingServer = new QRemoteObjectNode {this};
    }

    if (!m_remotingServer->connectToNode(m_serverAddress)) {
        qCCritical(HYELICHT_REMOTING) << i18n("Unable to connect to remoting API server: %1",
            m_remotingServer->lastError());
    } else {
        m_remoteModel = m_remotingServer->acquireModel(QStringLiteral("shelfModel"),
            QtRemoteObjects::PrefetchData);

        if (!m_remoteModel) {
            qCCritical(HYELICHT_REMOTING) << i18n("Failed to acquire shelf model from the remoting API server: %1",
                m_remotingServer->lastError());
            return;
        }

        setSourceModel(m_remoteModel);

        m_remoteModelApi =  m_remotingServer->acquireDynamic(QStringLiteral("shelfModelApi"));

        if (!m_remoteModelApi) {
            qCCritical(HYELICHT_REMOTING) << i18n("Failed to acquire shelf API from the remoting API server: %1",
                m_remotingServer->lastError());
            return;
        }

        emit connectedChanged();

        QObject::connect(m_remoteModelApi, &QRemoteObjectReplica::stateChanged,
            this, &RemoteShelfModel::connectedChanged);

        qCInfo(HYELICHT_REMOTING) << i18n("Connected to remoting API server at: %1",
            m_serverAddress.toString());

        QObject::connect(m_remoteModelApi, &QRemoteObjectReplica::initialized, this,
            [=]() {
                QObject::connect(m_remoteModelApi, SIGNAL(enabledChanged()),
                    this, SIGNAL(enabledChanged()));

                QObject::connect(m_remoteModelApi, SIGNAL(rowsChanged()),
                    this, SIGNAL(rowsChanged()));
                QObject::connect(m_remoteModelApi, SIGNAL(columnsChanged()),
                    this, SIGNAL(columnsChanged()));
                QObject::connect(m_remoteModelApi, SIGNAL(densityChanged()),
                    this, SIGNAL(densityChanged()));
                QObject::connect(m_remoteModelApi, SIGNAL(wallThicknessChanged()),
                    this, SIGNAL(wallThicknessChanged()));

                QObject::connect(m_remoteModelApi, SIGNAL(brightnessChanged()),
                    this, SIGNAL(brightnessChanged()));
                QObject::connect(m_remoteModelApi, SIGNAL(animateBrightnessTransitionsChanged()),
                    this, SIGNAL(animateBrightnessTransitionsChanged()));

                QObject::connect(m_remoteModelApi, SIGNAL(averageColorChanged()),
                    this, SIGNAL(averageColorChanged()));
                QObject::connect(m_remoteModelApi, SIGNAL(animateAverageColorTransitionsChanged()),
                    this, SIGNAL(animateAverageColorTransitionsChanged()));

                QObject::connect(m_remoteModelApi, SIGNAL(transitionDurationChanged()),
                    this, SIGNAL(transitionDurationChanged()));

                QObject::connect(m_remoteModelApi, SIGNAL(animatingChanged()),
                    this, SIGNAL(animatingChanged()));
            }
        );
    }
}
