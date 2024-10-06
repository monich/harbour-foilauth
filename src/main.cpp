/*
 * Copyright (C) 2019-2024 Slava Monich <slava@monich.com>
 * Copyright (C) 2019-2022 Jolla Ltd.
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer
 *     in the documentation and/or other materials provided with the
 *     distribution.
 *
 *  3. Neither the names of the copyright holders nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * any official policies, either expressed or implied.
 */

#include "FoilAuthDefs.h"
#include "FoilAuthFavoritesModel.h"
#include "FoilAuthGroupModel.h"
#include "FoilAuthImportModel.h"
#include "FoilAuthModel.h"
#include "FoilAuthSettings.h"
#include "FoilAuthToken.h"
#include "FoilAuth.h"

#include "SailOTP.h"
#include "QrCodeScanner.h"

#include "HarbourDebug.h"
#include "HarbourOrganizeListModel.h"
#include "HarbourProcessState.h"
#include "HarbourQrCodeGenerator.h"
#include "HarbourQrCodeImageProvider.h"
#include "HarbourSelectionListModel.h"
#include "HarbourSingleImageProvider.h"
#include "HarbourSystemState.h"

#include <sailfishapp.h>

#include <gutil_log.h>

#include <MGConfItem>

#include <QtGui/QGuiApplication>
#include <QtMultimedia/QCameraExposure>
#include <QtQuick>

#define CAMERA_DCONF_KEY(name) "/apps/jolla-camera/primary/image/" name

static void register_types(const char* uri, int v1 = 1, int v2 = 0)
{
#define REGISTER_TYPE(uri, v1, v2, Class) \
    qmlRegisterType<Class>(uri, v1, v2, #Class)
#define REGISTER_SINGLETON_TYPE(uri, v1, v2, Class) \
    qmlRegisterSingletonType<Class>(uri, v1, v2, #Class, \
    Class::createSingleton)

    REGISTER_SINGLETON_TYPE(uri, v1, v2, HarbourProcessState);
    REGISTER_SINGLETON_TYPE(uri, v1, v2, HarbourSystemState);
    REGISTER_SINGLETON_TYPE(uri, v1, v2, FoilAuthSettings);
    REGISTER_SINGLETON_TYPE(uri, v1, v2, FoilAuthModel);
    REGISTER_SINGLETON_TYPE(uri, v1, v2, FoilAuth);
    REGISTER_SINGLETON_TYPE(uri, v1, v2, SailOTP);
    REGISTER_TYPE(uri, v1, v2, FoilAuthFavoritesModel);
    REGISTER_TYPE(uri, v1, v2, FoilAuthGroupModel);
    REGISTER_TYPE(uri, v1, v2, FoilAuthImportModel);
    REGISTER_TYPE(uri, v1, v2, HarbourOrganizeListModel);
    REGISTER_TYPE(uri, v1, v2, HarbourQrCodeGenerator);
    REGISTER_TYPE(uri, v1, v2, HarbourSelectionListModel);
    REGISTER_TYPE(uri, v1, v2, HarbourSingleImageProvider);
    REGISTER_TYPE(uri, v1, v2, QrCodeScanner);
    qRegisterMetaType<FoilAuthToken>();
    qRegisterMetaType<QList<FoilAuthToken> >();
}

int main(int argc, char *argv[])
{
    QGuiApplication* app = SailfishApp::application(argc, argv);

    app->setApplicationName(FOILAUTH_APP_NAME);
    register_types(FOILAUTH_QML_IMPORT, 1, 0);

    bool torchSupported = false;

    // Parse Qt version to find out what's supported and what's not
    const char* qver = qVersion();
    HDEBUG("Qt version" << qver);
    if (qver) {
        const QStringList s(QString(qver).split('.'));
        if (s.size() >= 3) {
            int v = QT_VERSION_CHECK(s[0].toInt(), s[1].toInt(), s[2].toInt());
            if (v >= QT_VERSION_CHECK(5,6,0)) {
                // If flash is not supported at all, this key contains [2]
                // at least on the most recent versions of Sailfish OS
                QString flashValuesKey(CAMERA_DCONF_KEY("flashValues"));
                MGConfItem flashValuesConf(flashValuesKey);
                QVariantList flashValues(flashValuesConf.value().toList());
                if (flashValues.size() == 1 &&
                    flashValues.at(0).toInt() == QCameraExposure::FlashOff) {
                    HDEBUG("Flash disabled by" << qPrintable(flashValuesKey));
                } else {
                    torchSupported = true;
                    HDEBUG("Torch supported");
                }
            }
        }
    }

    // Load translations
    QLocale locale;
    QTranslator* tr = new QTranslator(app);
#ifdef OPENREPOS
    // OpenRepos build has settings applet
    const QString transDir("/usr/share/translations");
#else
    const QString transDir = SailfishApp::pathTo("translations").toLocalFile();
#endif
    const QString transFile(FOILAUTH_APP_NAME);
    if (tr->load(locale, transFile, "-", transDir) ||
        tr->load(transFile, transDir)) {
        app->installTranslator(tr);
    } else {
        HDEBUG("Failed to load translator for" << locale <<
            "from" << qPrintable(transDir));
        delete tr;
    }

    gutil_log_timestamp = FALSE;
    gutil_log_func = gutil_log_stdout;
    gutil_log_default.name = FOILAUTH_APP_NAME;
#ifdef DEBUG
    gutil_log_default.level = GLOG_LEVEL_VERBOSE;
#endif

    // Create the view
    QQuickView* view = SailfishApp::createView();
    QQmlContext* context = view->rootContext();
    QQmlEngine* engine = context->engine();

    const QString qrCodeImageProvider("qrcode");
    engine->addImageProvider(qrCodeImageProvider, new HarbourQrCodeImageProvider);

    // Initialize global properties
    context->setContextProperty("QrCodeImageProvider", qrCodeImageProvider);
    context->setContextProperty("FoilAuthAppName", QString(FOILAUTH_APP_NAME));
    context->setContextProperty("TorchSupported", torchSupported);
    context->setContextProperty("FoilAuthMinDigits",
        QVariant::fromValue((int)FoilAuthTypes::MIN_DIGITS));
    context->setContextProperty("FoilAuthMaxDigits",
        QVariant::fromValue((int)FoilAuthTypes::MAX_DIGITS));
    context->setContextProperty("FoilAuthDefaultDigits",
        QVariant::fromValue((int)FoilAuthTypes::DEFAULT_DIGITS));
    context->setContextProperty("FoilAuthDefaultCounter",
        QVariant::fromValue((int)FoilAuthTypes::DEFAULT_COUNTER));
    context->setContextProperty("FoilAuthDefaultTimeShift",
        QVariant::fromValue((int)FoilAuthTypes::DEFAULT_TIMESHIFT));

    // Initialize the view and show it
    view->setTitle(qtTrId("foilauth-app_name"));
    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->showFullScreen();

    int ret = app->exec();

    delete view;
    delete app;
    return ret;
}
