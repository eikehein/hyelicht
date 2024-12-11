/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 * SPDX-FileCopyrightText: 2021-2024 Eike Hein <sho@eikehein.com>
 */

#include "animations/fireanimation.h"
#include "debug.h"
#include "displaycontroller.h"
#include "httpserver.h"
#include "ledstrip.h"
#include "remoteshelfmodel.h"
#include "settings.h"
#include "shelfmodel.h"
#include "version.h"

#include <KAboutData>
#include <KLocalizedContext>
#include <KLocalizedString>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include <QQmlPropertyMap>
#include <QRemoteObjectHost>

#ifdef Q_OS_ANDROID
#include <QColor>
#include <QCoreApplication>
#include <QJniObject>

// From android.view.WindowManager.LayoutParams
#define FLAG_TRANSLUCENT_STATUS 0x04000000
#define FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS 0x80000000

// From android.view.WindowInsetsController
#define APPEARANCE_LIGHT_STATUS_BARS 0x00000008
#define APPEARANCE_LIGHT_NAVIGATION_BARS 0x00000010
#endif

#ifdef Q_OS_ANDROID
Q_DECL_EXPORT
#endif
int main(int argc, char *argv[])
{
    QGuiApplication app {argc, argv};

    KLocalizedString::setApplicationDomain("hyelicht");

    KAboutData aboutData{Hyelicht::createAboutData(QStringLiteral("Hyelicht"),
        QStringLiteral("Hyelicht Controller"))};
    KAboutData::setApplicationData(aboutData);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("hyelicht")));

    auto settings {Settings::self()};

    QCommandLineParser parser;

    QCommandLineOption remotingServerAddressOption {
        {QStringLiteral("r"), QStringLiteral("remotingServerAddress")},
        xi18nc("@option", "Address to contact remoting API server on"),
        QStringLiteral("remotingServerAddress"),
        Settings::remotingServerAddress()
    };

    parser.addOption(remotingServerAddressOption);

#ifdef HYELICHT_BUILD_ONBOARD
    QCommandLineOption headlessOption {
        QStringLiteral("headless"),
        xi18nc("@option", "Don't start the GUI")
    };

    QCommandLineOption onboardOption {
        {QStringLiteral("o"), QStringLiteral("onboard")},
        xi18nc("@option", "Enable onboard hardware backends and services")
    };

    QCommandLineOption simulateShelfOption {
        QStringLiteral("simulate-shelf"),
        xi18nc("@option", "Simulate shelf (don't talk to server or LEDs)")
    };

    QCommandLineOption simulateDisplayOption {
        QStringLiteral("simulate-display"),
        xi18nc("@option", "(With GUI enabled) Simulate the display state (don't configure display)")
    };

    QCommandLineOption disableHttpApiOption {
        QStringLiteral("disableHttpApi"),
        xi18nc("@option", "Disable the HTTP REST API server")
    };

    QCommandLineOption httpListenAddressOption {
        {QStringLiteral("s"), QStringLiteral("httpListenAddress")},
        xi18nc("@option", "Listen address for HTTP REST API server"),
        QStringLiteral("httpListenAddress"),
        Settings::httpListenAddress()
    };

    QCommandLineOption httpPortOption {
        {QStringLiteral("p"), QStringLiteral("httpPort")},
        xi18nc("@option", "Port for HTTP REST API server"),
        QStringLiteral("httpPort"),
        QString::number(Settings::httpPort())
    };

    QCommandLineOption disableRemotingApiOption {
        QStringLiteral("disableRemotingApi"),
        xi18nc("@option", "Disable the remoting API server")
    };

    QCommandLineOption remotingListenAddressOption {
        {QStringLiteral("l"), QStringLiteral("remotingListenAddress")},
        xi18nc("@option", "Listen address for remoting API server"),
        QStringLiteral("remotingListenAddress"),
        Settings::remotingListenAddress()
    };

    parser.addOptions({
        headlessOption,
        onboardOption,
        simulateShelfOption,
        simulateDisplayOption,
        disableHttpApiOption,
        httpListenAddressOption,
        httpPortOption,
        disableRemotingApiOption,
        remotingListenAddressOption
    });
#endif

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    QScopedPointer<QQmlPropertyMap> options {new QQmlPropertyMap};
    options->insert(QStringLiteral("remotingServerAddress"), parser.value(remotingServerAddressOption));
#ifdef HYELICHT_BUILD_ONBOARD
    options->insert(QStringLiteral("onboard"), parser.isSet(onboardOption));
    options->insert(QStringLiteral("simulateShelf"), parser.isSet(simulateShelfOption));
    options->insert(QStringLiteral("simulateDisplay"), parser.isSet(simulateDisplayOption));
    options->insert(QStringLiteral("remotingApi"), parser.isSet(disableRemotingApiOption) ? false : Settings::remotingApi());
    options->insert(QStringLiteral("remotingListenAddress"), parser.value(remotingListenAddressOption));
    options->insert(QStringLiteral("httpApi"), parser.isSet(headlessOption) ? false : Settings::remotingApi());
    options->insert(QStringLiteral("httpListenAddress"), parser.value(httpListenAddressOption));
    options->insert(QStringLiteral("httpPort"), parser.value(httpPortOption));
#else
    options->insert(QStringLiteral("onboard"), false);
#endif
    qmlRegisterSingletonInstance(HYELICHT_DOMAIN_NAME, 1, 0, "Startup", options.get());

    qmlRegisterSingletonInstance(HYELICHT_DOMAIN_NAME, 1, 0, "Settings", settings);

    qmlRegisterType<DisplayController>(HYELICHT_DOMAIN_NAME, 1, 0, "DisplayController");
    qmlRegisterType<HttpServer>(HYELICHT_DOMAIN_NAME, 1, 0, "HttpServer");
    qmlRegisterType<LedStrip>(HYELICHT_DOMAIN_NAME, 1, 0, "LedStrip");
    qmlRegisterType<RemoteShelfModel>(HYELICHT_DOMAIN_NAME, 1, 0, "RemoteShelfModel");
    qmlRegisterType<ShelfModel>(HYELICHT_DOMAIN_NAME, 1, 0, "ShelfModel");

    const char *animationsDomain = QStringLiteral("%1.animations")
        .arg(QStringLiteral(HYELICHT_DOMAIN_NAME)).toUtf8().constData();
    qmlRegisterUncreatableType<AbstractAnimation>(animationsDomain, 1, 0, "AbstractAnimation", QStringLiteral(""));
    qmlRegisterType<FireAnimation>("com.hyerimandeike.hyelicht.animations", 1, 0, "FireAnimation");

    QQmlApplicationEngine engine {&app};

    // For i18n.
    engine.rootContext()->setContextObject(new KLocalizedContext {&engine});

    engine.load(QUrl {QStringLiteral("qrc:///qt/qml/com/hyerimandeike/hyelicht/declarative/main.qml")});

    if (engine.rootObjects().isEmpty()) {
        qCCritical(HYELICHT) << i18n("Application failed to load.");
        return 1;
    }

#ifdef Q_OS_ANDROID
    // Android currently requires this hack to set the status and navigation
    // bar colors.

    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([=]() {
        QJniObject window {QNativeInterface::QAndroidApplication::context()};
        window = window.callObjectMethod("getWindow", "()Landroid/view/Window;");
        window.callMethod<void>("addFlags", "(I)V", FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
        window.callMethod<void>("clearFlags", "(I)V", FLAG_TRANSLUCENT_STATUS);
        window.callMethod<void>("setStatusBarColor", "(I)V", QColor("#FFFFFF").rgba());
        window.callMethod<void>("setNavigationBarColor", "(I)V", QColor("#FFFFFF").rgba());
        QJniObject insetsController = window.callObjectMethod("getInsetsController",
            "()Landroid/view/WindowInsetsController;");
        insetsController.callMethod<void>("setSystemBarsAppearance", "(II)V",
            APPEARANCE_LIGHT_STATUS_BARS | APPEARANCE_LIGHT_NAVIGATION_BARS,
            APPEARANCE_LIGHT_STATUS_BARS | APPEARANCE_LIGHT_NAVIGATION_BARS);
    });
#endif

    return app.exec();
}
