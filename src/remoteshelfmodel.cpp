/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

#include "remoteshelfmodel.h"
#include "debug_remoting.h"
#include "rep_remoteshelfmodeliface_merged.h"

#include <KLocalizedString>

#include <QAbstractItemModelReplica>
#include <QColor>
#include <QRemoteObjectNode>
#include <QAbstractItemModelReplica>

RemoteShelfModel::RemoteShelfModel(QObject *parent)
    : QIdentityProxyModel(parent)
    , m_enabled {true}
    , m_serverAddress {QStringLiteral("192.168.178.129:8042")}
    , m_remotingServer {nullptr}
    , m_remoteModel {nullptr}
    , m_remoteModelIface {nullptr}
    , m_createdByQml {false}
    , m_complete {false}
{
}

RemoteShelfModel::~RemoteShelfModel()
{
}

bool RemoteShelfModel::connected() const
{
    return m_remoteModelIface && (m_remoteModelIface->state() == QRemoteObjectReplica::Valid);
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

        Q_EMIT serverAddressChanged();
    }
}

bool RemoteShelfModel::enabled() const
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return false;
    }

    return m_remoteModelIface->enabled();
}

void RemoteShelfModel::setEnabled(bool enabled)
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return;
    }

    m_remoteModelIface->setEnabled(enabled);
}

int RemoteShelfModel::rows() const
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return 4;
    }

    return m_remoteModelIface->rows();
}

void RemoteShelfModel::setRows(int rows)
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return;
    }

    m_remoteModelIface->setRows(rows);
}

int RemoteShelfModel::columns() const
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return 5;
    }

    return m_remoteModelIface->columns();
}

void RemoteShelfModel::setColumns(int columns)
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return;
    }

    m_remoteModelIface->setColumns(columns);
}

int RemoteShelfModel::density() const
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return 20;
    }

    return m_remoteModelIface->density();
}

void RemoteShelfModel::setDensity(int density)
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return;
    }

    m_remoteModelIface->setDensity(density);
}

int RemoteShelfModel::wallThickness() const
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return 1;
    }

    return m_remoteModelIface->wallThickness();
}

void RemoteShelfModel::setWallThickness(int thickness)
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return;
    }

    m_remoteModelIface->setWallThickness(thickness);
}

qreal RemoteShelfModel::brightness() const
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return 1.0;
    }

    return m_remoteModelIface->brightness();
}

void RemoteShelfModel::setBrightness(qreal brightness)
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return;
    }

    m_remoteModelIface->setBrightness(brightness);
}

bool RemoteShelfModel::animateBrightnessTransitions() const
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return true;
    }

    return m_remoteModelIface->animateBrightnessTransitions();
}

void RemoteShelfModel::setAnimateBrightnessTransitions(bool animate)
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return;
    }

    m_remoteModelIface->setAnimateBrightnessTransitions(animate);
}

QColor RemoteShelfModel::averageColor() const
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return QColor(QStringLiteral("white"));
    }

    return m_remoteModelIface->property("averageColor").value<QColor>();
}

void RemoteShelfModel::setAverageColor(const QColor &color)
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return;
    }

    m_remoteModelIface->setProperty("averageColor", color);
}

bool RemoteShelfModel::animateAverageColorTransitions() const
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return true;
    }

    return m_remoteModelIface->animateAverageColorTransitions();
}

void RemoteShelfModel::setAnimateAverageColorTransitions(bool animate)
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return;
    }

    m_remoteModelIface->setAnimateAverageColorTransitions(animate);
}

bool RemoteShelfModel::animating() const
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return false;
    }

    return m_remoteModelIface->animating();
}

void RemoteShelfModel::setAnimating(bool animating)
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return;
    }

    m_remoteModelIface->setAnimating(animating);
}

int RemoteShelfModel::transitionDuration() const
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return 400;
    }

    return m_remoteModelIface->transitionDuration();
}

void RemoteShelfModel::setTransitionDuration(int duration)
{
    if (!m_remoteModelIface || !m_remoteModelIface->isInitialized()) {
        return;
    }

    m_remoteModelIface->setTransitionDuration(duration);
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

        delete m_remoteModelIface;
        m_remoteModelIface = nullptr;

        delete m_remotingServer;
        m_remotingServer = nullptr;

        Q_EMIT connectedChanged();

        Q_EMIT enabledChanged();

        Q_EMIT rowsChanged();
        Q_EMIT columnsChanged();
        Q_EMIT densityChanged();
        Q_EMIT wallThicknessChanged();

        Q_EMIT shelfRowsChanged();
        Q_EMIT shelfColumnsChanged();

        Q_EMIT brightnessChanged();
        Q_EMIT animateBrightnessTransitionsChanged();

        Q_EMIT averageColorChanged();
        Q_EMIT animateAverageColorTransitionsChanged();

        Q_EMIT transitionDurationChanged();

        Q_EMIT animatingChanged();
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

        m_remoteModelIface =  m_remotingServer->acquire<RemoteShelfModelIfaceReplica>();

        if (!m_remoteModelIface) {
            qCCritical(HYELICHT_REMOTING) << i18n("Failed to acquire shelf API from the remoting API server: %1",
                m_remotingServer->lastError());
            return;
        }

        Q_EMIT connectedChanged();

        QObject::connect(m_remoteModelIface, &QRemoteObjectReplica::stateChanged,
            this, &RemoteShelfModel::connectedChanged);

        qCInfo(HYELICHT_REMOTING) << i18n("Connected to remoting API server at: %1",
            m_serverAddress.toString());

        QObject::connect(m_remoteModelIface, &QRemoteObjectReplica::initialized, this,
            [=]() {
                QObject::connect(m_remoteModelIface, SIGNAL(enabledChanged(bool)),
                    this, SIGNAL(enabledChanged()));

                QObject::connect(m_remoteModelIface, SIGNAL(rowsChanged(int)),
                    this, SIGNAL(rowsChanged()));
                QObject::connect(m_remoteModelIface, SIGNAL(columnsChanged(int)),
                    this, SIGNAL(columnsChanged()));
                QObject::connect(m_remoteModelIface, SIGNAL(densityChanged(int)),
                    this, SIGNAL(densityChanged()));
                QObject::connect(m_remoteModelIface, SIGNAL(wallThicknessChanged(int)),
                    this, SIGNAL(wallThicknessChanged()));

                QObject::connect(m_remoteModelIface, SIGNAL(brightnessChanged(qreal)),
                    this, SIGNAL(brightnessChanged()));
                QObject::connect(m_remoteModelIface, SIGNAL(animateBrightnessTransitionsChanged(bool)),
                    this, SIGNAL(animateBrightnessTransitionsChanged()));

                QObject::connect(m_remoteModelIface, SIGNAL(averageColorChanged(QColor)),
                    this, SIGNAL(averageColorChanged()));
                QObject::connect(m_remoteModelIface, SIGNAL(animateAverageColorTransitionsChanged(bool)),
                    this, SIGNAL(animateAverageColorTransitionsChanged()));

                QObject::connect(m_remoteModelIface, SIGNAL(transitionDurationChanged(int)),
                    this, SIGNAL(transitionDurationChanged()));

                QObject::connect(m_remoteModelIface, SIGNAL(animatingChanged(bool)),
                    this, SIGNAL(animatingChanged()));
            }
        );
    }
}
