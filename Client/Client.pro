QT  += \
    core gui \
    multimedia \
    multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++14

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h \
    rtp_io.hpp \
    sip_engine.hpp

FORMS += \
    mainwindow.ui

INCLUDEPATH +=    "C:\Dependencies\boost1_73_0"
INCLUDEPATH +=    "C:\Users\Maxim\Desktop\voip"
INCLUDEPATH +=    "C:\Dependencies\opencv\build\include"


LIBS +=  "-LC:\Dependencies\boost1_73_0\lib64-msvc-14.1"
LIBS +=  -L$$(OPENCV_DIR)\lib -lopencv_world343d

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
