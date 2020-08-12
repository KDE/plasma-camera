// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "plasmacamera.h"

PlasmaCamera::PlasmaCamera(QObject *parent)
    : QObject(parent)
{
}

void PlasmaCamera::setAboutData(const KAboutData &aboutData)
{
    m_aboutData = aboutData;
}
