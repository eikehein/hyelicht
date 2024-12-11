/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

#include "displaycontroller.h"
#include "debug_displaycontroller.h"

#include <KLocalizedString>

#include <QDataStream>

#include <cmath>

DisplayController::DisplayController(QObject *parent)
    : QObject {parent}
    , m_enabled {false}
    , m_on {true}
    , m_brightness {1.0}
    , m_pendingBrightness {m_brightness}
    , m_fadeDuration {400}
    , m_createdByQML {false}
    , m_complete {false}
{
#ifdef HYELICHT_BUILD_ONBOARD
    m_serialPort.setPortName(QStringLiteral("/dev/ttyUSB0"));
    m_serialPort.setBaudRate(QSerialPort::Baud115200);

    /* What follows are the QSerialPort defaults, which at the time of
       writing are what we want:

    m_serialPort.setDataBits(QSerialPort::Data8);
    m_serialPort.setParity(QSerialPort::NoParity);
    m_serialPort.setStopBits(QSerialPort::OneStop);
    m_serialPort.setFlowControl(QSerialPort::NoFlowControl);
    */
#endif

    m_fadeAnimation.setEasingCurve(QEasingCurve::InCubic);

    QObject::connect(&m_fadeAnimation, &QVariantAnimation::finished, this,
        [=]() {
            if (m_brightness != m_pendingBrightness) {
                m_brightness = m_pendingBrightness;
                Q_EMIT brightnessChanged();
            }

            if (m_fadeAnimation.endValue() == 0) {
                m_on = false;
                Q_EMIT onChanged();

                m_idleTimeoutTimer.stop();
            }
        }
    );

    QObject::connect(&m_fadeAnimation, &QVariantAnimation::valueChanged, this,
        [=](const QVariant &value) {
            // Ignore `valueChanged` emissions stemming from calls to
            // `setStartValue`/`setEndValue`.
            if (m_fadeAnimation.state() != QAbstractAnimation::Running) {
                return;
            }

#ifdef HYELICHT_BUILD_ONBOARD
            if (m_enabled) {
                writeBrightness(value.toReal());
            }
#else
    Q_UNUSED(value)
#endif
        }
    );

    m_idleTimeoutTimer.setInterval(1000 * 20);
    m_idleTimeoutTimer.setSingleShot(true);

    QObject::connect(&m_idleTimeoutTimer, &QTimer::timeout, this,
        [=]() {
            setOn(false);
        }
    );
}

DisplayController::~DisplayController()
{
    disconnectPwmGenerator();
}

bool DisplayController::enabled() const
{
    return m_enabled;
}

void DisplayController::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;

        if (!m_createdByQML || m_complete) {
            if (m_enabled) {
                connectPwmGenerator();
            } else {
                disconnectPwmGenerator();
            }
        }

        Q_EMIT enabledChanged();
    }
}

QString DisplayController::serialPortName() const
{
#ifdef HYELICHT_BUILD_ONBOARD
    return m_serialPort.portName();
#else
    return QString();
#endif
}

void DisplayController::setSerialPortName(const QString &name)
{
#ifdef HYELICHT_BUILD_ONBOARD
    if (m_serialPort.portName() != name) {
        m_serialPort.setPortName(name);

        if (!m_createdByQML || m_complete) {
            if (m_enabled) {
                connectPwmGenerator();
            }
        }

        Q_EMIT serialPortNameChanged();
    }
#else
    Q_UNUSED(name)
#endif
}

qint32 DisplayController::baudRate() const
{
#ifdef HYELICHT_BUILD_ONBOARD
    return m_serialPort.baudRate();
#else
    return 0;
#endif
}

void DisplayController::setBaudRate(qint32 rate)
{
#ifdef HYELICHT_BUILD_ONBOARD
    if (m_serialPort.baudRate() != rate) {
        m_serialPort.setBaudRate(rate);

        if (!m_createdByQML || m_complete) {
            if (m_enabled) {
                connectPwmGenerator();
            }
        }

        Q_EMIT baudRateChanged();
    }
#else
    Q_UNUSED(rate)
#endif
}

bool DisplayController::on() const
{
    return m_on;
}

void DisplayController::setOn(bool on)
{
    if (m_on != on) {
        if ((!m_createdByQML || m_complete)) {
            if (m_idleTimeoutTimer.interval() > 0) {
                m_idleTimeoutTimer.start();
            }

            if (m_fadeAnimation.duration() > 0 && m_enabled) {
                // If we're fading in, update the prop. Otherwise,
                // the fade will update it whenever we're fading to
                // 0.
                if (on) {
                    m_on = on;
                    Q_EMIT onChanged();
                }

                fade(on ? 0.0 : m_brightness, on ? m_brightness : 0.0);

                return;
            }
        }

        m_on = on;
        Q_EMIT onChanged();

        if (m_on && m_idleTimeoutTimer.interval() > 0) {
            m_idleTimeoutTimer.start();
        } else {
            m_idleTimeoutTimer.stop();
        }
    }
}

qreal DisplayController::brightness() const
{
    return m_brightness;
}

void DisplayController::setBrightness(qreal brightness)
{
    if (m_brightness != brightness) {
        if ((!m_createdByQML || m_complete)
            && m_fadeAnimation.duration() > 0 && m_enabled && m_on) {
            m_pendingBrightness = brightness;
            fade(m_brightness, brightness);
            return;
        }

        m_brightness = brightness;
        Q_EMIT brightnessChanged();
    }
}

int DisplayController::idleTimeout() const
{
    return m_idleTimeoutTimer.interval() / 1000;
}

void DisplayController::setIdleTimeout(int timeout)
{
    if (m_idleTimeoutTimer.interval() / 1000 != timeout) {
        m_idleTimeoutTimer.setInterval(timeout * 1000);

        if (!m_createdByQML || m_complete) {
            if (m_idleTimeoutTimer.interval() == 0) {
                m_idleTimeoutTimer.stop();
            } else if (m_on) { // TODO: Could behave in a smarter way depending on
                               // whether the new value is < or > the old one.
                m_idleTimeoutTimer.start();
            }
        }

        Q_EMIT idleTimeoutChanged();
    }
}

void DisplayController::resetIdleTimeout()
{
    if (!m_on || m_idleTimeoutTimer.interval() == 0) {
        return;
    }

    m_idleTimeoutTimer.start();
}

int DisplayController::fadeDuration() const
{
    return m_fadeDuration;
}

void DisplayController::setFadeDuration(int fadeDuration)
{
    if (m_fadeDuration != fadeDuration) {
        m_fadeDuration = fadeDuration;

        Q_EMIT fadeDurationChanged();
    }
}

QEasingCurve DisplayController::fadeEasing() const
{
    return m_fadeAnimation.easingCurve();
}

void DisplayController::setFadeEasing(const QEasingCurve &fadeEasing)
{
    if (m_fadeAnimation.easingCurve() != fadeEasing) {
        m_fadeAnimation.setEasingCurve(fadeEasing);
        Q_EMIT fadeEasingChanged();
    }
}

void DisplayController::classBegin()
{
    m_createdByQML = true;
}

void DisplayController::componentComplete()
{
    m_complete = true;

    if (m_enabled) {
        connectPwmGenerator();
    }

    if (m_on && m_idleTimeoutTimer.interval() > 0) {
        m_idleTimeoutTimer.start();
    }
}

void DisplayController::connectPwmGenerator()
{
    if (!m_enabled) {
        return;
    }

#ifdef HYELICHT_BUILD_ONBOARD
    if (m_serialPort.isOpen()) {
        m_serialPort.close();
    }

    if (!m_serialPort.open(QIODevice::WriteOnly) || !m_serialPort.isWritable()) {
        qCCritical(HYELICHT_DISPLAYCONTROLLER) << i18n("Error opening writable serial connection to PWM generator: %1",
            m_serialPort.error());
    } else {
        qCInfo(HYELICHT_DISPLAYCONTROLLER) << i18n("Connected to PWM generator via serial port '%1'.",
            m_serialPort.portName());
        writeBrightness(m_brightness);
    }
#endif
}

void DisplayController::disconnectPwmGenerator()
{
#ifdef HYELICHT_BUILD_ONBOARD
    if (m_serialPort.isOpen()) {
        qCInfo(HYELICHT_DISPLAYCONTROLLER) << i18n("Closing connection to PWM generator at serial port '%1'.",
            m_serialPort.portName());
        m_serialPort.close();
    }
#endif
}

void DisplayController::fade(qreal from, qreal to)
{
    m_fadeAnimation.stop();
    const qreal delta {std::abs(m_brightness - to) ? std::abs(m_brightness - to) : 1.0};
    m_fadeAnimation.setDuration(m_fadeDuration * delta);
    m_fadeAnimation.setStartValue(from);
    m_fadeAnimation.setEndValue(to);
    m_fadeAnimation.start();
}

void DisplayController::writeBrightness(qreal brightness)
{
#ifdef HYELICHT_BUILD_ONBOARD
    QByteArray data;

    QDataStream stream {&data, QIODevice::WriteOnly};
    stream.setVersion(QDataStream::Qt_5_15); // Ensure against future behavior changes.
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << static_cast<quint8>(255 - std::rint(255 * brightness));

    m_serialPort.write(data.data(), 1);

    // Block max 10ms on writing to the serial port. We want to
    // keep the output and input buffers shallow.
    m_serialPort.waitForBytesWritten(10);
#else
    Q_UNUSED(brightness)
#endif
}
