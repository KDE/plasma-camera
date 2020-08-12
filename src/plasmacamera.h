// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
