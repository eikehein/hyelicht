/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>
 */

#pragma once

#include <QColor>
#include <QObject>
#include <QQmlParserStatus>

#include <linux/spi/spidev.h>

//! \file

/*!
 * Maximum LED brightness level (31).
 */
#define LED_MAX_BRIGHTNESS 0x1F

//! Connects to and performs painting operations on a strip of SK9822/APA102 LEDs
/*!
 * \ingroup Backend
 *
 * Handles communication with a strip of SK9822/APA102 LEDs using the Linux SPI API.
 * 
 * Features:
 *
 * - Set the device name (property \ref deviceName) and communication speed (property \ref frequency).
 * - Set the strip length (property \ref count).
 * - Set colors and brightness for individual LEDs or ranges (methods \ref setLed, \ref fill and various others).
 * - Get colors and brightness for indivdual LEDs or ranges. For ranges of LEDs, in the form of an average.
 * - Reverse the LED strip data (method \ref reverse).
 * - Toggle optional gamma correction (property \ref gammaCorrection).
 * - Toggle whether LED brightness should be based on the HSV value component of the color data 
 *   (property \ref hsvBrightness).
 * - Write current state to the strip (method \ref show) or clear the strip (method \ref clear).
 * - Save and restore strip state (methods \ref save, \ref restore and others).
 *
 * Implements \c QQmlParserStatus for use from QML.
 *
 * \sa QQmlParserStatus
 */
class LedStrip : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    //! Toggle SPI-based communication with the LED strip.
    /*!
    * Defaults to \c false.
    *
    * \sa setEnabled
    * \sa enabledChanged
    * \sa deviceName
    * \sa frequency
    * \sa connected
    */
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

    //! SPI device filename used to communicate with the LED strip.
    /*!
    * Defaults to \c /dev/spidev0.0.
    *
    * \sa setDeviceName
    * \sa deviceNameChanged
    * \sa frequency
    */
    Q_PROPERTY(QString deviceName READ deviceName WRITE setDeviceName NOTIFY deviceNameChanged)

    //! Clock frequency in Hz used for SPI communication with the LEDs.
    /*!
    * Defaults to \c 8000000 (8 Mhz).
    *
    * \sa setFrequency
    * \sa frequencyChanged
    * \sa deviceName
    */
    Q_PROPERTY(int frequency READ frequency WRITE setFrequency NOTIFY frequencyChanged)

    //! Whether there is an open SPI connection to the LED strip.
    /*!
    * Defaults to \c false.
    *
    * \sa connectedChanged
    * \sa enabled
    */
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

    //! Number of LEDs in the strip.
    /*!
    * Defaults to \c 1.
    *
    * \sa setCount
    * \sa countChanged
    */
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)

    //! Toggle optional gamma correction using embedded LUT.
    /*!
    * Gamma correction is applied during \ref show, just before the color
    * data is written to the LED strip. Saved color data is not affected
    * by this property.
    *
    * Will automatically call \ref show when toggled.
    *
    * If \ref hsvBrightness is enabled, HSV-based brightness derivation is
    * applied before gamma correction.
    *
    * Defaults to \c true.
    *
    * \sa setGammaCorrection
    * \sa gammaCorrectionChanged
    * \sa gamma
    * \sa hsvBrightness
    */
    Q_PROPERTY(bool gammaCorrection READ gammaCorrection WRITE setGammaCorrection NOTIFY gammaCorrectionChanged)

    //! Gamma correction value.
    /*!
    * Will automatically call \ref show when changed.
    *
    * Defaults to \c 2.6.
    *
    * \sa setGammaCorrection
    * \sa gammaCorrectionChanged
    * \sa gamma
    */
    Q_PROPERTY(qreal gamma READ gamma WRITE setGamma NOTIFY gammaChanged)

    //! Toggle brightness based on color HSV value component.
    /*!
    * When enabled, seperately set brightness is ignored. Instead, an LED's
    * brightness between \c and \ref LED_MAX_BRIGHTNESS is set based on the
    * HSV value component of its color.
    *
    * The final brightness is calculated during \ref show. Stored brightness
    * data is not affected by this property.
    *
    * Will automatically call \ref show when toggled.
    *
    * If \ref gammaCorrection is enabled, it is applied after brightness
    * correction.
    *
    * Defaults to \c false.
    *
    * \sa setHsvBrightness
    * \sa hsvBrightnessChanged
    * \sa gammaCorrection
    */
    Q_PROPERTY(bool hsvBrightness READ hsvBrightness WRITE setHsvBrightness NOTIFY hsvBrightnessChanged)

    //! Whether there is saved strip state that can be restored by calling \ref restore().
    /*!
    * \sa canRestoreChanged
    * \sa save
    * \sa forgetSavedData
    * \sa restore
    */
    Q_PROPERTY(bool canRestore READ canRestore NOTIFY canRestoreChanged)

    public:
        //! Used as parameters to \ref restore to choose what saved strip state to restore.
        enum RestoreOption {
            RestoreColor = 0x1,     //!< Restore the color data from the saved strip state.
            RestoreBrightness = 0x2 //!< Restore the brightness data from the saved strip state.
        };
        Q_DECLARE_FLAGS(RestoreOptions, RestoreOption)
        Q_FLAG(RestoreOptions)

        //! Create a strip of default length.
        /*!
        * Creates a strip with a default \ref count of \c 1.
        *
        * @param parent Parent object
        */
        explicit LedStrip(QObject *parent = nullptr);

        //! Create a strip of specific length.
        /*!
        * @param count Initializes the \ref count property. \ref count is the number of LEDs in the strip.
        * @param parent Parent object
        */
        explicit LedStrip(int count, QObject *parent = nullptr);

        //! Cleanup on destruction.
        /*!
        * Will gracefully end SPI-based communication.
        */
        ~LedStrip() override;

        //! Whether SPI-based communication with the LED strip is enabled.
        /*!
        * @return SPI communication on or off.
        * \sa enabled (property)
        * \sa setEnabled
        * \sa enabledChanged
        */
        bool enabled() const;

        //! Turn SPI-based communication with the LED strip on or off.
        /*!
        * @param enabled SPI communication on or off.
        * \sa enabled
        * \sa enabledChanged
        */
        void setEnabled(bool enabled);

        //! The SPI device filename used to communicate with the LED strip.
        /*!
        * @return SPI device filename in use.
        * \sa deviceName (property)
        * \sa setDeviceName
        * \sa deviceNameChanged
        * \sa frequency
        */
        QString deviceName() const;

        //! Set the SPI device filename used to communicate with the LED strip.
        /*!
        * @param deviceName SPI device filename to use.
        * \sa deviceName
        * \sa deviceNameChanged
        * \sa frequency
        */
        void setDeviceName(const QString &deviceName);

        //! Clock frequency in Hz used for SPI communication with the LEDs.
        /*!
        * @return SPI clock frequency in Hz.
        * \sa frequency (property)
        * \sa setFrequency
        * \sa frequencyChanged
        * \sa deviceName
        */
        int frequency() const;

        //! Set the clock frequency in Hz used for SPI communication with the LEDs.
        /*!
        * @param frequency SPI clock frequency in Hz.
        * \sa frequency
        * \sa frequencyChanged
        * \sa deviceName
        */
        void setFrequency(int frequency);

        //! Whether there is an open SPI connection to the LED strip.
        /*!
        * @return SPI connection established or not.
        * \sa connectedChanged
        * \sa enabled
        * \sa setEnabled
        * \sa enabledChanged
        */
        bool connected() const;

        //! The number of LEDs in the strip.
        /*!
        * @return Number of LEDs.
        * \sa count (property)
        * \sa setCount
        * \sa countChanged
        */
        int count() const;

        //! Set the number of LEDs in the strip.
        /*!
        * @param leds Number of LEDs.
        * \sa count
        * \sa countChanged
        */
        void setCount(int leds);

        //! Whether gamma correction is turned on.
        /*!
        * Gamma correction is applied during \ref show, just before the color
        * data is written to the LED strip. Saved color data is not affected
        * by this property.
        *
        * If \ref hsvBrightness is enabled, HSV-based brightness derivation is
        * applied before gamma correction.
        *
        * @return Gamma correction is turned on or off.
        * \sa gammaCorrection (property)
        * \sa setGammaCorrection
        * \sa gammaCorrectionChanged
        * \sa gamma
        * \sa hsvBrightness
        */
        bool gammaCorrection() const;

        //! Turn gamma correction on or off.
        /*!
        * Gamma correction is applied during \ref show, just before the color
        * data is written to the LED strip. Saved color data is not affected
        * by this property.
        *
        * Will automatically call \ref show when toggled.
        *
        * If \ref hsvBrightness is enabled, HSV-based brightness derivation is
        * applied before gamma correction.
        *
        *
        * @param gammaCorrection Gamma correction on or off.
        * \sa gammaCorrection
        * \sa gammaCorrectionChanged
        * \sa gamma
        * \sa hsvBrightness
        */
        void setGammaCorrection(bool gammaCorrection);

        //! The gamma correction value.
        /*!
        * @return Gamma correction value.
        * \sa gamma (property)
        * \sa setGamma
        * \sa gammaChanged
        * \sa gammaCorrection
        */
        qreal gamma() const;

        //! Set the gamma correction value.
        /*!
        *
        * Will automatically call \ref show when changed.
        * @param gamma Gamma correction value.
        * \sa gamma
        * \sa gammaChanged
        * \sa gammaCorrection
        */
        void setGamma(qreal gamma);

        //! Whether brightness is based on color HSV value components.
        /*!
        * The final brightness is calculated during \ref show. Stored brightness
        * data is not affected by this property.
        *
        * If \ref gammaCorrection is enabled, it is applied after brightness
        * correction.
        *
        * @return HSV-based brightness on or off.
        * \sa hsvBrightness (property)
        * \sa setHsvBrightness
        * \sa hsvBrightnessChanged
        * \sa gammaCorrection
        */
        bool hsvBrightness() const;

        //! Set whether brightness is based on color HSV value components.
        /*!
        * When enabled, seperately set brightness is ignored. Instead, an LED's
        * brightness between \c and \ref LED_MAX_BRIGHTNESS is set based on the
        * HSV value component of its color.
        *
        * The final brightness is calculated during \ref show. Stored brightness
        * data is not affected by this property.
        *
        * Will automatically call \ref show when toggled.
        *
        * If \ref gammaCorrection is enabled, it is applied after brightness
        * correction.
        *
        * @param hsvBrightness HSV-based brightness on or off.
        * \sa hsvBrightness
        * \sa hsvBrightnessChanged
        * \sa gammaCorrection
        */
        void setHsvBrightness(bool hsvBrightness);

        //! Changes a specific LED.
        /*!
        * @param index LED to operate on.
        * @param color Color to set the LED to.
        * @param brightness Brightness to set the LED to. Must be between 0 and LED_MAX_BRIGHTNESS.
        * @return Success.
        */
        Q_INVOKABLE bool setLed(int index, const QColor &color, int brightness);

        //! Change a range of LEDs.
        /*!
        * @param first First LED in the range.
        * @param last Last LED in the range.
        * @param color Color to set the LED range to.
        * @param brightness Brightness to set the LED to. Must be between 0 and LED_MAX_BRIGHTNESS. Defaults to LED_MAX_BRIGHTNESS.
        * @return Success.
        */
        Q_INVOKABLE bool fill(int first, int last, const QColor &color, int brightness = LED_MAX_BRIGHTNESS);

        //! Retrieves the color of a specific LED.
        /*!
        * @param index LED to operate on.
        * @return Color of the LED.
        */
        Q_INVOKABLE QColor color(int index) const;

        //! Retrieves the average color of a range of LEDs.
        /*!
        * @param first First LED in the range.
        * @param last Last LED in the range.
        * @return Average color of the LED range.
        */
        Q_INVOKABLE QColor colorAverage(int first, int last) const;

        //! Set the color of a specific LED.
        /*!
        * @param index LED to operate on.
        * @param color Color to set the LED to.
        * @return Success.
        */
        Q_INVOKABLE bool setColor(int index, const QColor &color);

        //! Set the color of a range of LEDs.
        /*!
        * @param first First LED in the range.
        * @param last Last LED in the range.
        * @param color Color to set the LED range to.
        * @return Success.
        */
        Q_INVOKABLE bool setColor(int first, int last, const QColor &color);

        //! Retrieves the brightness of a specific LED.
        /*!
        * @param index LED to operate on.
        * @return Brightness of the LED. Will be between \c 0 and \ref LED_MAX_BRIGHTNESS.
        */
        Q_INVOKABLE int brightness(int index) const;

        //! Retrieves the average brightness of a range of LEDs.
        /*!
        * @param first First LED in the range.
        * @param last Last LED in the range.
        * @return Average brightness of the LED range. Will be between \c 0 and \ref LED_MAX_BRIGHTNESS.
        */
        Q_INVOKABLE int brightnessAverage(int first, int last) const;

        //! Set the brightness of a specific LED.
        /*!
        * @param index LED to operate on.
        * @param brightness Brightness to set the LED to. Must be between \c 0 and \ref LED_MAX_BRIGHTNESS.
        * @return Success.
        */
        Q_INVOKABLE bool setBrightness(int index, int brightness);

        //! Set the brightness of a range of LEDs.
        /*!
        * @param first First LED in the range.
        * @param last Last LED in the range.
        * @param brightness Brightness to set the LED to. Must be between \c 0 and \ref LED_MAX_BRIGHTNESS.
        * @return Success.
        */
        Q_INVOKABLE bool setBrightness(int first, int last, int brightness);

        //! Reverse the LED strip data.
        /*!
        * @return Success.
        */
        Q_INVOKABLE bool reverse();

        //! Clear the LED strip.
        /*!
        * Sets color to black and brightness to \c 0.
        *
        * @return Success.
        */
        Q_INVOKABLE bool clear();

        //! Clear a range of LEDs.
        /*!
        * Sets color to black and brightness to \c 0.
        *
        * @param first First LED in the range.
        * @param last Last LED in the range.
        * @return Success.
        */
        Q_INVOKABLE bool clear(int first, int last);

        //! Write latest state to the LED strip.
        /*!
        * Updates the LED strip with new state after painting operations.
        *
        * If \ref gammaCorrection is \c true, the color data will be gamma-corrected
        * at this time before writing it to the strip.
        *
        * @return Success.
        * \sa gammaCorrection
        */
        Q_INVOKABLE bool show();

        //! Save current strip state for later restoration.
        /*!
        * \sa forgetSavedData
        * \sa canRestore
        * \sa canRestoreChanged
        * \sa restore
        */
        Q_INVOKABLE void save();

        //! Forget saved strip data.
        /*!
        * \sa save
        * \sa canRestore
        * \sa canRestoreChanged
        * \sa restore
        */
        Q_INVOKABLE void forgetSavedData();

        //! Whether there is saved strip state that can be restored by calling \ref restore().
        /*!
        * @return Saved strip data available.
        * \sa save
        * \sa forgetSavedData
        * \sa canRestoreChanged
        * \sa restore
        */
        Q_INVOKABLE bool canRestore() const;

        //! Restore saved strip data if available.
        /*!
        * @param options Choose the strip data to restore. Combination of \ref RestoreOption flags.
        * @return Success
        * \sa RestoreOption
        * \sa save
        * \sa forgetSavedData
        * \sa canRestore
        * \sa canRestoreChanged
        */
        Q_INVOKABLE bool restore(RestoreOptions options);

        //! Implements the \c QQmlParserStatus interface.
        void classBegin() override;
        //! Implements the \c QQmlParserStatus interface.
        void componentComplete() override;
        
    Q_SIGNALS:
        //! SPI-based communication with the LED strip has turned on or off.
        /*!
        * \sa enabled
        * \sa setEnabled
        */
        void enabledChanged() const;

        //! The SPI device filename used to communicate with the LED strip has changed.
        /*!
        * \sa deviceName
        * \sa setDeviceName
        */
        void deviceNameChanged();

        //! The clock frequency in Hz used for SPI communication with the LEDs has changed.
        /*!
        * \sa frequency
        * \sa setFrequency
        */
        void frequencyChanged() const;

        //! Whether there is an open SPI connection to the LED strip has changed.
        /*!
        * \sa connected
        * \sa enabled
        * \sa setEnabled
        * \sa enabledChanged
        */
        void connectedChanged();

        //! The number of LEDs in the strip has changed.
        /*!
        * \sa count
        * \sa setCount
        */
        void countChanged() const;

        //! Whether gamma correction is turned on has changed.
        /*!
        * \sa gammaCorrection
        * \sa setGammaCorrection
        */
        void gammaCorrectionChanged();

        //! The gamma correction value has changed.
        /*!
        * \sa gamma
        * \sa setGamma
        */
        void gammaChanged();

        //! Whether brightness is based on color HSV value components has changed.
        /*!
        * \sa hsvBrightness
        * \sa setHsvBrightness
        */
        void hsvBrightnessChanged();

        //! Whether there is saved strip data that can be restored has changed.
        /*!
        * \sa canRestore
        * \sa save
        * \sa forgetSavedData
        * \sa restore
        */
        void canRestoreChanged();

    private:
        void connect();
        void disconnect();
        void updateData(int count);
        void updateLut();
        void clearInternal(uint32_t *data, int first, int last);

        bool m_enabled;

        QString m_deviceName;
        int m_frequency;
        int m_fd;
        bool m_connected;

        int m_count;
        
        bool m_gammaCorrection;
        long double m_gamma;
        QByteArray m_lut;
        uint32_t *m_gammaCorrectedData;

        bool m_hsvBrightness;
        uint32_t *m_brightnessCorrectedData;

        uint8_t *m_header;
        spi_ioc_transfer m_message[3];
        uint8_t *m_footer;

        uint32_t *m_data;
        uint32_t *m_savedData;
        int m_savedSize;

        bool m_createdByQml;
        bool m_complete;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(LedStrip::RestoreOptions)
