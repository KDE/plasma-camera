// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-FileCopyrightText: Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <qtmultimediaglobal.h>
#include <QStandardPaths>
#include <QDir>

namespace PlasmaLibcameraUtils {
    QDir defaultDirectory(QStandardPaths::StandardLocation type);
    QString generateFileName(const QString &requestedName,
                                    const QString &extension,
                                    int filenamePattern,
                                    int outputPath);
};
