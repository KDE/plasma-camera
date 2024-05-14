// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QQmlApplicationEngine>
#include <QtQml>
#include <QUrl>
#include <QIcon>

#include <KAboutData>
#include <KLocalizedString>
#include <KLocalizedContext>

#include "plasmacamera.h"
#include "camerasettings.h"

#ifdef Q_OS_ANDROID
#include <QGuiApplication>
#else
#include <QApplication>
#endif

using namespace Qt::Literals::StringLiterals;

constexpr auto URI = "org.kde.plasmacamera";

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#ifdef Q_OS_ANDROID
    QGuiApplication app(argc, argv);
#else
    QApplication app(argc, argv);
#endif

    KLocalizedString::setApplicationDomain("plasma-camera");

    QCoreApplication::setOrganizationName(u"KDE"_s);
    QCoreApplication::setOrganizationDomain(u"kde.org"_s);
    QCoreApplication::setApplicationName(u"plasma-camera"_s);
    QGuiApplication::setApplicationDisplayName(u"Plasma Camera"_s);
    QGuiApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("camera-photo")));

    KAboutData about(app.applicationName(), app.applicationDisplayName(), app.applicationVersion(), QString(),
                     KAboutLicense::GPL, i18n("© Plasma Mobile Developers"), QString());

    about.addAuthor(i18n("Marco Martin"), QString(), QStringLiteral("mart@kde.org"), QStringLiteral("https://notmart.org"));
    about.addAuthor(i18n("Jonah Brüchert"), QString(), QStringLiteral("jbb@kaidan.im"), QStringLiteral("https://jbbgameich.github.io"));
    about.setProgramLogo(app.windowIcon());

    KAboutData::setApplicationData(about);

    QQmlApplicationEngine engine;

    PlasmaCamera plasmaCamera;
    plasmaCamera.setAboutData(about);
    qmlRegisterSingletonInstance<PlasmaCamera>(URI, 1, 0, "PlasmaCamera", &plasmaCamera);
    qmlRegisterSingletonInstance<CameraSettings>(URI, 1, 0, "CameraSettings", CameraSettings::self());

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, QCoreApplication::instance(), [] {
        CameraSettings::self()->save();
    });

    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));

    engine.load(QUrl(QStringLiteral("qrc:///main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
