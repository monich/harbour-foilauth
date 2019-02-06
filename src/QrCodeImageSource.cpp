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

#include "QrCodeImageSource.h"

QrCodeImageSource::QrCodeImageSource(QImage aImage) :
    zxing::LuminanceSource(aImage.width(), aImage.height())
{
    if (aImage.depth() == 32) {
        iImage = aImage;
    } else {
        iImage = aImage.convertToFormat(QImage::Format_RGB32);
    }

    const int height =  getHeight();
    iGrayRows = new zxing::byte*[height];
    memset(iGrayRows, 0, sizeof(iGrayRows[0]) * height);
}

QrCodeImageSource::~QrCodeImageSource()
{
    const int height =  iImage.height();
    for (int i = 0; i < height; i++) {
        delete [] iGrayRows [i];
    }
    delete [] iGrayRows;
}

zxing::ArrayRef<zxing::byte> QrCodeImageSource::getRow(int aY, zxing::ArrayRef<zxing::byte> aRow) const
{
    const int width = getWidth();
    if (aRow->size() != width) {
        aRow.reset(zxing::ArrayRef<zxing::byte>(width));
    }
    memcpy(&aRow[0], getGrayRow(aY), width);
    return aRow;
}

zxing::ArrayRef<zxing::byte> QrCodeImageSource::getMatrix() const
{
    const int width = getWidth();
    const int height =  getHeight();
    zxing::ArrayRef<zxing::byte> matrix(width*height);
    zxing::byte* m = &matrix[0];

    for (int y = 0; y < height; y++) {
        memcpy(m, getGrayRow(y), width);
        m += width;
    }

    return matrix;
}

const zxing::byte* QrCodeImageSource::getGrayRow(int aY) const
{
    if (!iGrayRows[aY]) {
        const int width = iImage.width();
        zxing::byte* row = new zxing::byte[width];
        const QRgb* pixels = (const QRgb*)iImage.scanLine(aY);
        for (int x = 0; x < width; x++) {
            const QRgb rgb = *pixels++;
            // This is significantly faster than gGray() but is
            // just as good for our purposes
            row[x] = (zxing::byte)((((rgb & 0x00ff0000) >> 16) +
                ((rgb && 0x0000ff00) >> 8) +
                (rgb & 0xff))/3);
        }
        iGrayRows[aY] = row;
    }
    return iGrayRows[aY];
}

QImage QrCodeImageSource::grayscaleImage() const
{
    const int w = iImage.width();
    const int h =  iImage.height();
    QRgb* buf = (QRgb*)malloc(w * h * sizeof(QRgb));
    QRgb* ptr = buf;
    for (int y = 0; y < h; y++) {
        const zxing::byte* src = getGrayRow(y);
        for (int x = 0; x < w; x++) {
            int g = *src++;
            *ptr++ = qRgb(g, g, g);
        }
    }
    return QImage((uchar*)buf, w, h, QImage::Format_ARGB32, free, buf);
}
