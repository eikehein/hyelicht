/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>
 */

#include "debug_hyelichtctl.h"
#include "version.h"

#include <KAboutData>
#include <KLocalizedString>

#include <QColor>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>

int main(int argc, char *argv[])
{
    QCoreApplication app {argc, argv};

    KLocalizedString::setApplicationDomain("hyelicht");

    KAboutData aboutData {Hyelicht::createAboutData(QStringLiteral("hyelichtctl"),
        QStringLiteral("hyelicht command line client"))};
    KAboutData::setApplicationData(aboutData);

    QCommandLineOption serverOption {
        {QStringLiteral("s"), QStringLiteral("server")},
        xi18nc("@option", "Server address of backend"),
        QStringLiteral("address"),
        QStringLiteral("127.0.0.1")
    };

    QCommandLineOption portOption {
        {QStringLiteral("p"), QStringLiteral("port")},
        xi18nc("@option", "Port of backend"),
        QStringLiteral("port"),
        QStringLiteral("8082")
    };

    QCommandLineOption jsonOption {
        {QStringLiteral("j"), QStringLiteral("json")},
        xi18nc("@option", "Output in JSON format")
    };

    QCommandLineParser parser;

    parser.addOptions({
        serverOption,
        portOption,
        jsonOption
    });

    parser.addPositionalArgument(QStringLiteral("command"),
        i18nc("@option", "Command to run"));
    parser.addPositionalArgument(QStringLiteral("args"),
        i18nc("@option", "Arguments"),
        QStringLiteral("[args...]"));

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    const QStringList &args {parser.positionalArguments()};
    const bool json = parser.isSet(jsonOption);

    if (args.isEmpty()) {
        qCCritical(HYELICHTCTL) << i18n("No command specified.");
        return 1;
    }
    
    QScopedPointer<QNetworkAccessManager> networkAccessManager {new QNetworkAccessManager};
    
    const QString &urlTemplate {QStringLiteral("http://%1:%2/v1/%3").arg(parser.value(serverOption))
        .arg(parser.value(portOption))};
    
    auto exitWithError = [=](const QString &error) {
        qCCritical(HYELICHTCTL) << error;
        QMetaObject::invokeMethod(qApp, [=]() { qApp->exit(1); }, Qt::QueuedConnection);
    };
    
    auto handleReply = [=](QNetworkReply *reply) {
        return [=]() {
            if (reply->error()) {
                exitWithError(i18n("Error during network request to server: %1", reply->errorString()));
                return;
            }

            QJsonParseError error;
            const QJsonDocument &response {QJsonDocument::fromJson(reply->readAll(), &error)};

            if (error.error != QJsonParseError::NoError) {
                exitWithError(i18n("Error parsing JSON response from server: %1", reply->errorString()));
                return;
            }
            
            QTextStream out {stdout};
            
            if (json) {
                out << response.toJson(QJsonDocument::Indented);
            } else {
                const QStringList &keys  {response.object().keys()};

                const QString &longestKey {*std::max_element(keys.begin(), keys.end(),
                    [](const QString &a, const QString &b) {
                        return a.length() < b.length();
                    }
                )};
                
                for (const QString &key : keys) {
                    out << QString("%1 = %2")
                        .arg(key.leftJustified(longestKey.length()))
                        .arg(response.object().value(key).toVariant().toString())
                        << Qt::endl;
                }
            }
            
            QMetaObject::invokeMethod(qApp, &QCoreApplication::quit, Qt::QueuedConnection);
        };
    };
    
    auto get = [=, &networkAccessManager](const QString &resource) {
        const QUrl url {urlTemplate.arg(resource)};
        QNetworkReply *reply {networkAccessManager->get(QNetworkRequest(url))};
        QObject::connect(reply, &QNetworkReply::finished, qApp, handleReply(reply));
    };
    
    auto put = [=, &networkAccessManager](const QString &resource, const QString &prop,
        const QVariant &value) {
        QJsonObject body {{prop, QJsonValue::fromVariant(value)}};
        const QUrl url {urlTemplate.arg(resource)};
        QNetworkReply *reply {networkAccessManager->put(QNetworkRequest(url),
            QJsonDocument(body).toJson())};
        QObject::connect(reply, &QNetworkReply::finished, qApp, handleReply(reply));
    };
    auto commandStatus = [=]() {
        if (args.length() == 1) {
            get(QStringLiteral("shelf"));
        } else {
            exitWithError(i18n("Too many arguments."));
        }
    };

    auto commandShelfGenericBool = [=](const QString &prop) {
        if (args.length() == 1) {
            get(QStringLiteral("shelf/%1").arg(prop));
        } else if (args.length() == 2) {
            const QString &value {args.at(1)};

            if (QVariant(value).canConvert(QMetaType::Bool)) {
                put(QStringLiteral("shelf/%1").arg(prop), prop, value);
            } else {
                exitWithError(i18n("Not a valid argument: %1", value.trimmed()));
            }
        } else {
            exitWithError(i18n("Too many arguments."));
        }
    };

    auto commandEnableDisable = [=](bool enable) {
        if (args.length() == 1) {
            put(QStringLiteral("shelf/enabled"), QStringLiteral("enabled"), enable);
        } else {
            exitWithError(i18n("Too many arguments."));
        }
    };

    auto commandBrightness = [=]() {
        if (args.length() == 1) {
            get(QStringLiteral("shelf/brightness"));
        } else if (args.length() == 2) {
            const QString &value {args.at(1)};
            bool ok {true};
            const qreal brightness {QVariant(value).toFloat(&ok)};

            if (ok) {
                put(QStringLiteral("shelf/brightness"), QStringLiteral("brightness"), brightness);
            } else {
                exitWithError(i18n("Not a valid brightness: %1", value.trimmed()));
            }
        } else {
            exitWithError(i18n("Too many arguments."));
        }
    };

    auto commandColor = [=]() {
        if (args.length() == 1) {
            get(QStringLiteral("shelf/averageColor"));
        } else if (args.length() == 2) {
            const QString &value {args.at(1)};
            const QColor &color {value};
            bool ok {true};
            const int index {QVariant(value).toInt(&ok)};

            if (color.isValid()) {
                put(QStringLiteral("shelf/averageColor"), QStringLiteral("averageColor"), color);
            } else if (ok) {
                get(QString("squares/%1/averageColor").arg(QString::number(index)));
            } else {
                exitWithError(i18n("Not a valid color or square index: %1", value.trimmed()));
            }
        } else if (args.length() == 3) {
            bool ok {true};
            const int index {QVariant(args.at(1)).toInt(&ok)};
            
            if (!ok) {
                exitWithError(i18n("Not a square index: %1", args.at(1).trimmed()));
            }
            
            const QColor &color {args.at(2)};
            
            if (!color.isValid()) {
                exitWithError(i18n("Not a valid color: %1", args.at(2).trimmed()));
            }
            
            put(QString("squares/%1/averageColor").arg(QString::number(index)),
                QStringLiteral("averageColor"), color);
        } else {
            exitWithError(i18n("Too many arguments."));
        }
    };

    const QString &command {args.at(0)};

    if (command == QStringLiteral("status")) {
        commandStatus();
    } else if (command == QStringLiteral("enabled")) {
        commandShelfGenericBool(QStringLiteral("enabled"));
    } else if (command == QStringLiteral("enable")) {
        commandEnableDisable(true);
    } else if (command == QStringLiteral("disable")) {
        commandEnableDisable(false);
    } else if (command == QStringLiteral("brightness")) {
        commandBrightness();
    } else if (command == QStringLiteral("color")) {
        commandColor();
    } else if (command == QStringLiteral("animating")) {
        commandShelfGenericBool(QStringLiteral("animating"));
    } else {
        qCCritical(HYELICHTCTL) << i18n("Unknown command: %1", command.trimmed());
        return 1;
    }

    return app.exec();
}
