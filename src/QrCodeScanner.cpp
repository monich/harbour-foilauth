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

#include "QrCodeScanner.h"
#include "QrCodeImageSource.h"
#include "QrCodeDecoder.h"

#include "HarbourDebug.h"

#include <QtConcurrent>
#include <QtQuick/QQuickItem>
#include <QPainter>
#include <QBrush>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickPaintedItem>

#ifdef HARBOUR_DEBUG
#include <QStandardPaths>
static void saveDebugImage(QImage aImage, QString aFileName)
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) +
            "/foilauth/" + aFileName;
    if (aImage.save(path)) {
        HDEBUG("image saved:" << qPrintable(path));
    }
}
#else
#  define saveDebugImage(image,fileName) ((void)0)
#endif

// ==========================================================================
// QrCodeScanner::Private
// ==========================================================================

class QrCodeScanner::Private : public QObject
{
    Q_OBJECT
public:
    Private(QrCodeScanner* aParent);
    ~Private();

    QrCodeScanner* scanner();
    void scanThread(uint aScanId);
    void start();
    void stop();
    void requestStop();
    void setRotation(int aDegrees);
    void setTryRotated(bool aTryRotated);
    void setViewFinderRect(const QRect& aRect);
    void setViewFinderItem(QQuickItem* aItem);

Q_SIGNALS:
    void scanDone(uint aScanId, QImage aImage, QrCodeDecoder::Result aResult);
    void decodingFinished(QVariantMap Result);
    void needImage();

public Q_SLOTS:
    void onScanDone(uint aScanId, QImage aImage, QrCodeDecoder::Result aResult);
    void onGrabImage();

public:
    QrCodeDecoder* iDecoder;
    QQuickItem* iViewFinderItem;
    QImage iCaptureImage;
    int iRotation;
    bool iTryRotated;
    bool iGrabbing;
    bool iStopScan;
    uint iCurrentScanId;
    uint iNextScanId;

    QMutex iScanMutex;
    QWaitCondition iScanEvent;
    QFuture<void> iScanFuture;

    QRect iViewFinderRect;
    QColor iMarkerColor;
};

QrCodeScanner::Private::Private(QrCodeScanner* aParent) :
    QObject(aParent),
    iDecoder(new QrCodeDecoder),
    iViewFinderItem(NULL),
    iRotation(0),
    iTryRotated(false),
    iGrabbing(false),
    iCurrentScanId(0),
    iNextScanId(1),
    iMarkerColor(QColor(0, 255, 0)) // default green
{
    // Handled on the main thread
    connect(this, SIGNAL(scanDone(uint,QImage,QrCodeDecoder::Result)),
        SLOT(onScanDone(uint,QImage,QrCodeDecoder::Result)),
        Qt::QueuedConnection);

    // Forward needImage emitted by the decoding thread
    connect(this, SIGNAL(needImage()), SLOT(onGrabImage()),
        Qt::QueuedConnection);
}

QrCodeScanner::Private::~Private()
{
    if (iScanFuture.isStarted() && !iScanFuture.isFinished()) {
        requestStop();
        iScanFuture.waitForFinished();
    }
    delete iDecoder;
}

inline QrCodeScanner* QrCodeScanner::Private::scanner()
{
    return qobject_cast<QrCodeScanner*>(parent());
}

void QrCodeScanner::Private::onGrabImage()
{
    if (iViewFinderItem && !iStopScan) {
        QQuickWindow* window = iViewFinderItem->window();
        if (window) {
            // grabbing property allows QML to remove staff from the screen
            // that could interfere with decoding
            iGrabbing = true;
            QrCodeScanner* parentScanner = scanner();
            Q_EMIT parentScanner->grabbingChanged();
            HDEBUG("grabbing image");
            QImage image = window->grabWindow();
            iGrabbing = false;
            Q_EMIT parentScanner->grabbingChanged();
            if (!image.isNull() && !iStopScan) {
                HDEBUG(image);
                iScanMutex.lock();
                iCaptureImage = image;
                iScanEvent.wakeAll();
                iScanMutex.unlock();
            }
        }
    }
}

void QrCodeScanner::Private::scanThread(uint aScanId)
{
    HDEBUG("scan started");

    QrCodeDecoder::Result result;
    QImage image;
    qreal scale = 1;
    bool rotated = false;
    int scaledWidth = 0;

    const int maxWidth = 600;
    const int maxHeight = 800;

    iScanMutex.lock();
    while (!iStopScan && !result.isValid()) {
        while (!iStopScan && !iViewFinderItem) {
            iScanEvent.wait(&iScanMutex);
        }

        QRect viewFinderRect;
        int rotation;
        int tryRotated;

        if (!iStopScan && iViewFinderItem) {
            emit needImage();
            while (!iStopScan && iCaptureImage.isNull()) {
                iScanEvent.wait(&iScanMutex);
            }
        }

        viewFinderRect = iViewFinderRect;
        rotation = iRotation;
        tryRotated = iTryRotated;
        if (!iStopScan) {
            image = iCaptureImage;
            iCaptureImage = QImage();
        } else {
            image = QImage();
        }
        iScanMutex.unlock();

        if (!image.isNull()) {
#if HARBOUR_DEBUG
            QTime time(QTime::currentTime());
#endif
            // Crop the image - we need only the viewfinder
            saveDebugImage(image, "debug_screenshot.bmp");

            // Grabbed image is always in portrait orientation
            rotation %= 360;
            switch (rotation) {
            default:
                HDEBUG("Invalid rotation angle" << rotation);
            case 0:
                image = image.copy(viewFinderRect);
                break;
            case 90:
                {
                    QRect cropRect(image.width() - viewFinderRect.bottom(),
                        viewFinderRect.left(), viewFinderRect.height(),
                        viewFinderRect.width());
                    image = image.copy(cropRect).transformed(QTransform().
                        translate(cropRect.width()/2, cropRect.height()/2).
                        rotate(-90));
                }
                break;
            case 180:
                {
                    QRect cropRect(image.width() - viewFinderRect.right(),
                        image.height() - viewFinderRect.bottom(),
                        viewFinderRect.width(), viewFinderRect.height());
                    image = image.copy(cropRect).transformed(QTransform().
                        translate(cropRect.width()/2, cropRect.height()/2).
                        rotate(180));
                }
                break;
            case 270:
                {
                    QRect cropRect(viewFinderRect.top(),
                        image.height() - viewFinderRect.right(),
                        viewFinderRect.height(), viewFinderRect.width());
                    image = image.copy(cropRect).transformed(QTransform().
                        translate(cropRect.width()/2, cropRect.height()/2).
                        rotate(90));
                }
                break;
            }

            HDEBUG("extracted" << image);
            saveDebugImage(image, "debug_cropped.bmp");

            QImage scaledImage;
            if (image.width() > maxWidth || image.height() > maxHeight) {
                Qt::TransformationMode mode = Qt::SmoothTransformation;
                if (maxWidth * image.height() > maxHeight * image.width()) {
                    scaledImage = image.scaledToHeight(maxHeight, mode);
                    scale = image.height()/(qreal)maxHeight;
                    HDEBUG("scaled to height" << scale << scaledImage);
                } else {
                    scaledImage = image.scaledToWidth(maxWidth, mode);
                    scale = image.width()/(qreal)maxWidth;
                    HDEBUG("scaled to width" << scale << scaledImage);
                }
                saveDebugImage(scaledImage, "debug_scaled.bmp");
            } else {
                scaledImage = image;
                scale = 1;
            }

            QrCodeImageSource* source = new QrCodeImageSource(scaledImage);
            saveDebugImage(source->grayscaleImage(), "debug_grayscale.bmp");

            // Ref takes ownership of ImageSource:
            zxing::Ref<zxing::LuminanceSource> sourceRef(source);

            HDEBUG("decoding screenshot ...");
            result = iDecoder->decode(sourceRef);

            if (!result.isValid() && tryRotated) {
                // try the other orientation for 1D bar code
                QTransform transform;
                transform.rotate(90);
                scaledImage = scaledImage.transformed(transform);
                saveDebugImage(scaledImage, "debug_rotated.bmp");
                HDEBUG("decoding rotated screenshot ...");
                result = iDecoder->decode(scaledImage);
                // We need scaled width for rotating the points back
                scaledWidth = scaledImage.width();
                rotated = true;
            } else {
                rotated = false;
            }
            HDEBUG("decoding took" << time.elapsed() << "ms");
        }
        iScanMutex.lock();
    }
    iScanMutex.unlock();

    if (result.isValid()) {
        HDEBUG("decoding succeeded:" << result.getText() << result.getPoints());
        if (scale > 1 || rotated) {
            // The image could be a) scaled and b) rotated. Convert
            // points to the original coordinate system
            QList<QPointF> points = result.getPoints();
            const int n = points.size();
            for (int i = 0; i < n; i++) {
                QPointF p(points.at(i));
                if (rotated) {
                    const qreal x = p.rx();
                    p.setX(p.ry());
                    p.setY(scaledWidth - x);
                }
                p *= scale;
                HDEBUG(points[i] << "=>" << p);
                points[i] = p;
            }
            result = QrCodeDecoder::Result(result.getText(), points, result.getFormat());
        }
    } else {
        HDEBUG("nothing was decoded");
        image = QImage();
    }

    if (!image.isNull()) {
        const QList<QPointF> points(result.getPoints());
        HDEBUG("image:" << image);
        HDEBUG("points:" << points);
        HDEBUG("format:" << result.getFormat() << result.getFormatName());
        if (!points.isEmpty()) {
            QPainter painter(&image);
            painter.setPen(iMarkerColor);
            QBrush markerBrush(iMarkerColor);
            for (int i = 0; i < points.size(); i++) {
                const QPoint p(points.at(i).toPoint());
                painter.fillRect(QRect(p.x()-3, p.y()-15, 6, 30), markerBrush);
                painter.fillRect(QRect(p.x()-15, p.y()-3, 30, 6), markerBrush);
            }
            painter.end();
            saveDebugImage(image, "debug_marks.bmp");
        }
    }

    emit scanDone(aScanId, image, result);
}

void QrCodeScanner::Private::onScanDone(uint aScanId, QImage aImage,
    QrCodeDecoder::Result aResult)
{
    if (aScanId == iCurrentScanId) {
        HDEBUG("scan" << aScanId << "done");
        iCaptureImage = QImage();
        iCurrentScanId = 0;

        QVariantMap result;
        result.insert("valid", QVariant::fromValue(aResult.isValid()));
        result.insert("text", QVariant::fromValue(aResult.getText()));
        Q_EMIT scanner()->scanningChanged();
        Q_EMIT scanner()->scanFinished(result, aImage);
    } else {
        HDEBUG("exnexpected scan");
    }
}

void QrCodeScanner::Private::requestStop()
{
    if (!iStopScan) {
        iScanMutex.lock();
        iStopScan = true;
        iScanEvent.wakeAll();
        iScanMutex.unlock();
    }
}

void QrCodeScanner::Private::start()
{
    if (!iCurrentScanId) {
        if (iScanFuture.isStarted() && !iScanFuture.isFinished()) {
            requestStop();
            HDEBUG("waiting for previous scan to finish");
            iScanFuture.waitForFinished();
        }
        iStopScan = false;
        iCaptureImage = QImage();
        while (!(iCurrentScanId = iNextScanId++));
        HDEBUG("starting scan" << iCurrentScanId);
        iScanFuture = QtConcurrent::run(this, &Private::scanThread, iCurrentScanId);
        Q_EMIT scanner()->scanningChanged();
    }
}

void QrCodeScanner::Private::stop()
{
    if (iCurrentScanId) {
        HDEBUG("stopping scan" << iCurrentScanId);
        iCurrentScanId = 0;
        requestStop();
        Q_EMIT scanner()->scanningChanged();
    }
}

void QrCodeScanner::Private::setViewFinderRect(const QRect& aRect)
{
    iScanMutex.lock();
    iViewFinderRect = aRect;
    iScanMutex.unlock();
}

void QrCodeScanner::Private::setViewFinderItem(QQuickItem* aItem)
{
    iScanMutex.lock();
    iViewFinderItem = aItem;
    if (aItem) {
        iScanEvent.wakeAll();
    }
    iScanMutex.unlock();
}

void QrCodeScanner::Private::setRotation(int aRotation)
{
    iScanMutex.lock();
    iRotation = aRotation;
    iScanMutex.unlock();
}

void QrCodeScanner::Private::setTryRotated(bool aTryRotated)
{
    iScanMutex.lock();
    iTryRotated = aTryRotated;
    iScanMutex.unlock();
}

// ==========================================================================
// QrCodeScanner
// ==========================================================================

QrCodeScanner::QrCodeScanner(QObject* aParent) :
    QObject(aParent),
    iPrivate(new Private(this))
{
    HDEBUG("QrCodeScanner created");
}

QrCodeScanner::~QrCodeScanner()
{
    HDEBUG("QrCodeScanner destroyed");
    delete iPrivate;
}

int QrCodeScanner::rotation() const
{
    return iPrivate->iRotation;
}

void QrCodeScanner::setRotation(int aRotation)
{
    if (iPrivate->iRotation != aRotation) {
        HDEBUG(aRotation);
        iPrivate->setRotation(aRotation);
        Q_EMIT rotationChanged();
    }
}

bool QrCodeScanner::tryRotated() const
{
    return iPrivate->iTryRotated;
}

void QrCodeScanner::setTryRotated(bool aTryRotated)
{
    if (iPrivate->iTryRotated != aTryRotated) {
        HDEBUG(aTryRotated);
        iPrivate->setTryRotated(aTryRotated);
        Q_EMIT tryRotatedChanged();
    }
}

bool QrCodeScanner::grabbing() const
{
    return iPrivate->iGrabbing;
}

bool QrCodeScanner::scanning() const
{
    return iPrivate->iCurrentScanId != 0;
}

void QrCodeScanner::start()
{
    iPrivate->start();
}

void QrCodeScanner::stop()
{
    iPrivate->stop();
}

const QRect& QrCodeScanner::viewFinderRect() const
{
    return iPrivate->iViewFinderRect;
}

void QrCodeScanner::setViewFinderRect(const QRect& aRect)
{
    if (iPrivate->iViewFinderRect != aRect) {
        HDEBUG(aRect);
        iPrivate->setViewFinderRect(aRect);
        Q_EMIT viewFinderRectChanged();
    }
}

QObject* QrCodeScanner::viewFinderItem() const
{
    return iPrivate->iViewFinderItem;
}

void QrCodeScanner::setViewFinderItem(QObject* aItem)
{
    QQuickItem* item = qobject_cast<QQuickItem*>(aItem);
    if (iPrivate->iViewFinderItem != item) {
        iPrivate->iViewFinderItem = item;
        Q_EMIT viewFinderItemChanged();
    }
}

const QColor& QrCodeScanner::markerColor() const
{
    return iPrivate->iMarkerColor;
}

void QrCodeScanner::setMarkerColor(const QColor& aColor)
{
    if (iPrivate->iMarkerColor != aColor) {
        iPrivate->iMarkerColor = aColor;
        HDEBUG(qPrintable(aColor.name()));
        Q_EMIT markerColorChanged();
    }
}

#include "QrCodeScanner.moc"
