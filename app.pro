NAME = foilauth

openrepos {
    PREFIX = openrepos
    DEFINES += OPENREPOS
} else {
    PREFIX = harbour
}

TARGET = $${PREFIX}-$${NAME}
CONFIG += sailfishapp link_pkgconfig
PKGCONFIG += sailfishapp mlite5 glib-2.0 gobject-2.0
QT += qml sql quick dbus multimedia concurrent

QMAKE_CXXFLAGS += -Wno-unused-parameter
QMAKE_CFLAGS += -Wno-unused-parameter

app_settings {
    # This path is hardcoded in jolla-settings
    TRANSLATIONS_PATH = /usr/share/translations
} else {
    TRANSLATIONS_PATH = /usr/share/$${TARGET}/translations
}

CONFIG(debug, debug|release) {
    DEFINES += DEBUG HARBOUR_DEBUG
}

equals(QT_ARCH, arm64){
    message(Linking with OpenSSL)
    PKGCONFIG += libcrypto
}

# Directories
FOIL_UI_REL = foil-ui
FOIL_UI_DIR = $${_PRO_FILE_PWD_}/$${FOIL_UI_REL}
FOIL_UI_QML = $${FOIL_UI_DIR}

HARBOUR_LIB_REL = harbour-lib
HARBOUR_LIB_DIR = $${_PRO_FILE_PWD_}/$${HARBOUR_LIB_REL}
HARBOUR_LIB_INCLUDE = $${HARBOUR_LIB_DIR}/include
HARBOUR_LIB_SRC = $${HARBOUR_LIB_DIR}/src
HARBOUR_LIB_QML = $${HARBOUR_LIB_DIR}/qml

LIBGLIBUTIL_DIR = $${_PRO_FILE_PWD_}/libglibutil
LIBGLIBUTIL_INCLUDE = $${LIBGLIBUTIL_DIR}/include

FOIL_DIR = $${_PRO_FILE_PWD_}/foil
LIBFOIL_DIR = $${FOIL_DIR}/libfoil
LIBFOIL_INCLUDE = $${LIBFOIL_DIR}/include
LIBFOIL_SRC = $${LIBFOIL_DIR}/src

LIBFOILMSG_DIR = $${FOIL_DIR}/libfoilmsg
LIBFOILMSG_INCLUDE = $${LIBFOILMSG_DIR}/include
LIBFOILMSG_SRC = $${LIBFOILMSG_DIR}/src

LIBQRENCODE_DIR = $${_PRO_FILE_PWD_}/libqrencode

ZXING_DIR = $${_PRO_FILE_PWD_}/zxing

# Libraries
LIBS +=  libqrencode.a libzxing.a -ldl

OTHER_FILES += \
    *.desktop \
    qml/*.qml \
    qml/images/*.svg \
    qml/images/*.png \
    icons/*.svg \
    translations/*.ts

INCLUDEPATH += \
    src \
    $${LIBFOIL_SRC} \
    $${LIBFOIL_INCLUDE} \
    $${LIBFOILMSG_INCLUDE} \
    $${LIBGLIBUTIL_INCLUDE} \
    $${LIBQRENCODE_DIR} \
    $${ZXING_DIR}

HEADERS += \
    src/FoilAuth.h \
    src/FoilAuthDefs.h \
    src/FoilAuthFavoritesModel.h \
    src/FoilAuthModel.h \
    src/FoilAuthSettings.h \
    src/FoilAuthToken.h \
    src/FoilAuthTypes.h \
    src/QrCodeDecoder.h \
    src/QrCodeImageSource.h \
    src/QrCodeScanner.h \
    src/SailOTP.h

SOURCES += \
    src/FoilAuth.cpp \
    src/FoilAuthFavoritesModel.cpp \
    src/FoilAuthModel.cpp \
    src/FoilAuthSettings.cpp \
    src/FoilAuthToken.cpp \
    src/main.cpp \
    src/QrCodeDecoder.cpp \
    src/QrCodeImageSource.cpp \
    src/QrCodeScanner.cpp \
    src/SailOTP.cpp

SOURCES += \
    $${LIBFOIL_SRC}/*.c \
    $${LIBFOIL_SRC}/openssl/*.c \
    $${LIBFOILMSG_SRC}/*.c \
    $${LIBGLIBUTIL_DIR}/src/*.c

HEADERS += \
    $${LIBFOIL_INCLUDE}/*.h \
    $${LIBFOILMSG_INCLUDE}/*.h \
    $${LIBGLIBUTIL_INCLUDE}/*.h

# foil-ui

FOIL_UI_COMPONENTS = \
    $${FOIL_UI_QML}/FoilUiAppsWarning.qml \
    $${FOIL_UI_QML}/FoilUiChangePasswordPage.qml \
    $${FOIL_UI_QML}/FoilUiConfirmPasswordDialog.qml \
    $${FOIL_UI_QML}/FoilUiEnterPasswordView.qml \
    $${FOIL_UI_QML}/FoilUiGenerateKeyPage.qml \
    $${FOIL_UI_QML}/FoilUiGenerateKeyView.qml \
    $${FOIL_UI_QML}/FoilUiGenerateKeyWarning.qml \
    $${FOIL_UI_QML}/FoilUiGeneratingKeyView.qml

FOIL_UI_IMAGES = $${FOIL_UI_QML}/images/*.svg

OTHER_FILES += $${FOIL_UI_COMPONENTS}
foil_ui_components.files = $${FOIL_UI_COMPONENTS}
foil_ui_components.path = /usr/share/$${TARGET}/qml/foil-ui
INSTALLS += foil_ui_components

OTHER_FILES += $${FOIL_UI_IMAGES}
foil_ui_images.files = $${FOIL_UI_IMAGES}
foil_ui_images.path = /usr/share/$${TARGET}/qml/foil-ui/images
INSTALLS += foil_ui_images

# harbour-lib

INCLUDEPATH += \
    $${HARBOUR_LIB_INCLUDE}

HEADERS += \
    $${HARBOUR_LIB_INCLUDE}/HarbourBase32.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourDebug.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourOrganizeListModel.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourProcessState.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourQrCodeGenerator.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourQrCodeImageProvider.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourSelectionListModel.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourSingleImageProvider.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourSystemState.h \
    $${HARBOUR_LIB_INCLUDE}/HarbourTask.h \
    $${HARBOUR_LIB_SRC}/HarbourMce.h

SOURCES += \
    $${HARBOUR_LIB_SRC}/HarbourBase32.cpp \
    $${HARBOUR_LIB_SRC}/HarbourMce.cpp \
    $${HARBOUR_LIB_SRC}/HarbourOrganizeListModel.cpp \
    $${HARBOUR_LIB_SRC}/HarbourProcessState.cpp \
    $${HARBOUR_LIB_SRC}/HarbourQrCodeGenerator.cpp \
    $${HARBOUR_LIB_SRC}/HarbourQrCodeImageProvider.cpp \
    $${HARBOUR_LIB_SRC}/HarbourSelectionListModel.cpp \
    $${HARBOUR_LIB_SRC}/HarbourSingleImageProvider.cpp \
    $${HARBOUR_LIB_SRC}/HarbourSystemState.cpp \
    $${HARBOUR_LIB_SRC}/HarbourTask.cpp

HARBOUR_QML_COMPONENTS = \
    $${HARBOUR_LIB_QML}/HarbourFitLabel.qml \
    $${HARBOUR_LIB_QML}/HarbourHighlightIcon.qml \
    $${HARBOUR_LIB_QML}/HarbourIconTextButton.qml \
    $${HARBOUR_LIB_QML}/HarbourHintIconButton.qml \
    $${HARBOUR_LIB_QML}/HarbourPasswordInputField.qml \
    $${HARBOUR_LIB_QML}/HarbourShakeAnimation.qml \
    $${HARBOUR_LIB_QML}/HarbourTextFlip.qml

OTHER_FILES += $${HARBOUR_QML_COMPONENTS}

qml_components.files = $${HARBOUR_QML_COMPONENTS}
qml_components.path = /usr/share/$${TARGET}/qml/harbour
INSTALLS += qml_components

# openssl
!equals(QT_ARCH, arm64){
SOURCES += \
    $${HARBOUR_LIB_SRC}/libcrypto.c
}

# Icons
ICON_SIZES = 86 108 128 172 256
for(s, ICON_SIZES) {
    icon_target = icon$${s}
    icon_dir = icons/$${s}x$${s}
    $${icon_target}.files = $${icon_dir}/$${TARGET}.png
    $${icon_target}.path = /usr/share/icons/hicolor/$${s}x$${s}/apps
    equals(PREFIX, "openrepos") {
        $${icon_target}.extra = cp $${icon_dir}/harbour-$${NAME}.png $$eval($${icon_target}.files)
        $${icon_target}.CONFIG += no_check_exist
    }
    INSTALLS += $${icon_target}
}

# Desktop file
equals(PREFIX, "openrepos") {
    desktop.extra = sed s/harbour/openrepos/g harbour-$${NAME}.desktop > $${TARGET}.desktop
    desktop.CONFIG += no_check_exist
}

# Translations
TRANSLATION_IDBASED=-idbased
TRANSLATION_SOURCES = \
  $${_PRO_FILE_PWD_}/qml

defineTest(addTrFile) {
    rel = translations/$${1}
    OTHER_FILES += $${rel}.ts
    export(OTHER_FILES)

    in = $${_PRO_FILE_PWD_}/$$rel
    out = $${OUT_PWD}/$$rel

    s = $$replace(1,-,_)
    lupdate_target = lupdate_$$s
    qm_target = qm_$$s

    $${lupdate_target}.commands = lupdate -noobsolete -locations none $${TRANSLATION_SOURCES} -ts \"$${in}.ts\" && \
        mkdir -p \"$${OUT_PWD}/translations\" &&  [ \"$${in}.ts\" != \"$${out}.ts\" ] && \
        cp -af \"$${in}.ts\" \"$${out}.ts\" || :

    $${qm_target}.path = $$TRANSLATIONS_PATH
    $${qm_target}.depends = $${lupdate_target}
    $${qm_target}.commands = lrelease $$TRANSLATION_IDBASED \"$${out}.ts\" && \
        $(INSTALL_FILE) \"$${out}.qm\" $(INSTALL_ROOT)$${TRANSLATIONS_PATH}/

    QMAKE_EXTRA_TARGETS += $${lupdate_target} $${qm_target}
    INSTALLS += $${qm_target}

    export($${lupdate_target}.commands)
    export($${qm_target}.path)
    export($${qm_target}.depends)
    export($${qm_target}.commands)
    export(QMAKE_EXTRA_TARGETS)
    export(INSTALLS)
}

LANGUAGES = de fr hu pl ru sv zh_CN

addTrFile($${TARGET})
for(l, LANGUAGES) {
    addTrFile($${TARGET}-$$l)
}

qm.path = $$TRANSLATIONS_PATH
qm.CONFIG += no_check_exist
INSTALLS += qm
