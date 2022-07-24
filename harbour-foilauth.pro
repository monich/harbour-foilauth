TEMPLATE = subdirs
SUBDIRS = app qrencode zbar

app.file = app.pro
app.depends = qrencode-target zbar-target

qrencode.file = qrencode.pro
qrencode.target = qrencode-target

zbar.file = zbar/zbar.pro
zbar.target = zbar-target

OTHER_FILES += README.md LICENSE rpm/*.spec
