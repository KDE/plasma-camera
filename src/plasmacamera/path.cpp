// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-FileCopyrightText: Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "path.h"

#include <QUrl>


QDir PlasmaLibcameraUtils::defaultDirectory(QStandardPaths::StandardLocation type)
{
    QStringList dirCandidates;

    dirCandidates << QStandardPaths::writableLocation(type);
    dirCandidates << QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    dirCandidates << QDir::homePath();
    dirCandidates << QDir::currentPath();
    dirCandidates << QDir::tempPath();

    for (const QString &path : std::as_const(dirCandidates)) {
        QDir dir(path);
        if (dir.exists() && QFileInfo(path).isWritable())
            return dir;
    }

    return QDir();
}

QString generateFileName(const QDir &dir, const QString &prefix, const QString &extension)
{
    // The extension may be empty if Qt is built without the MIME type feature.
    int lastMediaIndex = 0;
    const QStringView maybeDot = !extension.isEmpty() && !extension.startsWith(u'.') ? u"." : u"";
    const auto filesList =
            dir.entryList({ QStringView(u"%1*%2%3").arg(prefix, maybeDot, extension) });
    for (const QString &fileName : filesList) {
        const qsizetype mediaIndexSize =
                fileName.size() - prefix.size() - extension.size() - maybeDot.size();
        const int mediaIndex = QStringView{ fileName }.mid(prefix.size(), mediaIndexSize).toInt();
        lastMediaIndex = qMax(lastMediaIndex, mediaIndex);
    }

    const QString newMediaIndexStr = QStringLiteral("%1").arg(lastMediaIndex + 1, 4, 10, QLatin1Char(u'0'));
    const QString name = prefix + newMediaIndexStr + maybeDot + extension;

    return dir.absoluteFilePath(name);
}

QString PlasmaLibcameraUtils::generateFileName(const QString &requestedName,
                        const QString &extension,
                        int filenamePattern,
                        int outputPath)
{
    /*
     * when filenamePattern=0, requestedName is empty and generateFileName(dir,prefix,extension)
     * is invoked to generate the default output filename like image_0001.jpg
     * otherwise, requestedName is actual datetime and we add prefix, absoluteFilePath and extension to it
     */
    qDebug() << "PlasmaLibcameraUtils::generateFileName" << requestedName << extension << filenamePattern << outputPath;
    using namespace Qt::StringLiterals;

    // if needed and not-existing, let's create DCIM folder
    if ( (outputPath) && (! QDir(QLatin1String("DCIM")).exists() ) ) {
        QDir().mkdir(QLatin1String("DCIM"));
    }
    auto prefix = "image_"_L1;

    if (requestedName.isEmpty()) {
        if (!outputPath) {
            return generateFileName(defaultDirectory(QStandardPaths::PicturesLocation), prefix, extension);
        } else {
            return generateFileName(QDir(QLatin1String("DCIM")), prefix, extension);
        }
    } else {
        switch (filenamePattern) {
            case 0:
            default:
                qDebug() << "filenamePattern=" << filenamePattern << "but requestedName is not empty\n";
                break;
            case 1:
                prefix = "IMG"_L1;
                break;
            case 2:
                prefix = "IMG_"_L1;
                break;
        }
        QString path = requestedName;
        path = path.prepend(prefix);

        const QFileInfo fileInfo{ path };

        if (fileInfo.isRelative() && QUrl(path).isRelative()) {
            if (!outputPath) {
                path = defaultDirectory(QStandardPaths::PicturesLocation).absoluteFilePath(path);
            } else {
                path = QDir(QLatin1String("DCIM")).absoluteFilePath(path);
            }
        }

        if (fileInfo.suffix().isEmpty() && !extension.isEmpty()) {
            // File does not have an extension, so add the suggested one
            if (!path.endsWith(u'.'))
                path.append(u'.');
            path.append(extension);
        }
        return path;
    }
}
