/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

#include "ledstrip.h"
#include "debug_ledstrip.h"

#include <KLocalizedString>

#include <cmath>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#define APA102_HEADER_BYTES 4
#define LED_BRIGHTNESS_MASK 0x1F
#define LED_BRIGHTNESS_HIGH_BITS 0xE0

LedStrip::LedStrip(QObject *parent)
    : LedStrip(1, parent)
{
}

LedStrip::LedStrip(int count, QObject *parent)
    : QObject {parent}
    , m_enabled {false}
    , m_deviceName {QStringLiteral("/dev/spidev0.0")}
    , m_frequency {8000000}
    , m_fd {-1}
    , m_connected {false}
    , m_count {count}
    , m_gammaCorrection {false}
    , m_gamma {2.6}
    , m_gammaCorrectedData {nullptr}
    , m_hsvBrightness {false}
    , m_brightnessCorrectedData {nullptr}
    , m_header {nullptr}
    , m_footer {nullptr}
    , m_data {nullptr}
    , m_savedData {nullptr}
    , m_savedSize(0)
    , m_createdByQml {false}
    , m_complete {false}
{
    if (m_count < 1) {
        qCCritical(HYELICHT_LEDSTRIP) << i18n("Strip length bounded to 1 after attempt"
            "to initialize with a 0 or negative strip length, namely: %1", count);
        m_count = 1;
    }

    updateData(m_count);
}

LedStrip::~LedStrip()
{
    if (m_data) {
        free(m_data);
        m_data = nullptr;
    }

    if (m_gammaCorrectedData) {
        free(m_gammaCorrectedData);
        m_gammaCorrectedData = nullptr;
    }

    disconnect();
}

bool LedStrip::enabled() const
{
    return m_enabled;
}

void LedStrip::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;

        if (!m_enabled) {
            disconnect();
        } if (!m_createdByQml || m_complete) {
            connect();
        }

        Q_EMIT enabledChanged();
    }
}

QString LedStrip::deviceName() const
{
    return m_deviceName;
}

void LedStrip::setDeviceName(const QString &deviceName)
{
    if (m_deviceName != deviceName) {
        m_deviceName = deviceName;

        if ((!m_createdByQml || m_complete) && m_enabled) {
            connect();
        }

        Q_EMIT deviceNameChanged();
    }
}

int LedStrip::frequency() const
{
    return m_frequency;
}

void LedStrip::setFrequency(int frequency)
{
    if (m_frequency != frequency) {
        m_frequency = frequency;

        if ((!m_createdByQml || m_complete) && m_enabled) {
            connect();
        }

        Q_EMIT frequencyChanged();
    }
}

bool LedStrip::connected() const
{
    return m_connected;
}

int LedStrip::count() const
{
    return m_count;
}

void LedStrip::setCount(int count)
{
    if (count < 1) {
        count = 1;
        qCCritical(HYELICHT_LEDSTRIP) << i18n("Strip length bounded to 1 after"
            "attempt to set a 0 or negative strip length, namely: %1", count);
    }

    if (m_count != count) {
        if ((!m_createdByQml || m_complete) && m_enabled) {
            updateData(count);

            m_count = count;

            connect();
        } else {
            updateData(count);

            m_count = count;
        }

        Q_EMIT countChanged();
    }
}

bool LedStrip::gammaCorrection() const
{
    return m_gammaCorrection;
}

void LedStrip::setGammaCorrection(bool gammaCorrection)
{
    if (m_gammaCorrection != gammaCorrection) {
        m_gammaCorrection = gammaCorrection;

        if ((!m_createdByQml || m_complete)) {
            updateData(m_count);

            if (m_enabled) {
                show();
            }
        }

        Q_EMIT gammaCorrectionChanged();
    }
}

qreal LedStrip::gamma() const
{
    return m_gamma;
}

void LedStrip::setGamma(qreal gamma)
{
    if (m_gamma != gamma) {
        m_gamma = gamma;

        if ((!m_createdByQml || m_complete) && m_gammaCorrection) {
            updateLut();

            if (m_enabled) {
                show();
            }
        }

        Q_EMIT gammaChanged();
    }
}

bool LedStrip::hsvBrightness() const
{
    return m_hsvBrightness;
}

void LedStrip::setHsvBrightness(bool hsvBrightness)
{
    if (m_hsvBrightness != hsvBrightness) {
        m_hsvBrightness = hsvBrightness;

        if ((!m_createdByQml || m_complete)) {
            updateData(m_count);

            if (m_enabled) {
                show();
            }
        }

        Q_EMIT hsvBrightnessChanged();
    }
}

bool LedStrip::setLed(int index, const QColor &color, int brightness)
{
    if (index < 0 || index >= m_count) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("setLed: Index out of bounds: %1", index);
        return false;
    }

    if (brightness < 0 || brightness > LED_MAX_BRIGHTNESS) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("setLed: Brightness out of bounds: %1", brightness);
        return false;
    }

    uint8_t *ptr {reinterpret_cast<uint8_t *>(&m_data[index])};
    ptr[0] = brightness | LED_BRIGHTNESS_HIGH_BITS;
    ptr[1] = color.blue();
    ptr[2] = color.green();
    ptr[3] = color.red();

    return true;
}

bool LedStrip::fill(int first, int last, const QColor &color, int brightness)
{
    if (first < 0 || first >= m_count) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("fill: 'first' out of bounds: %1", first);
        return false;
    }

    if (last < first || last >= m_count) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("fill: 'last' out of bounds: %1", last);
        return false;
    }

    if (brightness < 0 || brightness > LED_MAX_BRIGHTNESS) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("fill: Brightness out of bounds: %1", brightness);
        return false;
    }

    for (int i {first}; i < last + 1; i++) {
        uint8_t *ptr {reinterpret_cast<uint8_t *>(&m_data[i])};
        ptr[0] = brightness | LED_BRIGHTNESS_HIGH_BITS;
        ptr[1] = color.blue();
        ptr[2] = color.green();
        ptr[3] = color.red();
    }

    return true;
}

QColor LedStrip::color(int index) const
{
    if (index < 0 || index >= m_count) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("Requested color for index out of bounds: %1", index);
    }

    uint8_t *ptr {reinterpret_cast<uint8_t *>(&m_data[index])};

    return QColor(ptr[1], ptr[2], ptr[3]);
}

QColor LedStrip::colorAverage(int first, int last) const
{
    if (first < 0 || first >= m_count) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("colorAverage: 'first' out of bounds: %1", first);
        return QColor {};
    }

    uint8_t *ptr_first {reinterpret_cast<uint8_t *>(&m_data[first])};

    // The average color of a single LED is ... the color of the LED.
    if (first == last || first - last == 1) {
        return QColor {ptr_first[3], ptr_first[2], ptr_first[1]};
    }

    if (last < first || last >= m_count) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("colorAverage: 'last' out of bounds: %1", last);
        return QColor {};
    }

    // Check if all LEDs in the range are set to a uniform color.
    bool same {true};

    for (int i {first + 1}; i < last + 1; i++) {
        uint8_t *ptr {reinterpret_cast<uint8_t *>(&m_data[i])};

        if (ptr[1] != ptr_first[1] || ptr[2] != ptr_first[2] || ptr[3] != ptr_first[3]) {
            same = false;
            break;
        }
    }

    if (same) {
        return QColor {ptr_first[3], ptr_first[2], ptr_first[1]};
    }

    // Average the colors of the LEDs in the range.
    int r {ptr_first[3]};
    int g {ptr_first[2]};
    int b {ptr_first[1]};

    for (int i {first + 1}; i < last + 1; i++) {
        uint8_t *ptr {reinterpret_cast<uint8_t *>(&m_data[i])};
        r += ptr[3] * ptr[3];
        g += ptr[2] * ptr[2];
        b += ptr[1] * ptr[1];
    }

    const int count = (last - first) + 1;

    return QColor {
        static_cast<int>(std::sqrt(r / count)),
        static_cast<int>(std::sqrt(g / count)),
        static_cast<int>(std::sqrt(b / count))
    };
}

bool LedStrip::setColor(int index, const QColor &color)
{
    if (index < 0 || index >= m_count) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("setColor: Index out of bounds: %1", index);
        return false;
    }

    uint8_t *ptr {reinterpret_cast<uint8_t *>(&m_data[index])};
    ptr[1] = color.blue();
    ptr[2] = color.green();
    ptr[3] = color.red();

    return true;
}

bool LedStrip::setColor(int first, int last, const QColor &color)
{
    if (first < 0 || first >= m_count) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("setColor: 'first' out of bounds: %1", first);
        return false;
    }

    if (last < first || last >= m_count) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("setColor: 'last' out of bounds: %1", last);
        return false;
    }

    for (int i {first}; i < last + 1; i++) {
        uint8_t *ptr {reinterpret_cast<uint8_t *>(&m_data[i])};
        ptr[1] = color.blue();
        ptr[2] = color.green();
        ptr[3] = color.red();
    }

    return true;
}

int LedStrip::brightness(int index) const
{
    if (index < 0 || index >= m_count) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("Requested brightness for index out of bounds: %1", index);
    }

    uint8_t *ptr {reinterpret_cast<uint8_t *>(&m_data[index])};

    return ptr[0] & LED_BRIGHTNESS_MASK;
}

int LedStrip::brightnessAverage(int first, int last) const
{
    if (first < 0 || first >= m_count) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("brightnessAverage: 'first' out of bounds: %1", first);
        return 0;
    }

    uint8_t *ptr_first {reinterpret_cast<uint8_t *>(&m_data[first])};

    // The average brightness of a single LED is ... the brightness of the LED.
    if (first == last || first - last == 1) {
        return ptr_first[0];
    }

    if (last < first || last >= m_count) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("brightnessAverage: 'last' out of bounds: %1", last);
        return 0;
    }

    // Check if all LEDs in the range are set to a uniform brightness.
    bool same {true};

    for (int i {first + 1}; i < last + 1; i++) {
        uint8_t *ptr {reinterpret_cast<uint8_t *>(&m_data[i])};

        if (ptr[0] != ptr_first[0]) {
            same = false;
            break;
        }
    }

    if (same) {
        return ptr_first[0] & LED_BRIGHTNESS_MASK;
    }

    // Average the brightness of the LEDs in the range.
    int brightness {ptr_first[0]};

    for (int i {first + 1}; i < last + 1; i++) {
        uint8_t *ptr {reinterpret_cast<uint8_t *>(&m_data[i])};
        brightness += ptr[0] & LED_BRIGHTNESS_MASK;
    }

    const int count = (last - first) + 1;
    return brightness / count;
}

bool LedStrip::setBrightness(int index, int brightness)
{
    if (index < 0 || index >= m_count) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("setBrightness: Index out of bounds: %1", index);
        return false;
    }

    if (brightness < 0 || brightness > LED_MAX_BRIGHTNESS) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("setBrightness: Brightness out of bounds: %1", brightness);
        return false;
    }

    uint8_t *ptr {reinterpret_cast<uint8_t *>(&m_data[index])};
    ptr[0] = brightness | LED_BRIGHTNESS_HIGH_BITS;

    return true;
}

bool LedStrip::setBrightness(int first, int last, int brightness)
{
    if (first < 0 || first >= m_count) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("setBrightness: fill: 'first' out of bounds: %1", first);
        return false;
    }

    if (last < first || last >= m_count) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("setBrightness: fill: 'last' out of bounds: %1", last);
        return false;
    }

    if (brightness < 0 || brightness > LED_MAX_BRIGHTNESS) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("setBrightness: Brightness out of bounds: %1", brightness);
        return false;
    }

    for (int i {first}; i < last + 1; i++) {
        uint8_t *ptr {reinterpret_cast<uint8_t *>(&m_data[i])};
        ptr[0] = brightness | LED_BRIGHTNESS_HIGH_BITS;
    }

    return true;
}

bool LedStrip::reverse()
{
    if (!m_count) {
        return false;
    }

    const int size {static_cast<int>(m_count * sizeof(uint32_t))};
    uint32_t *reversedData = static_cast<uint32_t *>(malloc(size));

    if (!reversedData) {
        qCCritical(HYELICHT_LEDSTRIP) << i18n("Error allocating memory to reverse strip data.");
        return false;
    }

    for (int i {0}; i < m_count; i++) {
        uint8_t *ptr {reinterpret_cast<uint8_t *>(&m_data[(m_count - 1) - 1])};
        uint8_t *ptr_reversed {reinterpret_cast<uint8_t *>(&reversedData[i])};
        ptr_reversed[0] = ptr[0];
        ptr_reversed[1] = ptr[1];
        ptr_reversed[2] = ptr[2];
        ptr_reversed[3] = ptr[3];
    }

    return true;
}

bool LedStrip::clear()
{
    clearInternal(m_data, 0, m_count - 1);

    return true;
}

bool LedStrip::clear(int first, int last)
{
    if (first < 0 || first >= m_count) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("clear: 'first' out of bounds: %1", first);
        return false;
    }

    if (last < first || last >= m_count) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("clear: 'last' out of bounds: %1", last);
        return false;
    }

    clearInternal(m_data, first, last);

    return true;
}

bool LedStrip::show()
{
    if (m_createdByQml && !m_complete) {
        return false;
    }

    if (!m_enabled) {
        return false;
    }

    if (!m_connected) {
        return false;
    }

    if (m_hsvBrightness) {
        if (!m_brightnessCorrectedData) {
            return false;
        }

        for (int i {0}; i < m_count; i++) {

            uint8_t *ptr {reinterpret_cast<uint8_t *>(&m_data[i])};
            uint8_t *ptr_corrected {reinterpret_cast<uint8_t *>(&m_brightnessCorrectedData[i])};
            ptr_corrected[0] = (static_cast<int>(LED_MAX_BRIGHTNESS
                * QColor(ptr[1], ptr[2], ptr[3]).valueF()) | LED_BRIGHTNESS_HIGH_BITS);
            ptr_corrected[1] = ptr[1]; // Color data is not affected.
            ptr_corrected[2] = ptr[2];
            ptr_corrected[3] = ptr[3];
        }
    }

    if (m_gammaCorrection) {
        if (!m_gammaCorrectedData) {
            return false;
        }

        for (int i {0}; i < m_count; i++) {
            uint8_t *ptr {reinterpret_cast<uint8_t *>(m_hsvBrightness ? &m_brightnessCorrectedData[i]
                : &m_data[i])};
            uint8_t *ptr_corrected {reinterpret_cast<uint8_t *>(&m_gammaCorrectedData[i])};
            ptr_corrected[0] = ptr[0]; // No gamma-correction for brightness.
            ptr_corrected[1] = static_cast<uint8_t>(m_lut[ptr[1]]);
            ptr_corrected[2] = static_cast<uint8_t>(m_lut[ptr[2]]);
            ptr_corrected[3] = static_cast<uint8_t>(m_lut[ptr[3]]);
        }
    }

    m_message[1].tx_buf = m_gammaCorrection ? reinterpret_cast<unsigned long>(m_gammaCorrectedData)
        : m_hsvBrightness ? reinterpret_cast<unsigned long>(m_brightnessCorrectedData)
        : reinterpret_cast<unsigned long>(m_data);
    const int ret {ioctl(m_fd, SPI_IOC_MESSAGE(3), m_message)};

    if (ret < 1) {
        qCCritical(HYELICHT_LEDSTRIP) << i18n("Error sending SPI message: %1",
            QString::fromUtf8(strerror(errno)));
        return false;
    }

    return true;
}

void LedStrip::save()
{
    if (!m_savedData) {
        m_savedData = {static_cast<uint32_t *>(malloc(m_count * sizeof(uint32_t)))};

        if (!m_savedData) {
            qCCritical(HYELICHT_LEDSTRIP) << i18n("Error allocating memory to save current strip data.");
            return;
        }

        m_savedSize = static_cast<int>(m_count * sizeof(uint32_t));
    }

    memcpy(m_savedData, m_data, m_savedSize);

    Q_EMIT canRestoreChanged();
}

void LedStrip::forgetSavedData()
{
    free(m_savedData);
    m_savedData = nullptr;
    m_savedSize = 0;
}

bool LedStrip::canRestore() const
{
    return m_savedData != nullptr;
}

bool LedStrip::restore(RestoreOptions options)
{
    if (!canRestore()) {
        qCWarning(HYELICHT_LEDSTRIP) << i18n("Asked to restore saved strip data with no data saved.");
        return false;
    }

    const int currentSize {static_cast<int>(m_count * sizeof(uint32_t))};
    const int smallerSize {m_savedSize < currentSize ? m_savedSize : currentSize};

    if (options.testFlag(RestoreColor) && options.testFlag(RestoreBrightness)) {
        memcpy(m_data, m_savedData, smallerSize);
    } else if (options.testFlag(RestoreColor)) {
        for (int i = 0; i < static_cast<int>(smallerSize / sizeof(uint32_t)); i++) {
            uint8_t *ptr {reinterpret_cast<uint8_t *>(&m_data[i])};
            uint8_t *ptr_saved {reinterpret_cast<uint8_t *>(&m_savedData[i])};
            ptr[1] = ptr_saved[1];
            ptr[2] = ptr_saved[2];
            ptr[3] = ptr_saved[3];
        }
    } else {
        for (int i = 0; i < static_cast<int>(smallerSize / sizeof(uint32_t)); i++) {
            uint8_t *ptr {reinterpret_cast<uint8_t *>(&m_data[i])};
            uint8_t *ptr_saved {reinterpret_cast<uint8_t *>(&m_savedData[i])};
            ptr[0] = ptr_saved[0];
        }
    }

    forgetSavedData();
    Q_EMIT canRestoreChanged();

    return true;
}

void LedStrip::classBegin()
{
    m_createdByQml = true;
}

void LedStrip::componentComplete()
{
    m_complete = true;

    updateData(m_count);
    updateLut();

    if (m_enabled) {
        connect();
    }
}

void LedStrip::connect()
{
    if (m_connected) {
        disconnect();
    }

    int ret {0};

    int fd = open(m_deviceName.toUtf8().data(), O_RDWR);

    if (fd < 0) {
        qCCritical(HYELICHT_LEDSTRIP) << i18n("Unable to open device: %1",
            QString::fromUtf8(strerror(errno)));
        return;
    }

    uint8_t mode {0};
    ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);

    if (ret == -1) {
        qCCritical(HYELICHT_LEDSTRIP) << i18n("Unable to set SPI mode: %1",
            QString::fromUtf8(strerror(errno)));
        return;
    }

    uint8_t bits {8};
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);

    if (ret == -1) {
         qCCritical(HYELICHT_LEDSTRIP) << i18n("Unable to set bits per word: %1",
            QString::fromUtf8(strerror(errno)));
         return;
    }

    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &m_frequency);

    if (ret == -1) {
        qCCritical(HYELICHT_LEDSTRIP) << i18n("Unable to set max speed HZ: %1",
            QString::fromUtf8(strerror(errno)));
        return;
    }

    m_fd = fd;

    m_header = static_cast<uint8_t *>(calloc(APA102_HEADER_BYTES, 1));

    if (!m_header) {
        disconnect();
        qCCritical(HYELICHT_LEDSTRIP) << i18n("Error allocating memory for message header.");
        return;
    }

    uint32_t footerLength {static_cast<uint32_t>((m_count + 15)/16)};
    m_footer = static_cast<uint8_t *>(malloc(footerLength));

    if (!m_footer) {
        disconnect();
        qCCritical(HYELICHT_LEDSTRIP) << i18n("Error allocating memory for message footer.");
        return;
    } else {
        memset(m_footer, 0xFF, footerLength);
    }

    // Zero-initialize.
    memset(&m_message, 0, sizeof(m_message));

    // Header
    m_message[0].tx_buf = reinterpret_cast<unsigned long>(m_header);
    m_message[0].len = APA102_HEADER_BYTES;
    m_message[0].speed_hz = m_frequency;
    m_message[0].bits_per_word = bits;

    // Strip data
    m_message[1].len = m_count * sizeof(uint32_t);
    m_message[1].speed_hz = m_frequency;
    m_message[1].bits_per_word = bits;

    // Footer
    m_message[2].tx_buf = reinterpret_cast<unsigned long>(m_footer);
    m_message[2].len = footerLength;
    m_message[2].speed_hz = m_frequency;
    m_message[2].bits_per_word = bits;

    m_connected = true;
    Q_EMIT connectedChanged();
}

void LedStrip::disconnect()
{
    if (m_fd > -1) {
        close(m_fd);
        m_fd = -1;
    }

    if (m_header) {
        free(m_header);
        m_header = nullptr;
    }

    if (m_footer) {
        free(m_footer);
        m_header = nullptr;
    }

    if (m_connected) {
        m_connected = false;
        Q_EMIT connectedChanged();
    }
}

void LedStrip::updateData(int count)
{
    // Allocate data array.
    if (!m_data) {
        m_data = static_cast<uint32_t *>(malloc(count * sizeof(uint32_t)));

        if (!m_data) {
            qCCritical(HYELICHT_LEDSTRIP) << i18n("Error allocating memory for the strip data.");
        } else {
            clear(); // Initialize data.
        }
    } else if (m_count != count) { // Strip length changed.
        uint32_t *newData {static_cast<uint32_t *>(malloc(count * sizeof(uint32_t)))};

        // Strip length got longer.
        if (count > m_count) {
            memcpy(newData, m_data, m_count * sizeof(uint32_t));
            // Initialize data for the new LEDs at the end.
            // Calling `clearInternal()` as `m_count` may not be updated
            // yet and `clear` adds a bounds check against it.
            clearInternal(newData, m_count, count - 1);
        } else { // Strip length got shorter.
            memcpy(newData, m_data, count * sizeof(uint32_t));
        }

        free(m_data);
        m_data = newData;
    }

    // Allocate array for gamma-corrected data.
    // Gamma correction is performed in `show()`, so we don't need to
    // initialize `clear()` or handle a resize beyond performing a
    // new allocation.
    if (m_gammaCorrection) {
        if (!m_gammaCorrectedData) {
            m_gammaCorrectedData = static_cast<uint32_t *>(malloc(count * sizeof(uint32_t)));

            if (!m_gammaCorrectedData) {
                qCCritical(HYELICHT_LEDSTRIP) << i18n("Error allocating memory for the gamma-corrected strip data.");
            }
        } else if (m_count != count) { // Strip length changed.
            free(m_gammaCorrectedData);
            m_gammaCorrectedData = static_cast<uint32_t *>(malloc(count * sizeof(uint32_t)));
        }
    } else if (m_gammaCorrectedData) { // Gamma correction was disabled.
        free(m_gammaCorrectedData);
        m_gammaCorrectedData = nullptr;
    }

    // Allocate array for HSV-based brightness-corrected data.
    // Brightness correction is performed in `show()`, so we don't need
    // to initialize `clear()` or handle a resize beyond performing a
    // new allocation.
    if (m_hsvBrightness) {
        if (!m_brightnessCorrectedData) {
            m_brightnessCorrectedData = static_cast<uint32_t *>(malloc(count * sizeof(uint32_t)));

            if (!m_brightnessCorrectedData) {
                qCCritical(HYELICHT_LEDSTRIP) << i18n("Error allocating memory for the HSV-based brightness-corrected strip data.");
            }
        } else if (m_count != count) { // Strip length changed.
            free(m_brightnessCorrectedData);
            m_brightnessCorrectedData = static_cast<uint32_t *>(malloc(count * sizeof(uint32_t)));
        }
    } else if (m_brightnessCorrectedData) { // Gamma correction was disabled.
        free(m_brightnessCorrectedData);
        m_brightnessCorrectedData = nullptr;
    }
}

void LedStrip::updateLut()
{
    if (!m_gammaCorrection) {
        m_lut.resize(0);
        return;
    }

    m_lut.resize(256);

    for (int i {0}; i < m_lut.size(); ++i) {
        m_lut[i] = static_cast<uint8_t>(std::pow(i / 255.0, m_gamma) * 255.0 + 0.5);
    }
}

void LedStrip::clearInternal(uint32_t *data, int first, int last)
{
    for (int i {first}; i < last + 1; i++) {
        uint8_t *ptr {reinterpret_cast<uint8_t *>(&data[i])};
        ptr[0] = LED_MAX_BRIGHTNESS | LED_BRIGHTNESS_HIGH_BITS;
        ptr[1] = 0;
        ptr[2] = 0;
        ptr[3] = 0;
    }
}
