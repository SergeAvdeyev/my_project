TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

DESTDIR += "../bin"

INCLUDEPATH += ./include ./lapb_lib

SOURCES += ./src/server_main.c \
    ./lapb_lib/lapb_iface.c \
    ./lapb_lib/lapb_in.c \
    ./lapb_lib/lapb_out.c \
    ./lapb_lib/lapb_subr.c \
    ./lapb_lib/lapb_timer.c \
    ./src/tcp_server.c \
    ./src/my_timer.c \
    ./src/common.c \
    lapb_lib/lapb_queue.c

HEADERS += \
    ./lapb_lib/net_lapb.h \
    ./include/tcp_server.h \
    ./include/types_n_consts.h \
    ./include/my_timer.h \
    ./include/common.h \
    lapb_lib/lapb_queue.h


LIBS += -pthread
