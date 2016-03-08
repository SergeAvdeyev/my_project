TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

DESTDIR += "../../../bin"

INCLUDEPATH += ../include

INCLUDEPATH += ../../lib/include
INCLUDEPATH += ../../lib/export

INCLUDEPATH += ../../LAPB/lib/export
INCLUDEPATH += ../../LAPB/lib/include

SOURCES += \
    ../src/client_main.c \
    ../src/common.c \
    ../src/tcp_client.c \
    ../src/my_timer.c \
    ../src/logger.c \
    ../../lib/src/x25_iface.c \
    ../../lib/src/x25_in.c \
    ../../lib/src/x25_out.c \
    ../../lib/src/x25_subr.c \
    ../../lib/src/x25_timer.c \
    ../../lib/src/x25_restart.c \
    ../../lib/src/x25_facilities.c \
    ../../lib/src/x25_link.c \
#
    ../../LAPB/lib/src/queue.c \
    ../../LAPB/lib/src/subr.c \
    ../../LAPB/lib/src/lapb_iface.c \
    ../../LAPB/lib/src/lapb_timer.c \
    ../../LAPB/lib/src/lapb_in.c \
    ../../LAPB/lib/src/lapb_out.c \
    ../../LAPB/lib/src/lapb_subr.c



HEADERS += \
    ../include/common.h \
    ../include/tcp_client.h \
    ../include/my_timer.h \
    ../include/logger.h \
    ../../lib/export/x25_iface.h \
    ../../lib/include/x25_int.h \
    ../../LAPB/lib/include/lapb_int.h \
    ../../LAPB/lib/include/queue.h \
    ../../LAPB/lib/include/subr.h

LIBS += -pthread
