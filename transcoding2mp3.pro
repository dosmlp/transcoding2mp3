TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

#ffmpeg
INCLUDEPATH += D:/3rd/ffmpeg4_ltl/include
DEPENDPATH += D:/3rd/ffmpeg4_ltl/include
LIBS += -LD:/3rd/ffmpeg4_ltl/lib -lswresample -lavutil -lavcodec -lavformat
#lame
INCLUDEPATH += D:/3rd/lame/include
DEPENDPATH += D:/3rd/lame/include
LIBS += -LD:/3rd/lame/lib -llibmp3lame

QMAKE_LFLAGS += /DEBUG /OPT:REF
#QMAKE_CFLAGS_RELEASE    = -O2 -MT
QMAKE_CXXFLAGS += -Zi

SOURCES += \
        main.cpp \
    EncodeTask.cpp

HEADERS += \
    EncodeTask.h \
    Global.h \
    align_malloc.h
