#include "plasmacamera.h"

PlasmaCamera::PlasmaCamera(QObject *parent)
    : QObject(parent)
{
}

void PlasmaCamera::setAboutData(const KAboutData &aboutData)
{
    m_aboutData = aboutData;
}
