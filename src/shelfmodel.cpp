/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

#include "shelfmodel.h"
#include "debug.h"
#include "debug_remoting.h"
#include "rep_remoteshelfmodeliface_merged.h"

#include <KLocalizedString>

#include <QColor>
#include <QMetaEnum>
#include <QRemoteObjectHost>

#include <cmath>

ShelfModel::ShelfModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_ledStrip {nullptr}
    , m_enabled {false}
    , m_rows {4}
    , m_columns {5}
    , m_density {20}
    , m_wallThickness {1}
    , m_brightness {1.0}
    , m_animateBrightnessTransitions {true}
    , m_pendingBrightnessTransition {false}
    , m_averageColor {QStringLiteral("white")}
    , m_animateAverageColorTransitions {true}
    , m_transitionDuration {400}
    , m_animating {false}
    , m_remotingEnabled {true}
    , m_listenAddress {QStringLiteral("tcp://0.0.0.0:8042")}
    , m_remotingServer {nullptr}
    , m_createdByQml {false}
    , m_complete {false}
{
    m_brightnessTransition.setDuration(m_transitionDuration);

    QObject::connect(&m_brightnessTransition, &QVariantAnimation::valueChanged, this,
        [=](const QVariant &value) {
            Q_UNUSED(value)

            // Ignore `valueChanged` emissions stemming from calls to
            // `setStartValue`/`setEndValue`.
            if (m_brightnessTransition.state() != QAbstractAnimation::Running) {
                return;
            }

            syncBrightness();

            Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
        }
    );

    QObject::connect(&m_brightnessTransition, &QVariantAnimation::stateChanged, this,
        [=](const QAbstractAnimation::State newState, const QAbstractAnimation::State oldState) {
            Q_UNUSED(oldState)

            // Stop a running animation after the shelf has been faded out.
            if (!m_enabled && newState == QAbstractAnimation::Stopped) {
                updateAnimation();
            }
        }
    );

    m_averageColorTransition.setDuration(m_transitionDuration);

    QObject::connect(&m_averageColorTransition, &QVariantAnimation::valueChanged, this,
        [=](const QVariant &value) {
            // Ignore `valueChanged` emissions stemming from calls to
            // `setStartValue`/`setEndValue`.
            if (m_averageColorTransition.state() != QAbstractAnimation::Running) {
                return;
            }

            if (!m_ledStrip) {
                return;
            }

            setRangesToColor(value.value<QColor>());
            m_ledStrip->show();

            Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
        }
    );
}

ShelfModel::~ShelfModel()
{
}

LedStrip *ShelfModel::ledStrip() const
{
    return m_ledStrip;
}

void ShelfModel::setLedStrip(LedStrip *ledStrip)
{
    if (m_ledStrip != ledStrip) {
        beginResetModel();

        m_ledStrip = ledStrip;

        if (m_animation) {
            m_animation->setLedStrip(m_ledStrip);
        }

        if (m_ledStrip) {
            if (!m_createdByQml || m_complete) {
                updateLedStrip();
                syncBrightness();
            }

            // Here for correctness - but this class makes other assumptions
            // about the length of the strip, as it's specific to a particular
            // shelf.
            QObject::connect(m_ledStrip, &LedStrip::countChanged, this,
                [=]() {
                    beginResetModel();

                    if (!m_createdByQml || m_complete) {
                        if (!m_animating) {
                            setRangesToColor(m_averageColor);
                        }

                        syncBrightness();
                    }

                    endResetModel();

                    Q_EMIT averageColorChanged(averageColor());
                }
            );
        }

        endResetModel();

        Q_EMIT ledStripChanged();
    }
}

bool ShelfModel::enabled() const
{
    return m_enabled;
}

void ShelfModel::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;

        if (!m_createdByQml || m_complete) {
            abortTransitions();

            // When the shelf is enabled while it is fully painted black,
            // repaint it fully white implicitly for reasonable default
            // behavior.
            if (enabled && !m_animating) {
                if (averageColor() == QStringLiteral("black")) {
                    setAverageColor(QStringLiteral("white"));
                }
            }

            // Wait for the next animation frame to call `LedStrip::show`
            // and update the views, so we don't briefly flash white (the
            // default fill set up earlier) if we're enabling both shelf
            // and animation at the same time.
            if (!(enabled && m_animating)) {
                if (m_animateBrightnessTransitions) {
                    // NOTE: We don't call `updateAnimation` here as it will be
                    // called when the transition animation finishes. This allows
                    // us to fade out with the animation still running when the
                    // shelf is disabled.
                    transitionToCurrentBrightness();
                } else {
                    syncBrightness(false /* show */);
                    updateAnimation();

                    m_ledStrip->show();

                    Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
                }
            } else {
                m_pendingBrightnessTransition = true;

                updateAnimation();
            }
        } else {
            Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
        }

        Q_EMIT enabledChanged(m_enabled);
    }
}

int ShelfModel::rows() const
{
    return m_rows;
}

void ShelfModel::setRows(int rows)
{
    if (rows < 1) {
        if (rows == 1) {
            qCWarning(HYELICHT) << i18n("setRows: '%1' rows requested, but cannot be lower than 1."
            "Already at 1.", rows);
        } else {
            qCWarning(HYELICHT) << i18n("setRows: '%1' rows requested, but cannot be lower than 1."
            "Setting 1.", rows);

            rows = 1;
        }
    }

    if (m_rows != rows) {
        m_rows = rows;

        if ((!m_createdByQml || m_complete) && m_ledStrip) {
            beginResetModel();

            updateLedStrip();

            endResetModel();
        }

        Q_EMIT rowsChanged(m_rows);
    }
}

int ShelfModel::columns() const
{
    return m_columns;
}

void ShelfModel::setColumns(int columns)
{
    if (columns < 1) {
        if (columns == 1) {
            qCWarning(HYELICHT) << i18n("setColumns: '%1' rows requested, but cannot be lower than 1."
            "Already at 1.",
            columns);
        } else {
            qCWarning(HYELICHT) << i18n("setColumns: '%1' rows requested, but cannot be lower than 1."
            "Setting 1.", columns);

            columns = 1;
        }
    }

    if (m_columns != columns) {
        m_columns = columns;

        if ((!m_createdByQml || m_complete) && m_ledStrip) {
            beginResetModel();

            updateLedStrip();

            endResetModel();
        }

        Q_EMIT columnsChanged(m_columns);
    }
}

int ShelfModel::density() const
{
    return m_density;
}

void ShelfModel::setDensity(int density)
{
    if (density < 1) {
        if (density == 1) {
            qCWarning(HYELICHT) << i18n("setDensity: '%1' rows requested, but cannot be lower than 1."
            "Already at 1.",
            density);
        } else {
            qCWarning(HYELICHT) << i18n("setDensity: '%1' rows requested, but cannot be lower than 1."
            "Setting 1.", density);

            density = 1;
        }
    }

    if (m_density != density) {
        m_density = density;

        if ((!m_createdByQml || m_complete) && m_ledStrip) {
            beginResetModel();

            updateLedStrip();

            endResetModel();
        }

        Q_EMIT densityChanged(m_density);
    }
}

int ShelfModel::wallThickness() const
{
    return m_wallThickness;
}

void ShelfModel::setWallThickness(int thickness)
{
    if (thickness < 0) {
        if (thickness == 0) {
            qCWarning(HYELICHT) << i18n("setWallThickness: '%1' rows requested, but cannot be lower than 0."
            "Already at 0.",
            thickness);
        } else {
            qCWarning(HYELICHT) << i18n("setWallThickness: '%1' rows requested, but cannot be lower than 1."
            "Setting 1.", thickness);

            thickness = 0;
        }
    }

    if (m_wallThickness != thickness) {
        m_wallThickness = thickness;

        if ((!m_createdByQml || m_complete) && m_ledStrip) {
            beginResetModel();

            updateLedStrip();

            endResetModel();
        }

        Q_EMIT wallThicknessChanged(m_wallThickness);
    }
}

qreal ShelfModel::brightness() const
{
    return m_brightness;
}

void ShelfModel::setBrightness(qreal brightness)
{
    if (m_brightness != brightness) {
        if ((!m_createdByQml || m_complete) && m_ledStrip) {
            if (m_animateBrightnessTransitions) {
                m_brightnessTransition.stop();

                // Implicitly enable the shelf when a brightness above zero is
                // requested.
                if (brightness > 0.0) {
                    // This will sync the currently set brightness, so as to
                    // not skip ahead to the end of the planned transition we
                    // must do this before updating the member variable.
                    setEnabled(true);
                }

                m_brightness = brightness;

                qreal currentAverageBrightness {0.0};

                for (int i {0}; i < rowCount(); ++i) {
                    const QPair <int, int> &range {rowIndexToRange(i)};
                    currentAverageBrightness += m_ledStrip->brightnessAverage(range.first, range.second);
                }

                currentAverageBrightness = (currentAverageBrightness / rowCount()) / LED_MAX_BRIGHTNESS;
                const qreal delta {std::abs(brightness - currentAverageBrightness)
                    ? std::abs(brightness - currentAverageBrightness) : 0};

                m_brightnessTransition.setDuration(m_transitionDuration * delta);
                m_brightnessTransition.setStartValue(currentAverageBrightness);
                m_brightnessTransition.setEndValue(brightness);
                m_brightnessTransition.start();
            } else {
                m_brightness = brightness;

                // Implicitly enable the shelf.
                if (!m_enabled) {
                    // Will call `syncBrightness`.
                    setEnabled(true);
                } else {
                    syncBrightness();
                }

                Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
            }
        } else {
            m_brightness = brightness;

            Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
        }

        Q_EMIT brightnessChanged(m_brightness);
    }
}

bool ShelfModel::animateBrightnessTransitions() const
{
    return m_animateBrightnessTransitions;
}

void ShelfModel::setAnimateBrightnessTransitions(bool animate)
{
    if (m_animateBrightnessTransitions != animate) {
        m_animateBrightnessTransitions = animate;

        if (!animate && m_brightnessTransition.state() == QAbstractAnimation::Running) {
            m_brightnessTransition.stop();

            // Calls `LedStrip::show`.
            syncBrightness();

            Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
        }

        Q_EMIT animateBrightnessTransitionsChanged(m_animateBrightnessTransitions);
    }
}

QColor ShelfModel::averageColor() const
{
    if (!m_ledStrip) {
        return m_averageColor;
    }

    if (m_averageColorTransition.state() == QAbstractAnimation::Running) {
        return m_averageColor;
    }

    int r {0};
    int g {0};
    int b {0};

    for (int i {0}; i < rowCount(); ++i) {
        const QPair <int, int> &range {rowIndexToRange(i)};
        const QColor &color {m_ledStrip->colorAverage(range.first, range.second)};
        r += color.red() * color.red();
        g += color.green() * color.green();
        b += color.blue() * color.blue();
    }

    return QColor {
        static_cast<int>(std::sqrt(r / rowCount())),
        static_cast<int>(std::sqrt(g / rowCount())),
        static_cast<int>(std::sqrt(b / rowCount()))
    };
}

void ShelfModel::setAverageColor(const QColor &color)
{
    if (!m_ledStrip) {
        if (m_averageColor != color) {
            setAnimating(false);

            m_averageColor = color;
            Q_EMIT averageColorChanged(averageColor());

            setEnabled(true);
        }
    }

    if (averageColor() != color) {
        m_averageColor = color;

        if (!m_createdByQml || m_complete) {
            const bool wasAnimating = m_animating;

            if (wasAnimating) {
                // `setAnimating(false)` will cause a call to `LedStrip::restore`,
                // but we don't want to briefly restore an old color before showing
                // the new one.
                m_ledStrip->forgetSavedData();
                setAnimating(false);
            }

            if (m_animateAverageColorTransitions && m_enabled && averageColor() != QStringLiteral("black")) {
                if (wasAnimating) {
                    setRangesToColor(averageColor());
                    m_ledStrip->show();
                    Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
                }

                // Implicitly enable the shelf.
                setEnabled(true);

                m_averageColorTransition.stop();
                m_averageColorTransition.setStartValue(averageColor());
                m_averageColorTransition.setEndValue(color);
                m_averageColorTransition.start();
            } else {
                setRangesToColor(color);

                // Implicitly enable the shelf.
                if (!m_enabled) {
                    // Will call `LedStrip::show` and Q_EMIT `dataChanged` for all
                    // model indices.
                    setEnabled(true);
                } else {
                    m_ledStrip->show();
                    Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
                }

            }
        } else {
            Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
        }

        Q_EMIT averageColorChanged(averageColor());
    }
}

bool ShelfModel::animateAverageColorTransitions() const
{
    return m_animateAverageColorTransitions;
}

void ShelfModel::setAnimateAverageColorTransitions(bool animate)
{
    if (m_animateAverageColorTransitions != animate) {
        m_animateAverageColorTransitions = animate;

        if (!animate && m_averageColorTransition.state() == QAbstractAnimation::Running) {
            m_averageColorTransition.stop();

            setRangesToColor(m_averageColor);

            if (m_enabled) {
                m_ledStrip->show();
            }

            Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
        }

        Q_EMIT animateAverageColorTransitionsChanged(m_animateAverageColorTransitions);
    }
}

int ShelfModel::transitionDuration() const
{
    return m_transitionDuration;
}

void ShelfModel::setTransitionDuration(int duration)
{
    if (m_transitionDuration != duration) {
        m_transitionDuration = duration;

        m_brightnessTransition.setDuration(m_transitionDuration);
        m_averageColorTransition.setDuration(m_transitionDuration);

        Q_EMIT transitionDurationChanged(m_transitionDuration);
    }
}

AbstractAnimation *ShelfModel::animation() const
{
    return m_animation;
}

void ShelfModel::setAnimation(AbstractAnimation *animation)
{
    if (m_animation != animation) {
        if (m_animation) {
            m_animation->disconnect(this);
        }

        m_animation = animation;

        if (m_animation) {
            QObject::connect(m_animation, &AbstractAnimation::destroyed, this,
                [=]() {
                    Q_EMIT animationChanged();
                    setAnimating(false);
                }
            );

            QObject::connect(m_animation, &AbstractAnimation::stateChanged, this,
                [=](QTimeLine::State newState) {
                    if (newState == QTimeLine::Running) {
                        if (m_ledStrip) {
                            m_ledStrip->save();

                            if (m_pendingBrightnessTransition) {
                                const qreal from {m_enabled ? 0.0 : m_brightness};
                                m_ledStrip->setBrightness(0, m_ledStrip->count() - 1,
                                    std::rint(LED_MAX_BRIGHTNESS * from));
                            }
                        }
                    } else {
                        // Don't write out to the strip if we didn't restore any old
                        // data or if we're not enabled.
                        if (m_ledStrip->restore(LedStrip::RestoreColor)) {
                            m_ledStrip->show();
                        }

                        Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
                        Q_EMIT averageColorChanged(averageColor());
                    }
                }
            );

            QObject::connect(m_animation, &AbstractAnimation::frameComplete, this,
                [=]() {
                    if (m_enabled) {
                        Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0));
                        Q_EMIT averageColorChanged(averageColor());

                        if (m_pendingBrightnessTransition) {
                            transitionToCurrentBrightness();

                            m_pendingBrightnessTransition = false;
                        }
                    }
                }
            );

            m_animation->setLedStrip(m_ledStrip);

            updateAnimation();
        } else {
            setAnimating(false);
        }

        Q_EMIT animationChanged();
    }
}

bool ShelfModel::animating() const
{
    return m_animating;
}

void ShelfModel::setAnimating(bool animating)
{
    if (m_animating != animating) {
        m_animating = animating;

        if (!m_createdByQml || m_complete) {
            // Implicitly enable the shelf when asked to
            // animate.
            if (animating && !m_enabled) {
                // Will call `updateAnimation`.
                setEnabled(true);
            } else {
                updateAnimation();
            }
        }

        Q_EMIT animatingChanged(m_animating);
    }
}

QHash<int, QByteArray> ShelfModel::roleNames() const
{
    QHash<int, QByteArray> roles {QAbstractItemModel::roleNames()};

    QMetaEnum e {metaObject()->enumerator(metaObject()->indexOfEnumerator("AdditionalRoles"))};

    for (int i {0}; i < e.keyCount(); ++i) {
        QString key {QString::fromLatin1(e.key(i))};
        key.replace(0, 1, key.at(0).toLower());
        roles.insert(e.value(i), key.toLatin1());
    }

    return roles;
}

int ShelfModel::rowCount(const QModelIndex &parent) const
{
    if (!checkIndex(parent, CheckIndexOption::ParentIsInvalid)) {
        return 0;
    }

    return m_rows * m_columns;
}

QVariant ShelfModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section == 0
        && orientation == Qt::Horizontal
        && role == Qt::DisplayRole)
    {
        return i18nc("@title:column", "Color");
    }

    return QVariant {};
}

QVariant ShelfModel::data(const QModelIndex &index, int role) const
{
    if (!m_ledStrip) {
        return QVariant {};
    }

    if (!checkIndex(index, CheckIndexOption::IndexIsValid
        | CheckIndexOption::ParentIsInvalid)) {
        return QVariant {};
    }

    if (!m_ledStrip) {
        return QVariant {};
    }

    if (!m_enabled && role != AverageBrightness) {
        return QColor {QStringLiteral("black")};
    }

    const QPair<int, int> &range {rowIndexToRange(index.row())};

    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return m_ledStrip->colorAverage(range.first, range.second).name(QColor::HexRgb);
        case Qt::DecorationRole:
        case AverageColor:
            return m_ledStrip->colorAverage(range.first, range.second);
        case AverageRed:
            return m_ledStrip->colorAverage(range.first, range.second).red();
        case AverageGreen:
            return m_ledStrip->colorAverage(range.first, range.second).green();
        case AverageBlue:
            return m_ledStrip->colorAverage(range.first, range.second).blue();
        case AverageBrightness:
            const int averageBrightness {m_ledStrip->brightnessAverage(range.first,
                range.second)};

            if (averageBrightness == 0) {
                return 0.0;
            } else {
                return std::rint(LED_MAX_BRIGHTNESS / averageBrightness);
            }
    }

    return QVariant {};
}

bool ShelfModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!m_ledStrip) {
        return false;
    }

    if (!checkIndex(index, CheckIndexOption::IndexIsValid
        | CheckIndexOption::ParentIsInvalid)) {
        return false;
    }

    if (role != Qt::EditRole) {
        return false;
    }

    if (!value.canConvert(QMetaType(QMetaType::QColor)))
    {
        return false;
    }

    const QColor &newColor {value.value<QColor>()};

    const QPair<int, int> &range {rowIndexToRange(index.row())};
    const QColor &color {m_ledStrip->colorAverage(range.first, range.second)};

    if (color == newColor) {
        return false;
    }

    // Disable animation implicitly.
    if (m_animating) {
        setAnimating(false);
    }

    m_ledStrip->setColor(range.first, range.second, newColor);
    m_ledStrip->show();

    // If the entire shelf was painted black, set the overall state
    // to disabled automatically. `setEnabled(true)` will repaint
    // the shelf fully white in this state, making it an easy (and
    // likely to be used, by frontends / their users) shortcut.
    if (averageColor() == QStringLiteral("black")) {
        setEnabled(false);
    // Implicitly enable the shelf. This will Q_EMIT a data
    // change signal for all model indices, so no need to Q_EMIT
    // `dataChanged()` here.
    } else if (!m_enabled) {
        setEnabled(true);
    } else {
        Q_EMIT dataChanged(index, index);
    }

    Q_EMIT averageColorChanged(averageColor());

    return true;
}

bool ShelfModel::remotingEnabled() const
{
    return m_remotingEnabled;
}

void ShelfModel::setRemotingEnabled(bool enabled)
{
    if (m_remotingEnabled != enabled) {
        m_remotingEnabled = enabled;

        if (!m_createdByQml || m_complete) {
            updateRemoting();
        }

        Q_EMIT remotingEnabledChanged();
    }
}

QUrl ShelfModel::listenAddress() const
{
    return m_listenAddress;
}

void ShelfModel::setListenAddress(const QUrl &url)
{
    if (m_listenAddress != url) {
        m_listenAddress = url;

        if (!m_createdByQml || m_complete) {
            updateRemoting();
        }

        Q_EMIT listenAddressChanged();
    }
}

void ShelfModel::classBegin()
{
    m_createdByQml = true;
}

void ShelfModel::componentComplete()
{
    m_complete = true;

    updateLedStrip();
    updateAnimation();
    syncBrightness(); // Calls `LedStrip::show`.
    updateRemoting();
}

QPair<int, int> ShelfModel::rowIndexToRange(const int rowIndex) const
{
    const int row {rowIndex / m_columns};
    const int indexInRow {row % 2 == 0 ? (m_columns - 1) - std::max(0,
        rowIndex - (row * m_columns)) : rowIndex - (row * m_columns)};
    const int rowLength {m_columns * m_density + (m_columns - 1) * m_wallThickness};
    const int first {(std::max(0, (m_rows - 1) - row) * rowLength)
        + std::max(0, indexInRow * m_density) + (indexInRow * m_wallThickness)};

    return QPair<int, int>(first, first + m_density - 1);
}

void ShelfModel::transitionToCurrentBrightness()
{
    const qreal from {m_enabled ? 0.0 : m_brightness};
    const qreal to {m_enabled ? m_brightness : 0.0};
    const qreal delta {std::abs(m_brightness - to) ? std::abs(m_brightness - to) : 1.0};

    m_brightnessTransition.setDuration(m_transitionDuration * delta);
    m_brightnessTransition.setStartValue(from);
    m_brightnessTransition.setEndValue(to);
    m_brightnessTransition.start();
}

void ShelfModel::syncBrightness(bool show /* Defaults to true */)
{
    if (!m_ledStrip) {
        return;
    }

    if (m_brightnessTransition.state() == QAbstractAnimation::Running) {
        m_ledStrip->setBrightness(0, m_ledStrip->count() - 1,
            std::rint(LED_MAX_BRIGHTNESS * m_brightnessTransition.currentValue().toReal()));
    } else {
        m_ledStrip->setBrightness(0, m_ledStrip->count() - 1,
            m_enabled ? std::rint(LED_MAX_BRIGHTNESS * m_brightness) : 0);
    }

    if (show) { // Defaults to true.
        m_ledStrip->show();
    }
}

void ShelfModel::setRangesToColor(const QColor &color)
{
    if (!m_ledStrip) {
        return;
    }

    m_ledStrip->clear();

    for (int i {0}; i < rowCount(); ++i) {
        const QPair <int, int> &range {rowIndexToRange(i)};
        m_ledStrip->setColor(range.first, range.second, color);
    }
}

void ShelfModel::abortTransitions()
{
    if (m_averageColorTransition.state() == QAbstractAnimation::Running) {
        m_averageColorTransition.stop();
        setRangesToColor(m_averageColor);
    }

    if (m_brightnessTransition.state() == QAbstractAnimation::Running) {
        m_brightnessTransition.stop();
    }
}

void ShelfModel::updateLedStrip()
{
    m_ledStrip->setCount((m_columns * m_density + (m_columns - 1)
        * m_wallThickness) * m_rows);

    if (!m_animating) {
        setRangesToColor(m_averageColor);
    }
}

void ShelfModel::updateAnimation()
{
    if (!m_animation) {
        return;
    }

    if (m_enabled && m_animating) {
        if (m_animation->state() != QTimeLine::Running) {
            m_animation->start();
        }
    } else if (m_brightnessTransition.state() != QAbstractAnimation::Running) {
        if (m_animation->state() == QTimeLine::Running) {
            m_animation->stop();
        }
    }
}

void ShelfModel::updateRemoting()
{
    if (!m_remotingEnabled && m_remotingServer) {
        delete m_remotingServer;
        m_remotingServer = nullptr;
    }

    if (m_remotingEnabled && (m_listenAddress.isEmpty() || !m_listenAddress.isValid()) && m_remotingServer) {
        delete m_remotingServer;
        m_remotingServer = nullptr;

        Q_EMIT remotingEnabledChanged();
    }

    if (m_remotingEnabled && m_listenAddress.isValid()) {
        if (!m_remotingServer) {
            m_remotingServer = new QRemoteObjectHost(this);
        }

        if (!m_remotingServer->setHostUrl(m_listenAddress)) {
            qCCritical(HYELICHT_REMOTING) << i18n("Error starting remoting API server.");
        } else {
            if (!m_remotingServer->enableRemoting(this, QStringLiteral("shelfModel"),
                roleNames().keys().toVector())) {
                qCCritical(HYELICHT_REMOTING) << i18n("Error exporting shelf model on the remoting API server.");

                return;
            }

            if (!m_remotingServer->enableRemoting<RemoteShelfModelIfaceSourceAPI>(this)) {
                qCCritical(HYELICHT_REMOTING) << i18n("Error exporting extended shelf model API on the remoting API server.");

                return;
            }

            qCInfo(HYELICHT_REMOTING) << i18n("Remoting API server now listening on: %1",
                m_remotingServer->hostUrl().toString());
        }
    } else if (!m_listenAddress.isValid()) {
        qCCritical(HYELICHT_REMOTING) << i18n("Failed to start remoting API server due to invalid listen address: %1",
            m_listenAddress.errorString());
    }
}
