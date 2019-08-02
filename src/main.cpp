// Qt includes
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include <QUrl>
#include <QIcon>

// KDE includes
#include <KAboutData>
#include <KLocalizedString>


Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("KDE");
    QCoreApplication::setOrganizationDomain("kde.org");
    QCoreApplication::setApplicationName("plasma-camera");
    QGuiApplication::setApplicationDisplayName("Plasma Camera");
    QGuiApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("camera-photo")));

    // About Data
    KAboutData about(app.applicationName(), app.applicationDisplayName(), app.applicationVersion(), QString(),
                     KAboutLicense::GPL, i18n("© Plasma Mobile Developers"), QString());

    about.addAuthor(i18n("Marco Martin"), QString(), QStringLiteral("mart@kde.org"), QStringLiteral("notmart.org"));
    about.addAuthor(i18n("Jonah Brüchert"), QString(), QStringLiteral("jbb@kaidan.im"), QStringLiteral("jbbgameich.github.io"));
    about.setProgramLogo(app.windowIcon());

    KAboutData::setApplicationData(about);


    // QML Engine
    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(QStringLiteral("cameraAboutData"),
                                             QVariant::fromValue(KAboutData::applicationData()));

    engine.load(QUrl(QStringLiteral("qrc:///main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
