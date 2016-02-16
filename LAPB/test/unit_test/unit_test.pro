TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

DESTDIR += "../../../bin"

INCLUDEPATH += ../include ../../lib/include ../../lib/export

SOURCES += \
    ../../lib/src/lapb_iface.c \
    ../../lib/src/lapb_in.c \
    ../../lib/src/lapb_out.c \
    ../../lib/src/lapb_subr.c \
    ../../lib/src/lapb_timer.c \
    ../../lib/src/lapb_queue.c \
    ../src/ut_main.c

HEADERS += \
    ../include/ut_main.h \
    ../../lib/include/lapb_queue.h \
    ../../lib/export/lapb_iface.h \
    ../../lib/include/lapb_int.h

LIBS += -pthread
