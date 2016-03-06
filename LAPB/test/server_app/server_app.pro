TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

DESTDIR += "../../../bin"

INCLUDEPATH += ../include ../../lib/include ../../lib/export

SOURCES += ../src/server_main.c \
    ../../lib/src/lapb_iface.c \
    ../../lib/src/lapb_in.c \
    ../../lib/src/lapb_out.c \
    ../../lib/src/lapb_subr.c \
    ../../lib/src/lapb_timer.c \
    ../../lib/src/queue.c \
    ../../lib/src/subr.c \
    ../src/tcp_server.c \
    ../src/my_timer.c \
    ../src/common.c \
    ../src/logger.c

HEADERS += \
    ../../lib/export/lapb_iface.h \
    ../../lib/include/lapb_int.h \
    ../../lib/include/queue.h \
    ../../lib/include/subr.h \
    ../include/tcp_server.h \
    ../include/types_n_consts.h \
    ../include/my_timer.h \
    ../include/common.h \
    ../include/logger.h


LIBS += -pthread
