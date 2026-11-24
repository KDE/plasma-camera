// SPDX-FileCopyrightText: 2026 Balló György <ballogyor@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QImage>
#include <QQuickPaintedItem>

class QPainter;

// A viewfinder implementation for Qt Quick's software rendering backend.
class SoftwareViewfinder : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QImage image READ image WRITE setImage NOTIFY imageChanged)
    Q_PROPERTY(int imageRotation READ imageRotation WRITE setImageRotation NOTIFY imageRotationChanged)
    Q_PROPERTY(bool imageMirrored READ imageMirrored WRITE setImageMirrored NOTIFY imageMirroredChanged)

public:
    explicit SoftwareViewfinder(QQuickItem *parent = nullptr);

    QImage image() const;
    void setImage(const QImage &image);

    int imageRotation() const;
    void setImageRotation(int rotation);

    bool imageMirrored() const;
    void setImageMirrored(bool mirrored);

    void paint(QPainter *painter) override;

Q_SIGNALS:
    void imageChanged();
    void imageRotationChanged();
    void imageMirroredChanged();

private:
    QImage m_image;
    int m_imageRotation = 0;
    bool m_imageMirrored = false;
};
