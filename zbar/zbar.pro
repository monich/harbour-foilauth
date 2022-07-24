TEMPLATE = lib
CONFIG += static
TARGET = zbar
QT-= gui

QMAKE_CFLAGS += \
    -Wno-unused-parameter \
    -Wno-sign-compare \
    -Wno-override-init \
    -Wno-missing-field-initializers \
    -Wno-parentheses \
    -Wno-implicit-fallthrough

ZBAR_DIR = $${_PRO_FILE_PWD_}/zbar

INCLUDEPATH += \
    $${ZBAR_DIR}/include \
    $${ZBAR_DIR}/zbar \
    $${ZBAR_DIR}

# qmake thinks that gets image.c is included by binarize.c and
# doesn't generate a rule for it
SOURCES += zbar_image.c

SOURCES += \
    $${ZBAR_DIR}/zbar/convert.c \
    $${ZBAR_DIR}/zbar/decoder.c \
    $${ZBAR_DIR}/zbar/error.c \
    $${ZBAR_DIR}/zbar/image.c \
    $${ZBAR_DIR}/zbar/img_scanner.c \
    $${ZBAR_DIR}/zbar/refcnt.c \
    $${ZBAR_DIR}/zbar/scanner.c \
    $${ZBAR_DIR}/zbar/symbol.c \
    $${ZBAR_DIR}/zbar/video.c \
    $${ZBAR_DIR}/zbar/decoder/qr_finder.c \
    $${ZBAR_DIR}/zbar/qrcode/binarize.c \
    $${ZBAR_DIR}/zbar/qrcode/bch15_5.c \
    $${ZBAR_DIR}/zbar/qrcode/isaac.c \
    $${ZBAR_DIR}/zbar/qrcode/qrdec.c \
    $${ZBAR_DIR}/zbar/qrcode/qrdectxt.c \
    $${ZBAR_DIR}/zbar/qrcode/util.c \
    $${ZBAR_DIR}/zbar/qrcode/rs.c \
    $${ZBAR_DIR}/zbar/video/null.c
