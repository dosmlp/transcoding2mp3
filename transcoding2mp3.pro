TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

#ffmpeg
INCLUDEPATH += D:/3rd/ffmpeg34_mt/include
DEPENDPATH += D:/3rd/ffmpeg34_mt/include
LIBS += -LD:/3rd/ffmpeg34_mt/lib -lswresample -lavutil -lavcodec -lavformat
#boost
INCLUDEPATH += D:/boost_1_68_0
DEPENDPATH += D:/boost_1_68_0
#LIBS += -LD:/boost_1_68_0/lib32-msvc-14.0 -llibboost_system-vc140-mt-x32-1_68
#lame
INCLUDEPATH += D:/3rd/lame/include
DEPENDPATH += D:/3rd/lame/include
LIBS += -LD:/3rd/lame/lib -lmp3lame
#qt
INCLUDEPATH += D:/Qt/Qt5.9.6_x86_mt/include
DEPENDPATH += D:/Qt/Qt5.9.6_x86_mt/include
LIBS += -LD:/Qt/Qt5.6.3/5.6.3/msvc2015/lib -lQt5Core

QMAKE_LFLAGS += /DEBUG /OPT:REF
#QMAKE_CFLAGS_RELEASE    = -O2 -MT
QMAKE_CXXFLAGS += -Zi

SOURCES += \
        main.cpp \
    EncodeTask.cpp

HEADERS += \
    EncodeTask.h \
    Global.h
