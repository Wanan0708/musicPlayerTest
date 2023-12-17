QT       += core gui multimedia network sql multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    datebase.cpp \
    lyricshow.cpp \
    main.cpp \
    mainwindow.cpp \
    mvshow.cpp \
    singlesheetshow.cpp

HEADERS += \
    datebase.h \
    lyricshow.h \
    mainwindow.h \
    mvshow.h \
    singlesheetshow.h

FORMS += \
    lyricshow.ui \
    mainwindow.ui \
    mvshow.ui \
    singlesheetshow.ui

TRANSLATIONS += trans_zh_CN.ts \
    trans_en_US.ts

RC_ICONS = ./image/dinosaur.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    image.qrc \
    translate.qrc
