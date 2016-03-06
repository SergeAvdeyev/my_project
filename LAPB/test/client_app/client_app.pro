TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

DESTDIR += "../../../bin"

INCLUDEPATH += ../include ../../lib/include ../../lib/export

SOURCES += \
    ../src/client_main.c \
    ../src/tcp_client.c \
    ../src/my_timer.c \
    ../src/common.c \
    ../src/logger.c \
    ../../lib/src/lapb_iface.c \
    ../../lib/src/lapb_in.c \
    ../../lib/src/lapb_out.c \
    ../../lib/src/lapb_subr.c \
    ../../lib/src/lapb_timer.c \
    ../../lib/src/queue.c \
    ../../lib/src/subr.c

HEADERS += \
    ../include/tcp_client.h \
    ../include/my_timer.h \
    ../include/common.h \
    ../include/logger.h \
    ../../lib/export/lapb_iface.h \
    ../../lib/include/lapb_int.h \
    ../../lib/include/queue.h \
    ../../lib/include/subr.h

LIBS += -pthread
