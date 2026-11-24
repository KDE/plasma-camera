// SPDX-FileCopyrightText: 2026 Balló György <ballogyor@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "softwareviewfinder.h"

#include <QPainter>

SoftwareViewfinder::SoftwareViewfinder(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
}

QImage SoftwareViewfinder::image() const
{
    return m_image;
}

void SoftwareViewfinder::setImage(const QImage &image)
{
    m_image = image;
    update();
    Q_EMIT imageChanged();
}

int SoftwareViewfinder::imageRotation() const
{
    return m_imageRotation;
}

void SoftwareViewfinder::setImageRotation(int rotation)
{
    if (m_imageRotation == rotation) {
        return;
    }

    m_imageRotation = rotation;
    update();
    Q_EMIT imageRotationChanged();
}

bool SoftwareViewfinder::imageMirrored() const
{
    return m_imageMirrored;
}

void SoftwareViewfinder::setImageMirrored(bool mirrored)
{
    if (m_imageMirrored == mirrored) {
        return;
    }

    m_imageMirrored = mirrored;
    update();
    Q_EMIT imageMirroredChanged();
}

void SoftwareViewfinder::paint(QPainter *painter)
{
    if (m_image.isNull() || width() <= 0 || height() <= 0) {
        return;
    }

    const bool rotated = m_imageRotation == 90 || m_imageRotation == 270;
    const QSizeF imageSize = rotated
        ? QSizeF(m_image.height(), m_image.width())
        : QSizeF(m_image.size());

    const qreal scale = qMin(width() / imageSize.width(), height() / imageSize.height());

    painter->save();
    painter->translate(width() / 2, height() / 2);
    painter->scale(m_imageMirrored ? -scale : scale, scale);
    painter->rotate(m_imageRotation);
    painter->drawImage(QPointF(-m_image.width() / 2.0, -m_image.height() / 2.0), m_image);
    painter->restore();
}
