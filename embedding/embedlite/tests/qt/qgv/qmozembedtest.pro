QT += opengl
SOURCES += textitem.cpp viewtab.cpp navbutton.cpp mainview.cpp main.cpp graphicsview.cpp embedcontext.cpp 
HEADERS += graphicsview.h navbutton.h mainview.h embedcontext.h viewtab.h textitem.h

TEMPLATE = app
CONFIG -= app_bundle
TARGET = $$PROJECT_NAME

PREFIX = /usr
isEmpty(OBJ_PATH) {
  message(OBJ_PATH not defined)
  CONFIG += link_pkgconfig
  PKGCONFIG += libxul-embedding mozilla-nspr
  GRE_HOME=$$system(pkg-config --variable=gredir libxul-embedding)
  SDK_HOME=$$system(pkg-config --variable=sdkdir libxul-embedding)
  SDK_LIBDIR=$$SDK_HOME/lib
  SDK_INC_PATH=$$SDK_HOME/include
} else {
  message(OBJ_PATH defined $$OBJ_PATH)
  GRE_HOME=$$OBJ_PATH/dist/bin
  SDK_LIBDIR=$$OBJ_PATH/dist/lib
  SDK_INC_PATH=$$OBJ_PATH/dist/include
}

OBJECTS_DIR += release
DESTDIR = ./release
MOC_DIR += ./release/tmp/moc/release_static
RCC_DIR += ./release/tmp/rcc/release_static

INCLUDEPATH += $$SDK_INC_PATH
INCLUDEPATH += $$SDK_INC_PATH/nspr

QMAKE_CXXFLAGS += -include mozilla-config.h
unix:QMAKE_CXXFLAGS += -fno-short-wchar -std=c++0x -fPIC
DEFINES += XPCOM_GLUE=1 XPCOM_GLUE_USE_NSPR=1 MOZ_GLUE_IN_PROGRAM=1

LIBS += -L$$SDK_LIBDIR -lxpcomglue -Wl,--whole-archive -lmozglue
LIBS += -Wl,--no-whole-archive -rdynamic -ldl

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
