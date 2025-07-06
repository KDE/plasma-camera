// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include <QIcon>

#include <KAboutData>
#include <KLocalizedString>
#include <KLocalizedContext>

#include "plasmacamera.h"
#include "plasmacameramanager.h"
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
    // Use ffmpeg and force software encoding by removing all hardware encoding methods
    /*
     * On the pixel 3a, we by default try to use hardware encoding, however, it doesn't work
     * - h264_v4l2m2m
     *  - when "encoding" it freezes the app and then crashes it
     *  - ffmpeg -i file.mp4 -vf scale=1280x720,format=nv12 -c:v h264_v4l2m2m test.mp4
     * - libx264
     *  - while is software encoding (and using a very poor quality) it works well and decently quickly
     *  - ffmpeg -i file.mp4 -vf scale=1280x720 -c:v libx264 -preset ultrafast -crf 28 test.mp4
     */
    qputenv("QT_MEDIA_BACKEND", "ffmpeg");
    // qputenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", ",");
    // qputenv("QT_FFMPEG_ENCODING_HW_DEVICE_TYPES", ",");

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

    KAboutData about(
        QApplication::applicationName(),
        QApplication::applicationDisplayName(),
        QApplication::applicationVersion(),
        QString(),
        KAboutLicense::GPL,
        i18n("© Plasma Mobile Developers"),
        QString());

    about.addAuthor(
        i18n("Marco Martin"),
        QString(),
        QStringLiteral("mart@kde.org"),
        QStringLiteral("https://notmart.org"));

    about.addAuthor(
        i18n("Jonah Brüchert"),
        QString(),
        QStringLiteral("jbb@kaidan.im"),
        QStringLiteral("https://jbbgameich.github.io"));

    about.addAuthor(
        i18n("Andrew Wang"),
        QString(),
        QString(),
        QStringLiteral("https://koitu.com/"));

    about.setProgramLogo(QApplication::windowIcon());

    // set about as the information about the app
    KAboutData::setApplicationData(about);

    // register about as a qml singleton
    qmlRegisterSingletonType(URI, 1, 0, "About",
        [](QQmlEngine* engine, QJSEngine *) -> QJSValue {
            // Here we retrieve our aboutData and give it to the QML engine
            // to turn it into a QML type
            return engine->toScriptValue(KAboutData::applicationData()); });

    // register the camera settings as a qml singleton
    qmlRegisterSingletonInstance<CameraSettings>(URI, 1, 0,
        "CameraSettings", CameraSettings::self());
    // save the camera settings as we are able to exit
    QObject::connect(
        QCoreApplication::instance(),
        &QCoreApplication::aboutToQuit,
        QCoreApplication::instance(), [] { CameraSettings::self()->save(); });

    // register the PlasmaCamera and PlasmaCameraManager qml types
    qmlRegisterType<PlasmaCamera>(URI, 1, 0,
        "PlasmaCamera");
    qmlRegisterType<PlasmaCameraManager>(URI, 1, 0,
        "PlasmaCameraManager");

    // load the application from a qml file
    QQmlApplicationEngine engine;

    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.load(QUrl(QStringLiteral("qrc:///main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return QApplication::exec();
}
