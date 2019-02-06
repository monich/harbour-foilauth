/*
 * Copyright (C) 2019 Jolla Ltd.
 * Copyright (C) 2019 Slava Monich <slava@monich.com>
 *
 * You may use this file under the terms of BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *   3. Neither the names of the copyright holders nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef QRCODE_SCANNER_H
#define QRCODE_SCANNER_H

#include <QRect>
#include <QColor>
#include <QImage>
#include <QObject>
#include <QVariantMap>

class QrCodeScanner : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* viewFinderItem READ viewFinderItem WRITE setViewFinderItem NOTIFY viewFinderItemChanged)
    Q_PROPERTY(QRect viewFinderRect READ viewFinderRect WRITE setViewFinderRect NOTIFY viewFinderRectChanged)
    Q_PROPERTY(QColor markerColor READ markerColor WRITE setMarkerColor NOTIFY markerColorChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(bool grabbing READ grabbing NOTIFY grabbingChanged)
    Q_PROPERTY(bool tryRotated READ tryRotated WRITE setTryRotated NOTIFY tryRotatedChanged)
    Q_PROPERTY(int rotation READ rotation WRITE setRotation NOTIFY rotationChanged)

    class Private;

public:
    QrCodeScanner(QObject* aParent = Q_NULLPTR);
    virtual ~QrCodeScanner();

    int rotation() const;
    void setRotation(int aDegrees);

    bool tryRotated() const;
    void setTryRotated(bool aTryRotated);

    bool grabbing() const;
    bool scanning() const;

    const QRect& viewFinderRect() const;
    void setViewFinderRect(const QRect& aRect);

    QObject* viewFinderItem() const;
    void setViewFinderItem(QObject* aItem);

    const QColor& markerColor() const;
    void setMarkerColor(const QColor& aColor);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

Q_SIGNALS:
    void viewFinderItemChanged();
    void viewFinderRectChanged();
    void markerColorChanged();
    void scanningChanged();
    void grabbingChanged();
    void tryRotatedChanged();
    void rotationChanged();
    void scanFinished(QVariantMap result, QImage image);

private:
    Private* iPrivate;
};

#endif // QRCODE_SCANNER_H
