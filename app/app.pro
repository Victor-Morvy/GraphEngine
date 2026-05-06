TEMPLATE = app
TARGET   = GraphEngineApp
CONFIG  += console c++17
CONFIG  -= app_bundle
QT      -= gui
QT      += core

SOURCES += main.cpp

INCLUDEPATH += $$PWD/../lib
DEPENDPATH  += $$PWD/../lib

# Link against the static GraphEngine library.
# Qt Creator resolves OUT_PWD per build kit (debug/release).
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../lib/release/ -lGraphEngine
else:win32:CONFIG(debug,   debug|release): LIBS += -L$$OUT_PWD/../lib/debug/   -lGraphEngine
else:unix:                                 LIBS += -L$$OUT_PWD/../lib/           -lGraphEngine

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/release/libGraphEngine.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/debug/libGraphEngine.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/release/GraphEngine.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/debug/GraphEngine.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../lib/libGraphEngine.a

# Copy grafo.json next to the executable after each build
win32 {
    QMAKE_POST_LINK += $$QMAKE_COPY \
        $$shell_path($$PWD/../grafo.json) \
        $$shell_path($$OUT_PWD/grafo.json)
}
unix {
    QMAKE_POST_LINK += cp $$PWD/../grafo.json $$OUT_PWD/grafo.json
}
