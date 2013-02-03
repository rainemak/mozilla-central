QT += opengl declarative
SOURCES += main.cpp qmlapplicationviewer.cpp
HEADERS += qmlapplicationviewer.h 

SOURCES += ../qtwidget/qdeclarativemozview.cpp ../qtwidget/qgraphicsmozview.cpp ../qtwidget/qmozcontext.cpp ../qtwidget/EmbedQtKeyUtils.cpp
HEADERS += ../qtwidget/qdeclarativemozview.h ../qtwidget/qgraphicsmozview.h ../qtwidget/qmozcontext.h ../qtwidget/EmbedQtKeyUtils.h

QML_FILES = qml/*.qml
RESOURCES += qmlMozEmbedTest.qrc

TEMPLATE = app
CONFIG -= app_bundle
CONFIG += link_pkgconfig
TARGET = $$PROJECT_NAME

PREFIX = /usr

INCLUDEPATH += ../qtwidget

PKGCONFIG += QJson

OBJECTS_DIR += release
DESTDIR = ./release
MOC_DIR += ./release/tmp/moc/release_static
RCC_DIR += ./release/tmp/rcc/release_static

include(qmozembed.pri)

target.path = $$PREFIX/bin
INSTALLS += target

contains(CONFIG,qdeclarative-boostable):contains(MEEGO_EDITION,harmattan) {
    DEFINES += HARMATTAN_BOOSTER
}
