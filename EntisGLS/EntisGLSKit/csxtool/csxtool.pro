TEMPLATE = app
CONFIG += console
CONFIG -= qt

LIBS += -liconv
QMAKE_LFLAGS += -static -static-libgcc -static-libstdc++
CONFIG(release, debug|release) {
    QMAKE_CXXFLAGS += -DNDEBUG
}

SOURCES += main.cpp \
    functionsection.cpp \
    section.cpp \
    imagesection.cpp \
    unicodestring.cpp \
    csxparser.cpp \
    positioncomputer.cpp \
    header.cpp \
    talk.cpp \
    choice.cpp \
    txtparser.cpp

HEADERS += \
    functionsection.h \
    section.h \
    imagesection.h \
    unicodestring.h \
    csxparser.h \
    positioncomputer.h \
    header.h \
    talk.h \
    choice.h \
    txtparser.h

OTHER_FILES +=

