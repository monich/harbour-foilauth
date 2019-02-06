TEMPLATE = subdirs
SUBDIRS = app qrencode zxing

app.file = app.pro
app.depends = qrencode-target zxing-target

qrencode.file = qrencode.pro
qrencode.target = qrencode-target

zxing.file = zxing.pro
zxing.target = zxing-target

OTHER_FILES += README.md LICENSE *.desktop rpm/*.spec
