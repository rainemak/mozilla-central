QT += opengl declarative
SOURCES += main.cpp qmlapplicationviewer.cpp
HEADERS += qmlapplicationviewer.h 

SOURCES += ../qtwidget/qdeclarativemozview.cpp ../qtwidget/qgraphicsmozview.cpp ../qtwidget/qmozcontext.cpp 
HEADERS += ../qtwidget/qdeclarativemozview.h ../qtwidget/qgraphicsmozview.h ../qtwidget/qmozcontext.h

QML_FILES = qml/*.qml
RESOURCES += qmlMozEmbedTest.qrc

TEMPLATE = app
CONFIG -= app_bundle
TARGET = $$PROJECT_NAME

PREFIX = /usr
#OBJ_PATH=/home/romaxa/build/mozilla-central/obj-xr-qt-opt
OBJ_PATH=/home/romaxa/build/mozilla-central/obj-build-n9-qt

INCLUDEPATH += $$OBJ_PATH/dist/include
INCLUDEPATH += $$OBJ_PATH/dist/include/nspr
INCLUDEPATH += ../qtwidget

OBJECTS_DIR += release
DESTDIR = ./release
MOC_DIR += ./release/tmp/moc/release_static
RCC_DIR += ./release/tmp/rcc/release_static

QMAKE_CXXFLAGS += -include mozilla-config.h
unix:QMAKE_CXXFLAGS += -fno-short-wchar -std=c++0x -fPIC
DEFINES += XPCOM_GLUE=1 XPCOM_GLUE_USE_NSPR=1 MOZ_GLUE_IN_PROGRAM=1

LIBS += -L$$OBJ_PATH/dist/lib -lxpcomglue -Wl,--whole-archive -lmozglue
LIBS += -Wl,--no-whole-archive -rdynamic -ldl
LIBS += $$OBJ_PATH/dist/lib/libqtembedwidget.a

target.path = $$PREFIX/bin
INSTALLS += target

contains(CONFIG,qdeclarative-boostable):contains(MEEGO_EDITION,harmattan) {
    DEFINES += HARMATTAN_BOOSTER
}

*-g++*: QMAKE_CXXFLAGS += -Wno-attributes
*-g++*: QMAKE_CXXFLAGS += -Wno-ignored-qualifiers
*-g++*: QMAKE_CXXFLAGS += -pedantic
*-g++*: QMAKE_CXXFLAGS += -Wall
*-g++*: QMAKE_CXXFLAGS += -Wno-unused-parameter
*-g++*: QMAKE_CXXFLAGS += -Wpointer-arith
*-g++*: QMAKE_CXXFLAGS += -Woverloaded-virtual
*-g++*: QMAKE_CXXFLAGS += -Werror=return-type
*-g++*: QMAKE_CXXFLAGS += -Wtype-limits
*-g++*: QMAKE_CXXFLAGS += -Wempty-body
*-g++*: QMAKE_CXXFLAGS += -Wno-ctor-dtor-privacy
*-g++*: QMAKE_CXXFLAGS += -Wno-format
*-g++*: QMAKE_CXXFLAGS += -Wno-overlength-strings
*-g++*: QMAKE_CXXFLAGS += -Wno-invalid-offsetof
*-g++*: QMAKE_CXXFLAGS += -Wno-variadic-macros
*-g++*: QMAKE_CXXFLAGS += -Wno-long-long
*-g++*: QMAKE_CXXFLAGS += -Wno-psabi
