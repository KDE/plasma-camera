#pragma once

#include <QObject>
#include <KAboutData>

class PlasmaCamera : public QObject
{
    Q_OBJECT
    Q_PROPERTY(KAboutData aboutData WRITE setAboutData MEMBER m_aboutData NOTIFY aboutDataChanged)

public:
    explicit PlasmaCamera(QObject *parent = nullptr);

    void setAboutData(const KAboutData &aboutData);

    Q_SIGNAL void aboutDataChanged();

private:
    KAboutData m_aboutData;
};
