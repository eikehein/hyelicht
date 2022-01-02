/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>
 */

#pragma once

#include <QEasingCurve>
#include <QObject>
#include <QQmlParserStatus>
#include <QVariantAnimation>

#ifdef HYELICHT_BUILD_ONBOARD
#include <QSerialPort>
#endif 

#include <QTimer>

class QSerialPort;

//! Provides PWM-based display backlight control with the help of an attached MCU
/*!
 * \ingroup Backend
 *
 * Performs serial port communication to a MCU (microcontroller unit) programmed to
 * drive a HDMI display's backlight brightness by way of a PWM signal. The intended
 * effect is to smoothly fade in and out the display for interactions with the
 * project's onboard GUI (see \ref fadeDuration).
 *
 * An additional feature is support for automatically turning the display off after
 * a determined idle period (see \ref idleTimeout)
 *
 * The attached MCU is assumed to accept writes of integer values between 0 and 255,
 * representing intended display backlight brightness, on serial.
 *
 * In the Hyelicht project, the attached MCU is an Arduino Nano 3.0 (AVR ATmega328).
 * The Arduino sketch used to program it is provided in the source file
 * \c support/arduino_pwm_generator.cpp.
 *
 * Implements \c QQmlParserStatus for use from QML.
 *
 * \sa QSerialPort
 * \sa QSerialPortInfo
 * \sa QQmlParserStatus
 */
class DisplayController : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    //! Toggle serial communication with the PWM generator MCU.
    /*!
    * Defaults to \c false.
    *
    * \sa setEnabled
    * \sa enabledChanged
    * \sa serialPortName
    * \sa baudRate
    */
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

    //! Serial port device filename used for communication with the PWM generator MCU.
    /*!
    * Defaults to \c /dev/ttyUSB0.
    *
    * \sa setSerialPortName
    * \sa serialPortNameChanged
    * \sa baudRate
    */
    Q_PROPERTY(QString serialPortName READ serialPortName WRITE setSerialPortName NOTIFY serialPortNameChanged)

    //! Baud rate used for serial communication with the PWM generator MCU.
    /*!
    * Defaults to \c 115200.
    *
    * \sa setBaudRate
    * \sa baudRateChanged
    * \sa serialPortName
    */
    Q_PROPERTY(qint32 baudRate READ baudRate WRITE setBaudRate NOTIFY baudRateChanged)

    //! Toggle the display on or off.
    /*!
    * When turned on, the display brightness is set to the current value of \ref brightness.
    *
    * When turned off, the display brightness is set to \c 0 (without changing \ref brightness).
    *
    * Defaults to \c true.
    *
    * \sa setOn
    * \sa onChanged
    * \sa brightness
    */   
    Q_PROPERTY(bool on READ on WRITE setOn NOTIFY onChanged)

    //! Display brightness level while on.
    /*!
    * Display brightness is set in a range between \c 0.0 and \c 1.0.
    *
    * This property is independent of the value of the property \ref on.
    *
    * Defaults to \c 1.0.
    *
    * \sa setBrightness
    * \sa brightnessChanged
    * \sa on
    */  
    Q_PROPERTY(qreal brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)

    //! Wait time in seconds after which the display is turned off.
    /*!
    * Can be set to \c 0 to turn off the auto-turnoff behavior.
    *
    * Defaults to \c 20.
    *
    * \sa setIdleTimeout
    * \sa idleTimeoutChanged
    * \sa resetIdleTimeout
    */  
    Q_PROPERTY(int idleTimeout READ idleTimeout WRITE setIdleTimeout NOTIFY idleTimeoutChanged)

    //! Duration in milliseconds for an animated fade between the two ends of the brightness range.
    /*!
    * The actual duration of a brightness fade is scaled by the delta between the old and the new
    * brightness levels, as a fraction of the full range of \c 0.0 - \c 1.0.
    *
    * Can be set to \c 0 to disable fading and change to new brightness levels immediately instead.
    *
    * Defaults to \c 400.
    *
    * \sa setFadeDuration
    * \sa fadeDurationChanged
    * \sa fadeEasing
    */     
    Q_PROPERTY(int fadeDuration READ fadeDuration WRITE setFadeDuration NOTIFY fadeDurationChanged)

    //! Easing curve used when fading between display brightness levels.
    /*!
    * Defaults to \c QEasingCurve::InCubic.
    *
    * \sa setFadeEasing
    * \sa fadeEasingChanged
    * \sa fadeDuration
    */     
    Q_PROPERTY(QEasingCurve fadeEasing READ fadeEasing WRITE setFadeEasing NOTIFY fadeEasingChanged)

    public:
        //! Create a display backlight controller.
        /*!
        * @param parent Parent object
        */
        explicit DisplayController(QObject *parent = nullptr);
        ~DisplayController() override;

        //! Whether serial communication with the PWM generator MCU is enabled.
        /*!
        * @return Serial communication on or off.
        * \sa enabled (property)
        * \sa setEnabled
        * \sa enabledChanged
        */ 
        bool enabled() const;

        //! Turn serial communication with the PWM generator MCU on or off.
        /*!
        * @param enabled Serial communication on or off.
        * \sa enabled
        * \sa enabledChanged
        */
        void setEnabled(bool enabled);

        //! The serial port device filename used for communication with the PWM generator MCU.
        /*!
        * @return Serial port device filename.
        * \sa serialPortName (property)
        * \sa setSerialPortName
        * \sa serialPortNameChanged
        * \sa baudRate
        */
        QString serialPortName() const;

        //! Set the serial port device filename used for communication with the PWM generator MCU.
        /*!
        * @param serialPortName Serial port device filename.
        * \sa serialPortName
        * \sa serialPortNameChanged
        * \sa baudRate
        */
        void setSerialPortName(const QString &serialPortName);

        //! The baud rate used for serial communication with the PWM generator MCU.
        /*!
        * @return Serial baud rate.
        * \sa baudRate (property)
        * \sa setBaudRate
        * \sa baudRateChanged
        * \sa serialPortName
        */
        qint32 baudRate() const;

        //! Set the baud rate used for serial communication with the PWM generator MCU.
        /*!
        * @param rate Serial baud rate.
        * \sa baudRate
        * \sa baudRateChanged
        * \sa serialPortName
        */
        void setBaudRate(qint32 rate);

        //! Whether the display is on or off.
        /*!
        * When on, the display brightness is set to the current value of \ref brightness.
        *
        * When off, the display brightness is set to \c 0 (without changing \ref brightness).
        *
        * @return Display on or off.
        * \sa on (property)
        * \sa setOn
        * \sa onChanged
        * \sa brightness
        */
        bool on() const;

        //! Turn the display on or off.
        /*!
        * When turned on, the display brightness is set to the current value of \ref brightness.
        *
        * When turned off, the display brightness is set to \c 0 (without changing \ref brightness).
        *
        * @param on Display on or off.
        * \sa on
        * \sa onChanged
        * \sa brightness
        */
        void setOn(bool on);
        
        //! The display brightness level while on.
        /*!
        * This property is independent of the value of the property \ref on.
        *
        * @return Display brightness level between \c 0.0 and \c 1.0.
        * \sa brightness (property)
        * \sa setBrightness
        * \sa brightnessChanged
        * \sa on
        */
        qreal brightness() const;

        //! Set the display brightness level while on.
        /*!
        * @param brightness Display brightness level between \c 0.0 and \c 1.0.
        * \sa brightness
        * \sa brightnessChanged
        * \sa on
        */
        void setBrightness(qreal brightness);

        //! The wait time in seconds after which the display is turned off.
        /*!
        * @return Wait time in seconds.
        * \sa idleTimeout (property)
        * \sa setIdleTimeout
        * \sa idleTimeoutChanged
        * \sa resetIdleTimeout
        */
        int idleTimeout() const;

        //! Set the wait time in seconds after which the display is turned off.
        /*!
        * Can be set to \c 0 to turn off the auto-turnoff  behavior.
        *
        * @param timeout Wait time in seconds.
        * \sa idleTimeout
        * \sa idleTimeoutChanged
        * \sa resetIdleTimeout
        */
        void setIdleTimeout(int timeout);

        //! Reset the auto-turnoff timer.
        /*!
        * Begins a new wait with the duration set as \ref idleTimeout.
        *
        * \sa idleTimeout
        * \sa setIdleTimeout
        * \sa idleTimeoutChanged
        * \sa resetIdleTimeout
        */
        Q_INVOKABLE void resetIdleTimeout();

        //! The duration in milliseconds for an animated fade between the two ends of the brightness range.
        /*!
        * @return Duration in milliseconds.
        * \sa fadeDuration (property)
        * \sa setFadeDuration
        * \sa fadeDurationChanged
        * \sa fadeEasing
        */
        int fadeDuration() const;

        //! Set the duration in milliseconds for an animated fade between the two ends of the brightness range.
        /*!
        * The actual duration of a brightness fade is scaled by the delta between the old and the new
        * brightness levels, as a fraction of the full range of \c 0.0 - \c 1.0.
        *
        * Can be set to \c 0 to disable fading and change to new brightness levels immediately instead.
        * @param fadeDuration Duration in milliseconds.
        * \sa fadeDuration
        * \sa fadeDurationChanged
        * \sa fadeEasing
        */
        void setFadeDuration(int fadeDuration);

        //! The easing curve used when fading between display brightness levels.
        /*!
        * @return Easing curve.
        * \sa fadeEasing (property)
        * \sa setFadeEasing
        * \sa fadeEasingChanged
        * \sa fadeDuration
        */
        QEasingCurve fadeEasing() const;

        //! Set the easing curve used when fading between display brightness levels.
        /*!
        * @param fadeEasing Easing curve.
        * \sa fadeEasing
        * \sa fadeEasingChanged
        * \sa fadeDuration
        */
        void setFadeEasing(const QEasingCurve &fadeEasing);
        
        //! Implements the \c QQmlParserStatus interface.
        void classBegin() override;
        //! Implements the \c QQmlParserStatus interface.
        void componentComplete() override;

    Q_SIGNALS:
        //! Serial communication with the PWM generator MCU has turned on or off.
        /*!
        * \sa enabled
        * \sa setEnabled
        */
        void enabledChanged() const;

        //! The serial port device filename used for communication with the PWM generator MCU has changed.
        /*!
        * \sa serialPortName
        * \sa setSerialPortName
        */
        void serialPortNameChanged() const;

        //! The baud rate used for serial communication with the PWM generator MCU has changed.
        /*!
        * \sa baudRate
        * \sa setBaudRate
        */
        void baudRateChanged() const;

        //! The display has turned on or off.
        /*!
        * \sa on
        * \sa setOn
        */
        void onChanged() const;
        
        //! The display brightness level while on has changed.
        /*!
        * \sa brightness
        * \sa setBrightness
        */
        void brightnessChanged() const;
        
        //! The wait time in seconds after which the display is turned off has changed.
        /*!
        * \sa idleTimeout
        * \sa setIdleTimeout
        */
        void idleTimeoutChanged() const;
        
        //! The duration in milliseconds for an animated fade between the two ends of the brightness range has changed.
        /*!
        * \sa fadeDuration
        * \sa setFadeDuration
        */
        void fadeDurationChanged() const;

        //! The easing curve used when fading between display brightness levels has changed.
        /*!
        * \sa fadeEasing
        * \sa setFadeEasing
        */
        void fadeEasingChanged() const;
        
    private:
        void connectPwmGenerator();
        void disconnectPwmGenerator();
        void fade(qreal from, qreal to);
        inline void writeBrightness(qreal brightness);

        bool m_enabled;

#ifdef HYELICHT_BUILD_ONBOARD
        QSerialPort m_serialPort;
#endif

        bool m_on;
        
        qreal m_brightness;
        qreal m_pendingBrightness;

        QTimer m_idleTimeoutTimer;

        QVariantAnimation m_fadeAnimation;
        int m_fadeDuration;

        bool m_createdByQML;
        bool m_complete;
};
